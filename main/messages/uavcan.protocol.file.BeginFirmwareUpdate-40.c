#include "esp_log.h"
#include "helpers/dronecan_communication.h"
#include "messages/uavcan.protocol.file.Read-48.h"
#include "esp_ota_ops.h"
#include "canard.h"
#include "helpers/dronecan_node_state.h"

#include "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.h"

static char *firmware_path = NULL;
static uint8_t firmware_source_node_id = 0;
static char *firmware_file_chunk_content = NULL;
static size_t firmware_file_chunk_start = 0;
static size_t firmware_file_chunk_offset = 0;
static esp_ota_handle_t update_handle = 0;
static const esp_partition_t *partition = NULL;

bool process_40_beginFirmwareUpdate(CanardRxTransfer *transfer)
{
    ESP_LOGI("FIRMWARE_UPDATE", "Begin firmware update request received");
    partition = esp_ota_get_next_update_partition(NULL);
    if (partition == NULL)
    {
        ESP_LOGI("FIRMWARE_UPDATE", "Failed to get next update partition");
        ESP_LOGE("FIRMWARE_UPDATE", "Failed to get next update partition");
        return false;
    }
    esp_err_t err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI("FIRMWARE_UPDATE", "Failed to begin OTA update");
        ESP_LOGE("FIRMWARE_UPDATE", "Failed to begin OTA update");
        return false;
    }
    canardDecodeScalar(transfer, 0, 8, false, &firmware_source_node_id);

    if (firmware_path != NULL)
    {
        free(firmware_path);
    }
    firmware_path = malloc(transfer->payload_len);
    firmware_path[transfer->payload_len - 1] = '\0';
    for (int i = 0; i < transfer->payload_len - 1; i++)
    {
        canardDecodeScalar(transfer, (i + 1) * 8, 8, false, &firmware_path[i]);
    }
    ESP_LOGI("FIRMWARE_UPDATE", "Firmware path: %s, source node id: %d", firmware_path, firmware_source_node_id);
    return true;
}

bool response_40_beginFirmwareUpdate(enum RESPONSE_TYPE response_type, uint8_t destination_node_id, uint8_t *transfer_id)
{
    uint8_t buffer[2] = {0};
    buffer[0] = (uint8_t)response_type;

    return dronecan_respond(
        destination_node_id,
        transfer_id,
        UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_SIGNATURE,
        UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID,
        CANARD_TRANSFER_PRIORITY_HIGH,
        buffer,
        sizeof(buffer));
}

void reset_firmware_chunk_file()
{
    if (firmware_file_chunk_content != NULL)
    {
        free(firmware_file_chunk_content);
        firmware_file_chunk_content = NULL;
    }
    firmware_file_chunk_start += firmware_file_chunk_offset;
    firmware_file_chunk_offset = 0;
}

void cleanup_firmware_update()
{
    if (firmware_path != NULL)
    {
        free(firmware_path);
        firmware_path = NULL;
    }
    firmware_source_node_id = 0;
    if (firmware_file_chunk_content != NULL)
    {
        free(firmware_file_chunk_content);
        firmware_file_chunk_content = NULL;
    }
    firmware_file_chunk_start = 0;
    firmware_file_chunk_offset = 0;
}

void firmware_file_chunk_received(CanardRxTransfer *transfer)
{
    ESP_LOGI("FIRMWARE_UPDATE", "Firmware file chunk received, size: %d bytes", transfer->payload_len - 2);
    char *temp = realloc(firmware_file_chunk_content, transfer->payload_len - 2 + firmware_file_chunk_offset);
    if (temp == 0)
    {
        ESP_LOGE("FIRMWARE_UPDATE", "Failed to allocate memory for firmware file content");
        set_node_mode(MODE_OFFLINE);
        cleanup_firmware_update();
        esp_ota_abort(update_handle);
        return;
    }
    firmware_file_chunk_content = temp;
    if (!arrived_response_read_48(transfer, &firmware_file_chunk_content[firmware_file_chunk_offset]))
    {
        ESP_LOGE("FIRMWARE_UPDATE", "Firmware file getting error in read file response");
        set_node_mode(MODE_OFFLINE);
        cleanup_firmware_update();
        esp_ota_abort(update_handle);
        return;
    }
    firmware_file_chunk_offset += transfer->payload_len - 2;
    if (firmware_file_chunk_offset > 4 * 1000) // 4KB, save to flash should be done in chunks, but for simplicity we just limit file size here
    {
        // save to memory in chunks and not all at once, also check free memory before saving chunk
        ESP_LOGI("FIRMWARE_UPDATE", "Firmware file size exceeds 4KB limit, saving to flash and resetting content");
        if (esp_ota_write(update_handle, firmware_file_chunk_content, firmware_file_chunk_offset) != ESP_OK)
        {
            ESP_LOGE("FIRMWARE_UPDATE", "Failed to write firmware file chunk to flash");
            set_node_mode(MODE_OFFLINE);
            cleanup_firmware_update();
            esp_ota_abort(update_handle);
            return;
        }

        reset_firmware_chunk_file();
    }
    if (transfer->payload_len - 2 == 0)
    {
        // save last chunk and start firmware update + restart
        if (esp_ota_write(update_handle, firmware_file_chunk_content, firmware_file_chunk_offset) != ESP_OK)
        {
            ESP_LOGE("FIRMWARE_UPDATE", "Failed to write last firmware file chunk to flash");
            set_node_mode(MODE_OFFLINE);
            cleanup_firmware_update();
            esp_ota_abort(update_handle);
            return;
        }
        if (esp_ota_end(update_handle) != ESP_OK)
        {
            ESP_LOGE("FIRMWARE_UPDATE", "Failed to end OTA update");
            set_node_mode(MODE_OFFLINE);
            cleanup_firmware_update();
            esp_ota_abort(update_handle);
            return;
        }
        ESP_LOGI("FIRMWARE_UPDATE", "Firmware file content received completely, size: %d bytes", firmware_file_chunk_offset);
        cleanup_firmware_update();
        if (esp_ota_set_boot_partition(partition) != ESP_OK)
        {
            ESP_LOGE("FIRMWARE_UPDATE", "Failed to set boot partition");
            set_node_mode(MODE_OFFLINE);
        }
        esp_restart();
    }
    else
    {
        request_read_48(firmware_source_node_id, firmware_file_chunk_start + firmware_file_chunk_offset, firmware_path);
    }
}

uint8_t get_firmware_source_node_id()
{
    return firmware_source_node_id;
}

char *get_firmware_path()
{
    return firmware_path;
}