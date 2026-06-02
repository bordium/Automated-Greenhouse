#include "WifiManager.h"

// Translate an ESP32 WiFi disconnect reason code into a human label so the
// timeout message tells the user *why* (wrong password, no SSID, auth fail,
// etc.) instead of just timing out silently.
static const char* wifiReasonStr(uint8_t reason) {
    switch (reason) {
        case 1:   return "UNSPECIFIED";
        case 2:   return "AUTH_EXPIRE";
        case 3:   return "AUTH_LEAVE";
        case 4:   return "ASSOC_EXPIRE";
        case 6:   return "NOT_AUTHED";
        case 7:   return "NOT_ASSOCED";
        case 8:   return "ASSOC_LEAVE";
        case 15:  return "4WAY_HANDSHAKE_TIMEOUT (often: wrong password)";
        case 16:  return "GROUP_KEY_UPDATE_TIMEOUT";
        case 17:  return "IE_IN_4WAY_DIFFERS";
        case 23:  return "IEEE_802_1X_AUTH_FAILED (enterprise auth rejected)";
        case 24:  return "CIPHER_SUITE_REJECTED";
        case 200: return "BEACON_TIMEOUT";
        case 201: return "NO_AP_FOUND (SSID not in range)";
        case 202: return "AUTH_FAIL (wrong username/password)";
        case 203: return "ASSOC_FAIL";
        case 204: return "HANDSHAKE_TIMEOUT (auth too slow / cert mismatch)";
        case 205: return "CONNECTION_FAIL";
        default:  return "(see esp_wifi_types.h)";
    }
}

WifiManager::WifiManager() {
    Serial.println("[WiFi] Initialized");
}

bool WifiManager::scanAndPrint(const String& targetSsid) {
    Serial.println("[WiFi] Scanning 2.4 GHz APs in range...");
    WiFi.mode(WIFI_STA);
    int16_t n = WiFi.scanNetworks(false, true);   // sync, show hidden
    if (n <= 0) {
        Serial.println("[WiFi] No APs visible. Either the radio is broken or you're out of range of every 2.4 GHz network.");
        WiFi.scanDelete();
        return false;
    }
    bool foundTarget = false;
    Serial.printf("[WiFi] %d AP(s) found:\n", n);
    for (int i = 0; i < n; ++i) {
        String s = WiFi.SSID(i);
        int    rssi = WiFi.RSSI(i);
        int    ch   = WiFi.channel(i);
        wifi_auth_mode_t auth = WiFi.encryptionType(i);
        const char* authStr =
            auth == WIFI_AUTH_OPEN              ? "OPEN"
          : auth == WIFI_AUTH_WEP               ? "WEP"
          : auth == WIFI_AUTH_WPA_PSK           ? "WPA-PSK"
          : auth == WIFI_AUTH_WPA2_PSK          ? "WPA2-PSK"
          : auth == WIFI_AUTH_WPA_WPA2_PSK      ? "WPA/WPA2-PSK"
          : auth == WIFI_AUTH_WPA2_ENTERPRISE   ? "WPA2-ENTERPRISE"
          : auth == WIFI_AUTH_WPA3_PSK          ? "WPA3-PSK"
          : auth == WIFI_AUTH_WPA2_WPA3_PSK     ? "WPA2/WPA3-PSK"
          : "UNKNOWN";
        Serial.printf("  [%2d] %-32s  ch=%2d  rssi=%4d  %s\n",
                      i, s.c_str(), ch, rssi, authStr);
        if (targetSsid.length() > 0 && s == targetSsid) foundTarget = true;
    }
    if (targetSsid.length() > 0) {
        Serial.printf("[WiFi] target '%s' %s in scan.\n",
                      targetSsid.c_str(), foundTarget ? "FOUND" : "NOT FOUND");
        if (!foundTarget) {
            Serial.println("[WiFi] Most likely: the AP near you doesn't broadcast on 2.4 GHz,");
            Serial.println("       or the ESP32 antenna can't hear it from this location.");
            Serial.println("       Remember: ESP32 only sees 2.4 GHz; your phone may be on 5 GHz.");
        }
    }
    WiFi.scanDelete();
    return foundTarget;
}

void WifiManager::_installEventHandler() {
    if (_eventHandlerInstalled) return;
    WiFi.onEvent([](WiFiEvent_t /*event*/, WiFiEventInfo_t info) {
        uint8_t r = info.wifi_sta_disconnected.reason;
        Serial.printf("[WiFi] DISCONNECTED reason=%u (%s)\n", r, wifiReasonStr(r));
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    _eventHandlerInstalled = true;
}

bool WifiManager::connectToWiFi(String ssid, String password, uint32_t timeoutMs) {
    Serial.printf("[WiFi] Connecting to '%s' (timeout %u ms)\n", ssid.c_str(), timeoutMs);

    _installEventHandler();
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeoutMs) {
            Serial.printf("\n[WiFi] Timed out (status=%d). Continuing without WiFi.\n",
                          WiFi.status());
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
                                        String outerIdentity, uint32_t timeoutMs) {
    if (outerIdentity.length() == 0) outerIdentity = username;
    Serial.printf("[WiFi] WPA Enterprise: ssid='%s' identity='%s' username='%s' timeout=%u ms\n",
                  ssid.c_str(), outerIdentity.c_str(), username.c_str(), timeoutMs);

    _installEventHandler();

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    esp_wifi_sta_wpa2_ent_set_identity(
        (uint8_t *)outerIdentity.c_str(), outerIdentity.length());
    esp_wifi_sta_wpa2_ent_set_username(
        (uint8_t *)username.c_str(), username.length());
    esp_wifi_sta_wpa2_ent_set_password(
        (uint8_t *)password.c_str(), password.length());
    esp_wifi_sta_wpa2_ent_enable();

    WiFi.begin(ssid.c_str());

    uint32_t start    = millis();
    uint32_t lastTick = 0;
    while (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if (now - start >= timeoutMs) {
            Serial.printf("\n[WiFi] WPA Enterprise timed out after %u ms (status=%d).\n",
                          timeoutMs, WiFi.status());
            Serial.println("[WiFi] Check serial for DISCONNECTED reason codes above:");
            Serial.println("       23 -> enterprise auth rejected (bad creds or wrong EAP method)");
            Serial.println("       201 -> SSID not found / out of range");
            Serial.println("       202 -> wrong username/password");
            Serial.println("       204 -> RADIUS handshake timed out (slow server, cert issue)");
            return false;
        }
        if (now - lastTick >= 2000) {
            lastTick = now;
            Serial.printf("\n[WiFi] waiting... status=%d (%u ms elapsed)\n",
                          WiFi.status(), now - start);
        } else {
            Serial.print(".");
        }
        delay(250);
    }

    Serial.println();
    Serial.print("[WiFi] Connected to WPA Enterprise. IP: ");
    Serial.println(WiFi.localIP());

    ip_addr_t dnsserver;
    IP_ADDR4(&dnsserver, 8, 8, 8, 8);
    dns_setserver(0, &dnsserver);
    return true;
}
