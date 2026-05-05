#include "dronecan_dna_server.h"
#include "dronecan_communication.h"
#include "dronecan_node_monitor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.NodeStatus-341.h"
#include "messages/uavcan.protocol.dynamic_node_id.Allocation-1.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "DNA_SERVER";

#define NVS_NAMESPACE_DNA "dc_dna"
#define MAX_ALLOCATIONS 100
#define DYNAMIC_ID_MIN 10
#define DYNAMIC_ID_MAX 125

typedef struct
{
  uint8_t uid[16];
  uint8_t node_id;
} dna_allocation_t;

static dna_allocation_t s_allocations[MAX_ALLOCATIONS];
static int s_allocation_count = 0;

bool dronecan_dna_server_get_uid(uint8_t node_id, uint8_t *out_uid)
{
  for (int i = 0; i < s_allocation_count; i++)
  {
    if (s_allocations[i].node_id == node_id)
    {
      memcpy(out_uid, s_allocations[i].uid, 16);
      return true;
    }
  }
  return false;
}

typedef struct
{
  uint8_t uid[16];
  uint8_t collected_mask; // bits 0, 1, 2 for parts
} pending_allocation_t;

static pending_allocation_t s_pending_allocation;

static uint32_t s_occupancy_mask[4]; // 128 bits for node IDs 0-127; even for
                                     // static IDs not saved in NVS

static nvs_handle_t s_nvs_handle = 0;

static void mark_occupied(uint8_t node_id)
{
  if (node_id < 128)
  {
    s_occupancy_mask[node_id / 32] |= (1UL << (node_id % 32));
  }
}

static bool is_occupied(uint8_t node_id)
{
  if (node_id >= 128)
    return true;
  return (s_occupancy_mask[node_id / 32] & (1UL << (node_id % 32))) != 0;
}

void dronecan_dna_server_init(void)
{
  ESP_LOGI(TAG, "Initializing DroneCAN DNA server...");

  esp_err_t err = nvs_open(NVS_NAMESPACE_DNA, NVS_READWRITE, &s_nvs_handle);
  if (err == ESP_OK)
  {
    size_t required_size = sizeof(s_allocations);
    err = nvs_get_blob(s_nvs_handle, "allocs", s_allocations, &required_size);
    if (err == ESP_OK && required_size > 0)
    {
      s_allocation_count = required_size / sizeof(dna_allocation_t);
      ESP_LOGI(TAG, "Loaded %d allocations from NVS", s_allocation_count);
    }
  }
  else
  {
    ESP_LOGW(TAG, "Could not open NVS for DNA server: %s",
             esp_err_to_name(err));
  }

  // Mark local node ID as occupied
  mark_occupied(canardGetLocalNodeID(get_dronecan_instance()));

  dronecan_node_monitor_init();
}

static void save_allocations(void)
{
  if (s_nvs_handle != 0)
  {
    nvs_set_blob(s_nvs_handle, "allocs", s_allocations,
                 s_allocation_count * sizeof(dna_allocation_t));
    nvs_commit(s_nvs_handle);
  }
}

// Find a free node ID in the dynamic range that is not occupied (by static node
// id) and not already allocated (by dynamic node id)
static uint8_t find_free_node_id(void)
{
  // Try to find a free ID in the dynamic range, starting from top
  for (uint8_t id = DYNAMIC_ID_MAX; id >= DYNAMIC_ID_MIN; id--)
  {
    if (!is_occupied(id))
    {
      // Check if already allocated to someone else
      bool already_allocated = false;
      for (int i = 0; i < s_allocation_count; i++)
      {
        if (s_allocations[i].node_id == id)
        {
          already_allocated = true;
          break;
        }
      }
      if (!already_allocated)
        return id;
    }
  }
  return 0;
}

static void process_completed_allocation(const uint8_t *uid)
{
  ESP_LOGI(TAG, "Full UID collected, issuing node ID...");

  // 1. Check if we already have an assignment for this UID
  uint8_t allocated_id = 0;
  for (int i = 0; i < s_allocation_count; i++)
  {
    if (memcmp(s_allocations[i].uid, uid, 16) == 0)
    {
      allocated_id = s_allocations[i].node_id;
      break;
    }
  }

  // 2. If new node, find a free ID and save it
  if (allocated_id == 0)
  {
    allocated_id = find_free_node_id();
    if (allocated_id != 0 && s_allocation_count < MAX_ALLOCATIONS)
    {
      memcpy(s_allocations[s_allocation_count].uid, uid, 16);
      s_allocations[s_allocation_count].node_id = allocated_id;
      s_allocation_count++;
      save_allocations();
    }
  }

  // 3. Respond if we have an ID
  if (allocated_id != 0)
  {
    publish_1_allocation_server_response(allocated_id, uid, 16);
  }
  else
  {
    ESP_LOGE(
        TAG,
        "Failed to allocate node ID (no free IDs or too many allocations)");
  }
}

