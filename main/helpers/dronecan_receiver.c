#include "canard.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/twai.h"

#include "helpers/dronecan_dna_receiver.h"
#include "helpers/dronecan_node_state.h"
#include "helpers/esp_can.h"

#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.RestartNode-5.h"
#include "messages/uavcan.protocol.param.GetSet-11.h"
#include "messages/uavcan.protocol.param.ExecuteOpcode-10.h"
#include "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.h"
#include "messages/uavcan.protocol.file.Read-48.h"
#include "messages/uavcan.protocol.dynamic_node_id.Allocation-1.h"
#include "messages/uavcan.protocol.GetTransportStats-4.h"

static const char *TAG = "DroneCAN receiver";
static uint64_t logical_rx = 0;

void restart(void *arg)
{
    ESP_LOGI(TAG, "Restarting node...");
    esp_restart();
}

void on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    logical_rx++;
    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID)
    {
        transfer_received_for_dna(ins, transfer);
    }
    else if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        switch (transfer->data_type_id)
        {
        case UAVCAN_GET_NODE_INFO_ID:
            uint32_t uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
            response_1_getNodeInfo(transfer->source_node_id, &transfer->transfer_id, get_unique_id(), uptime, get_node_health(), get_node_mode());
            break;
        case UAVCAN_RESTART_NODE_ID:
            if (check_response_5_restart_transfer_valid(transfer))
            {
                response_5_restartNode(transfer->source_node_id, &transfer->transfer_id);

                esp_timer_handle_t timer;
                esp_timer_create(&(esp_timer_create_args_t){.callback = restart}, &timer);
                esp_timer_start_once(timer, 1000000);
            }
            break;
        case UAVCAN_PARAM_GETSET_ID:
            uint16_t param_index = decode_11_paramGetSet_request(transfer, get_device_parameters(), get_device_parameters_len());

            if (param_index < get_device_parameters_len())
            {
                response_11_paramGetSet(transfer->source_node_id, &transfer->transfer_id, &get_device_parameters()[param_index]);
            }
            else
            {
                response_11_paramGetSetEmpty(transfer->source_node_id, &transfer->transfer_id);
            }
            break;
        case UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID:
            response_10_paramExecuteOpcode_process(transfer, process_parameters_to_nvs(decode_10_paramExecuteOpcode_process(transfer)));
            break;
        case UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID:
            if (*get_node_mode() == MODE_SOFTWARE_UPDATE)
            {
                response_40_beginFirmwareUpdate(IN_PROGRESS, transfer->source_node_id, &transfer->transfer_id);
                break;
            }
            set_node_mode(MODE_SOFTWARE_UPDATE);
            if (process_40_beginFirmwareUpdate(transfer))
            {
                response_40_beginFirmwareUpdate(OK, transfer->source_node_id, &transfer->transfer_id);
                request_read_48(get_firmware_source_node_id(), 0, get_firmware_path());
            }
            else
            {
                response_40_beginFirmwareUpdate(INVALID_MODE, transfer->source_node_id, &transfer->transfer_id);
            }

            break;
        case UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID:
            twai_status_info_t status;
            twai_get_status_info(&status);
            uint64_t phys_err = (uint64_t)status.rx_missed_count +
                                (uint64_t)status.rx_overrun_count +
                                (uint64_t)status.tx_failed_count +
                                (uint64_t)status.arb_lost_count +
                                (uint64_t)status.bus_error_count;
            response_4_getTransportStats(transfer->source_node_id, &transfer->transfer_id, phys_err, get_physical_tx(), get_physical_rx(), get_logical_error(), get_logical_tx(), logical_rx);
            break;
        default:
            ESP_LOGW(TAG, "UNKNOWN TRANSFER RECEIVED: type_id=%d, source_node_id=%d", transfer->data_type_id, transfer->source_node_id);
            break;
        }
    }
    else if (transfer->transfer_type == CanardTransferTypeResponse)
    {
        switch (transfer->data_type_id)
        {
        case UAVCAN_FILE_READ_ID:
            if (*get_node_mode() == MODE_SOFTWARE_UPDATE)
            {
                firmware_file_chunk_received(transfer);
            }

            break;
        default:
            ESP_LOGW(TAG, "UNKNOWN RESPONSE RECEIVED: type_id=%d, source_node_id=%d", transfer->data_type_id, transfer->source_node_id);
            break;
        }
    }
}