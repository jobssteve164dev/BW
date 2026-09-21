#include "Esp32WirelessRadioControl.h"

#include <esp_bt.h>
#include <esp_wifi.h>

namespace services {

bool Esp32WifiRadio::disable() {
    const auto stopResult = esp_wifi_stop();
    if (stopResult != ESP_OK &&
        stopResult != ESP_ERR_WIFI_NOT_INIT &&
        stopResult != ESP_ERR_WIFI_NOT_STARTED) {
        return false;
    }

    const auto deinitResult = esp_wifi_deinit();
    if (deinitResult != ESP_OK && deinitResult != ESP_ERR_WIFI_NOT_INIT) {
        return false;
    }

    return state() == RadioState::ISOLATED;
}

RadioState Esp32WifiRadio::state() const {
    wifi_mode_t mode = WIFI_MODE_NULL;
    const auto result = esp_wifi_get_mode(&mode);
    if (result == ESP_ERR_WIFI_NOT_INIT) {
        return RadioState::ISOLATED;
    }
    if (result != ESP_OK) {
        return RadioState::INDETERMINATE;
    }
    return mode == WIFI_MODE_NULL ? RadioState::ISOLATED : RadioState::ACTIVE;
}

bool Esp32BluetoothRadio::disable() {
    if (memoryReleased) {
        return esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE;
    }

    auto status = esp_bt_controller_get_status();
    if (status == ESP_BT_CONTROLLER_STATUS_ENABLED &&
        esp_bt_controller_disable() != ESP_OK) {
        return false;
    }

    status = esp_bt_controller_get_status();
    if (status == ESP_BT_CONTROLLER_STATUS_INITED &&
        esp_bt_controller_deinit() != ESP_OK) {
        return false;
    }

    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
        return false;
    }

    memoryReleased = esp_bt_mem_release(ESP_BT_MODE_BLE) == ESP_OK;
    return state() == RadioState::ISOLATED;
}

RadioState Esp32BluetoothRadio::state() const {
    const auto status = esp_bt_controller_get_status();
    if (memoryReleased && status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        return RadioState::ISOLATED;
    }
    if (status == ESP_BT_CONTROLLER_STATUS_IDLE ||
        status == ESP_BT_CONTROLLER_STATUS_INITED ||
        status == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        return RadioState::ACTIVE;
    }
    return RadioState::INDETERMINATE;
}

} // namespace services
