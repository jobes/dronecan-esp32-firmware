#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dronecan_node.h"

static const char* TAG = "Main";

// Uptime counter (externá referencia)
extern uint32_t g_uptime_sec;

// ============================================================================
// FreeRTOS Tasks
// ============================================================================

/**
 * Task pre spracovanie DroneCAN správ (vysoká priorita)
 */
static void dronecan_spin_task(void* arg) {
    ESP_LOGI(TAG, "DroneCAN spin task started");

    while (1) {
        dronecan_spin();
        vTaskDelay(pdMS_TO_TICKS(1));  // 1 kHz loop rate
    }
}

/**
 * Task pre publikovanie NodeStatus (1 Hz)
 */
static void node_status_task(void* arg) {
    ESP_LOGI(TAG, "NodeStatus task started");

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        dronecan_publish_node_status();
        g_uptime_sec++;

        ESP_LOGI(TAG, "NodeStatus published, uptime: %lu s", g_uptime_sec);

        // Presný 1 Hz timing
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));
    }
}

/**
 * Hlavná aplikačná task (príklad vlastnej logiky)
 */
static void app_task(void* arg) {
    ESP_LOGI(TAG, "Application task started");

    while (1) {
        // Tu môžete pridať vlastnú aplikačnú logiku
        // Napríklad čítanie senzorov, riadenie aktorov, atď.

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-C3 DroneCAN Node Starting...");
    ESP_LOGI(TAG, "========================================");

    // Inicializácia DroneCAN
    dronecan_init();

    // Vytvorenie DroneCAN spin tasku (najvyššia priorita)
    xTaskCreate(
        dronecan_spin_task,
        "dronecan_spin",
        4096,
        NULL,
        configMAX_PRIORITIES - 1,  // Najvyššia priorita
        NULL
    );

    // Vytvorenie NodeStatus tasku (stredná priorita)
    xTaskCreate(
        node_status_task,
        "node_status",
        2048,
        NULL,
        5,
        NULL
    );

    // Vytvorenie aplikačného tasku (nižšia priorita)
    xTaskCreate(
        app_task,
        "app_task",
        2048,
        NULL,
        3,
        NULL
    );

    ESP_LOGI(TAG, "All tasks created successfully");
}
