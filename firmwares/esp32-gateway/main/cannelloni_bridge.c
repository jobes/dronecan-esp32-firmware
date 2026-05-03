#include "cannelloni_bridge.h"
#include "dronecan_mini/dronecan_communication.h"
#include "dronecan_mini/dronecan_tasks.h"
#include "esp_can.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "project_config.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "messages/uavcan.protocol.GetTransportStats-4.h"

static const char *TAG = "CANNELLONI";

typedef struct
{
  struct sockaddr_in addr;
  uint32_t last_seen_ms;
  bool active;
  bool has_seq;
  uint8_t seq;
} cannelloni_peer_t;

static cannelloni_peer_t peers[CANNELLONI_MAX_PEERS];
static QueueHandle_t rx_bridge_queue = NULL;

static uint64_t cannelloni_rx_frames = 0;
static uint64_t cannelloni_tx_frames = 0;
static uint64_t cannelloni_rx_errors = 0;

uint8_t get_extra_iface_stats(uavcan_protocol_GetTransportStats_CanIfaceStats *stats)
{
  stats->tx_requests = cannelloni_tx_frames;
  stats->rx_messages = cannelloni_rx_frames;
  stats->errors = cannelloni_rx_errors;
  return 1;
}

// Cannelloni protocol V2 constants
#define CANNELLONI_VERSION 2
#define CANNELLONI_OP_DATA 0
#define CANNELLONI_HEADER_SIZE 5 // version(1), op_code(1), seq(1), count(2)
#define CANNELLONI_FRAME_SIZE 13 // id(4), length(1), data(8)
#define MAX_FRAMES_PER_PACKET 10

