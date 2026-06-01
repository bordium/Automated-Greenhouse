#ifndef GREENHOUSE_WIFI_MANAGER_H
#define GREENHOUSE_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wpa2.h"
#include "esp_wifi.h"
#include <lwip/dns.h>

/**
 * @brief Non-blocking wrapper around Arduino WiFi.
 *
 * `connectToWiFi()` returns within `timeoutMs` whether or not the connection
 * succeeded — the main loop runs either way. The radio's auto-reconnect is
 * enabled, so the radio keeps trying on its own in the background.
 *
 * Named WifiManager (not WIFI) to avoid colliding with the Arduino WiFi.h
 * header on case-insensitive filesystems (Windows / macOS default).
 */
class WifiManager {
public:
    WifiManager();

    /** @return true if connected within timeoutMs, false on timeout. */
    bool connectToWiFi(String ssid, String password, uint32_t timeoutMs = 15000);
    bool connectToWPAEnterprise(String ssid, String username, String password,
                                uint32_t timeoutMs = 20000);

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
};

#endif
