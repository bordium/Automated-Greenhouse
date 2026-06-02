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

    /**
     * @param outerIdentity Optional anonymous identity (e.g. "anonymous").
     *        If empty, falls back to `username`. Some enterprise networks
     *        (PEAP) want a blank/anonymous outer identity while the real
     *        username travels inside the TLS tunnel.
     */
    bool connectToWPAEnterprise(String ssid, String username, String password,
                                String outerIdentity = "",
                                uint32_t timeoutMs = 40000);

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

    /**
     * Scan visible 2.4 GHz APs and print them over Serial.
     * If `targetSsid` is non-empty, also returns whether that SSID was found
     * (useful to disambiguate "out of range" from "auth failure").
     */
    bool scanAndPrint(const String& targetSsid = "");

private:
    void _installEventHandler();
    bool _eventHandlerInstalled = false;
};

#endif
