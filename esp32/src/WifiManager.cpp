#include "WifiManager.h"

WifiManager::WifiManager() {
    Serial.println("[WiFi] Initialized");
}

bool WifiManager::connectToWiFi(String ssid, String password, uint32_t timeoutMs) {
    Serial.printf("[WiFi] Connecting to '%s' (timeout %u ms)\n", ssid.c_str(), timeoutMs);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeoutMs) {
            Serial.println("\n[WiFi] Timed out — continuing without WiFi.");
            Serial.println("[WiFi] Radio will keep retrying in the background.");
            return false;
        }
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool WifiManager::connectToWPAEnterprise(String ssid, String username, String password,
                                        uint32_t timeoutMs) {
    Serial.println("[WiFi] Connecting to WPA Enterprise...");

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username.c_str(), username.length());
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username.c_str(), username.length());
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password.c_str(), password.length());
    esp_wifi_sta_wpa2_ent_enable();

    WiFi.begin(ssid.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeoutMs) {
            Serial.println("\n[WiFi] WPA Enterprise timed out.");
            return false;
        }
        delay(250);
        Serial.print(".");
    }

    Serial.println("\n[WiFi] Connected to WPA Enterprise.");

    ip_addr_t dnsserver;
    IP_ADDR4(&dnsserver, 8, 8, 8, 8);
    dns_setserver(0, &dnsserver);
    return true;
}