static void handle_allocation_request(CanardInstance *ins,
                                      CanardRxTransfer *transfer)
{
  if (transfer->payload_len < 2)
    return;

  uint8_t header = transfer->payload_head[0];
  uint8_t part_len = transfer->payload_len - 1;
  const uint8_t *part_data = &transfer->payload_head[1];

  // Header is 1 for Part 1, 0 for Parts 2 and 3.
  // Part 2 has 6 bytes of UID, Part 3 has 4 bytes of UID.
  int part_index = -1;
  if (header == 1 && part_len == 6)
  {
    part_index = 0;
  }
  else if (header == 0)
  {
    if (part_len == 6)
    {
      part_index = 1;
    }
    else if (part_len == 4)
    {
      part_index = 2;
    }
  }

  if (part_index < 0)
  {
    ESP_LOGW(TAG, "Unknown DNA part: header=%d, len=%d", header, part_len);
    return;
  }

  ESP_LOGI(TAG, "DNA Request: part=%d, len=%d, mask=0x%02x", part_index,
           part_len, s_pending_allocation.collected_mask);

  if (part_index == 0)
  {
    // Start or restart pending allocation session
    memset(s_pending_allocation.uid, 0, 16);
    memcpy(s_pending_allocation.uid, part_data, part_len);
    s_pending_allocation.collected_mask = (1 << 0);
    ESP_LOGI(TAG, "Started pending DNA allocation for UID prefix %02x%02x",
             part_data[0], part_data[1]);

    // Respond to Part 1 to acknowledge prefix
    publish_1_allocation_server_response(0, s_pending_allocation.uid, 6);
    return;
  }

  // Continue existing session
  if (s_pending_allocation.collected_mask == 0)
    return;

  int offset = (part_index == 1) ? 6 : 12;
  if (!(s_pending_allocation.collected_mask & (1 << part_index)))
  {
    memcpy(&s_pending_allocation.uid[offset], part_data, part_len);
    s_pending_allocation.collected_mask |= (1 << part_index);

    if (s_pending_allocation.collected_mask == 0x07) // All 3 parts collected
    {
      process_completed_allocation(s_pending_allocation.uid);
      s_pending_allocation.collected_mask = 0;
    }
    else
    {
      // Respond to acknowledge prefix (Part 1 or 2)
      publish_1_allocation_server_response(0, s_pending_allocation.uid,
                                           offset + part_len);
    }
  }
}

void dronecan_extra_on_transfer_received(CanardInstance *ins,
                                         CanardRxTransfer *transfer)
{
  dronecan_node_monitor_process_transfer(ins, transfer);
  if (transfer->data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID)
  {
    handle_allocation_request(ins, transfer);
  }
  else if (transfer->data_type_id == UAVCAN_NODE_STATUS_ID)
  {
    mark_occupied(transfer->source_node_id);
  }
}

bool dronecan_extra_should_accept(const CanardInstance *ins,
                                  uint64_t *out_data_type_signature,
                                  uint16_t data_type_id,
                                  CanardTransferType transfer_type,
                                  uint8_t source_node_id)
{
  if (s_nvs_handle == 0)
  {
    return false; // not initialized yet, ignore messages
  }
  if (transfer_type == CanardTransferTypeBroadcast)
  {
    if (data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID)
    {
      *out_data_type_signature =
          UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
      return true;
    }
    if (data_type_id == UAVCAN_NODE_STATUS_ID)
    {
      *out_data_type_signature = UAVCAN_NODE_STATUS_SIGNATURE;
      return true;
    }
  }
  else if (transfer_type == CanardTransferTypeResponse)
  {
    if (data_type_id == UAVCAN_GET_NODE_INFO_ID)
    {
      *out_data_type_signature = UAVCAN_GET_NODE_INFO_SIGNATURE;
      return true;
    }
  }
  return false;
}