static uint32_t update_peer(struct sockaddr_in *addr, uint8_t seq)
{
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  int first_free = -1;
  uint32_t dropped = 0;

  for (int i = 0; i < CANNELLONI_MAX_PEERS; i++)
  {
    if (peers[i].active)
    {
      if (peers[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
          peers[i].addr.sin_port == addr->sin_port)
      {
        peers[i].last_seen_ms = now;
        if (peers[i].has_seq)
        {
          uint8_t expected_seq = (peers[i].seq + 1) & 0xFF;
          if (seq != expected_seq)
          {
            dropped = (seq - expected_seq) & 0xFF;
          }
        }
        peers[i].seq = seq;
        peers[i].has_seq = true;
        return dropped;
      }
    }
    else if (first_free == -1)
    {
      first_free = i;
    }
  }

  if (first_free != -1)
  {
    peers[first_free].addr = *addr;
    peers[first_free].last_seen_ms = now;
    peers[first_free].active = true;
    peers[first_free].seq = seq;
    peers[first_free].has_seq = true;
  }
  return 0;
}

static void prune_peers()
{
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  for (int i = 0; i < CANNELLONI_MAX_PEERS; i++)
  {
    if (peers[i].active)
    {
      if (now - peers[i].last_seen_ms > CANNELLONI_PEER_TIMEOUT_MS)
      {
        peers[i].active = false;
      }
    }
  }
}

void cannelloni_bridge_push_frame(const CanardCANFrame *frame)
{
  if (rx_bridge_queue != NULL)
  {
    xQueueSend(rx_bridge_queue, frame, 0);
  }
}

static void cannelloni_udp_rx_task(void *pvParameters)
{
  int sock = (int)(intptr_t)pvParameters;
  uint8_t rx_buffer[1500];
  struct sockaddr_in source_addr;
  socklen_t addr_len = sizeof(source_addr);

  while (1)
  {
    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0,
                       (struct sockaddr *)&source_addr, &addr_len);
    if (len < 0)
    {
      ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (len < CANNELLONI_HEADER_SIZE)
    {
      cannelloni_rx_errors++;
      continue;
    }

    uint8_t version = rx_buffer[0];
    uint8_t op_code = rx_buffer[1];
    uint8_t seq = rx_buffer[2];
    uint16_t count = (rx_buffer[3] << 8) | rx_buffer[4];

    if (version != CANNELLONI_VERSION || op_code != CANNELLONI_OP_DATA)
    {
      cannelloni_rx_errors++;
      continue;
    }

    cannelloni_rx_errors += update_peer(&source_addr, seq);

    int pos = CANNELLONI_HEADER_SIZE;
    for (int i = 0; i < count; i++)
    {

      CanardCANFrame frame;
      uint32_t id = ((uint32_t)rx_buffer[pos] << 24) |
                    ((uint32_t)rx_buffer[pos + 1] << 16) |
                    ((uint32_t)rx_buffer[pos + 2] << 8) |
                    (uint32_t)rx_buffer[pos + 3];

      // Check EFF flag in Linux format (native Cannelloni)
      if (id & (1UL << 31))
      {
        frame.id = (id & 0x1FFFFFFF) | CANARD_CAN_FRAME_EFF;
      }
      else
      {
        frame.id = (id & 0x7FF);
      }

      frame.data_len = rx_buffer[pos + 4];
      memcpy(frame.data, &rx_buffer[pos + 5], frame.data_len);

      can_transmit(&frame);
      dronecan_inject_rx_frame(&frame);
      cannelloni_rx_frames++;
      pos += CANNELLONI_FRAME_SIZE;
    }
  }
}

static void cannelloni_udp_tx_task(void *pvParameters)
{
  int sock = (int)(intptr_t)pvParameters;
  CanardCANFrame frames[MAX_FRAMES_PER_PACKET];
  uint8_t tx_buffer[CANNELLONI_HEADER_SIZE +
                    (CANNELLONI_FRAME_SIZE * MAX_FRAMES_PER_PACKET)];
  uint8_t seq = 0;

  ESP_LOGI(TAG, "UDP TX task started (V2) with batching (max %d frames)",
           MAX_FRAMES_PER_PACKET);

  while (1)
  {
    uint16_t count = 0;
    // Block until we have at least one frame
    if (xQueueReceive(rx_bridge_queue, &frames[count],
                      pdMS_TO_TICKS(CANNELLONI_TX_INTERVAL_MS)) == pdPASS)
    {
      count++;
      // Try to get more frames to batch them
      while (count < MAX_FRAMES_PER_PACKET &&
             xQueueReceive(rx_bridge_queue, &frames[count], 0) == pdPASS)
      {
        count++;
      }

      prune_peers();

      // Prepare packet header
      tx_buffer[0] = CANNELLONI_VERSION;
      tx_buffer[1] = CANNELLONI_OP_DATA;
      tx_buffer[2] = seq;
      tx_buffer[3] = (count >> 8) & 0xFF;
      tx_buffer[4] = count & 0xFF;

      int pos = CANNELLONI_HEADER_SIZE;
      for (int i = 0; i < count; i++)
      {
        uint32_t id = frames[i].id & 0x1FFFFFFF;
        if (frames[i].id & CANARD_CAN_FRAME_EFF)
        {
          id |= (1UL << 31);
        }

        tx_buffer[pos + 0] = (id >> 24) & 0xFF;
        tx_buffer[pos + 1] = (id >> 16) & 0xFF;
        tx_buffer[pos + 2] = (id >> 8) & 0xFF;
        tx_buffer[pos + 3] = id & 0xFF;
        tx_buffer[pos + 4] = frames[i].data_len;

        memset(&tx_buffer[pos + 5], 0, 8);
        memcpy(&tx_buffer[pos + 5], frames[i].data, frames[i].data_len);

        cannelloni_tx_frames++;
        pos += CANNELLONI_FRAME_SIZE;
      }

      int total_len = CANNELLONI_HEADER_SIZE + (count * CANNELLONI_FRAME_SIZE);

      int active_peers = 0;
      for (int i = 0; i < CANNELLONI_MAX_PEERS; i++)
      {
        if (peers[i].active)
        {
          active_peers++;
          sendto(sock, tx_buffer, total_len, 0,
                 (struct sockaddr *)&peers[i].addr, sizeof(peers[i].addr));
        }
      }

      if (active_peers == 0)
      {
        struct sockaddr_in broadcast_addr;
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(CANNELLONI_UDP_PORT);
        broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
        sendto(sock, tx_buffer, total_len, 0,
               (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
      }
      seq++;
    }
    else
    {
      prune_peers();
    }
  }
}

void cannelloni_bridge_init(void)
{
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0)
  {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    return;
  }

  int broadcast = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                 sizeof(broadcast)) < 0)
  {
    ESP_LOGE(TAG, "Socket unable to set SO_BROADCAST: errno %d", errno);
    close(sock);
    return;
  }

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(CANNELLONI_UDP_PORT);

  if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
  {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    close(sock);
    return;
  }

  rx_bridge_queue = xQueueCreate(64, sizeof(CanardCANFrame));
  memset(peers, 0, sizeof(peers));

  xTaskCreatePinnedToCore(cannelloni_udp_rx_task, "cannelloni_rx", 4096,
                          (void *)(intptr_t)sock, 6, NULL, 0); // Core 0 - Network
  xTaskCreatePinnedToCore(cannelloni_udp_tx_task, "cannelloni_tx", 4096,
                          (void *)(intptr_t)sock, 6, NULL, 0); // Core 0 - Network
}
