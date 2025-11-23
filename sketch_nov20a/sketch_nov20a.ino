#include <WiFi.h>
#include <esp_eap_client.h>    // PEAP/WPA2-Enterprise API

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>

// ===== SUTD WPA2-Enterprise (PEAP) WiFi credentials =====
const char* ssid      = "SUTD_Wifi";
const char* identity  = "1006908";    // Outer identity
const char* username  = "1006908";    // Inner username
const char* password  = "*";          // Your SUTD WiFi password

// -------- AWS IoT config --------
const char* MQTT_ENDPOINT = "a16ii7k74cp6ku-ats.iot.ap-southeast-1.amazonaws.com";
const int   MQTT_PORT     = 8883;
const char* MQTT_TOPIC    = "mall/anchor/scans";


// Put your AmazonRootCA1.pem here
const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Put your device certificate (.crt) here
const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUSejmQPE1k1kgJtYy8RnRoXf/mF4wDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTExNjA3MjMz
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALGfihUotA0QcgX4rTgH
13d2ajpkhSehnhnFfZ3PaOdjFX+1T7HvBYYZAp8hSAjKKfdjO8FbrLMTUVGWnlgN
LFwQjjw/8bneXnDobeeLRCnwXJgPSI9Stbty6Amw5FX0khEVMbNvqkXAnA2VMiZs
qsiJL5GB7Njf/MzRpE9w9OFVgsRSIT0qSgVGUdQ+27+SCNbWMQNkPWYaHPq2dwF/
okQ2yyAPLkbtPEX09Ccw69jfVL0GWMwnGNXc4ZKExwXXEpzsWigzhDXj8Q/++2ks
cQOr0Lvm+4zyZuYx1dxb50iYkcV9GfimM+Hwtlp/XgNnS+DJiXKP4cRJl54kjEJ/
4LkCAwEAAaNgMF4wHwYDVR0jBBgwFoAUYZO8kQ8Qlfu+vuN9KtMfoRrJThwwHQYD
VR0OBBYEFAJbr18WVn3Y8K9X81ULb99ZpZmdMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDJ5l1p0ppCjL+oFSHXsepD6vgW
g/bDnn1XIiZUXhRWPmAK9ZnhZ/RWG/joD9D/nlBgd0M21VrzWaIhRNQ2NTE/vlOW
Q+fm0Gd57Jf+SVejXw/Bz6AgG7KeHIY66AAR+FZhHKz94yqSRFQ1E4pUi4mUhR/X
WLl9l5X84g7R1Q6t3s4etJPzB1QVNU4xv9k54vPXChq+iGBZkxOorSQY1N4En/sf
4YKmQnpCqH9iaa0o9en9OHyiMzSRQf3p/z5AnGuimk6TIJz55Bjjk4ycxi81C82/
hn3abug4Yy1brqm+gYihYxABG89X0MqZqPbLdnZSol8wOx4iVMKOGN6fHjXP
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAsZ+KFSi0DRByBfitOAfXd3ZqOmSFJ6GeGcV9nc9o52MVf7VP
se8FhhkCnyFICMop92M7wVussxNRUZaeWA0sXBCOPD/xud5ecOht54tEKfBcmA9I
j1K1u3LoCbDkVfSSERUxs2+qRcCcDZUyJmyqyIkvkYHs2N/8zNGkT3D04VWCxFIh
PSpKBUZR1D7bv5II1tYxA2Q9Zhoc+rZ3AX+iRDbLIA8uRu08RfT0JzDr2N9UvQZY
zCcY1dzhkoTHBdcSnOxaKDOENePxD/77aSxxA6vQu+b7jPJm5jHV3FvnSJiRxX0Z
+KYz4fC2Wn9eA2dL4MmJco/hxEmXniSMQn/guQIDAQABAoIBABKEPzDVfW+fN0PI
GMauipe5kHll4TuwbXriS6YX6Se1/JfDnvs9n6XLFarhHVFNQfK4NZKCrA4/BUss
GHW1adpvH2TIhbqKo+3BTTPGIOT+WajAbyABz1ArOmVIO4V6aX++Zd/+1BFRcvM3
inLXATbGll7k3Hv/Xas8b5SYge67ftg/RwULq1mIkaHqFv03dJMA81uhTdICXZOb
yq62kNkeZyixl8BuvlrNghsPdcxJfONHDi1DFHAKHWUaeYHO7z8anrNEri3LZ2N/
l3sZYczl+BZKfnWDt8QlWNn/COD9lb3NP4W9imlfprCRzAg9rpUTfcRKTp8ZJPW+
tMpKv8ECgYEA3ZVfWnJToI581BPX/VVVKKQfbB0gUQw/grp0wASH/fftn4DQ5m9F
R8TAecfUW4W7gB/YbPtDVeEaxLo4SI4/qMS5FvTqssrGYhSlN0cmvstoVrmMPVhh
D3IggdcwU7nbJYrnSoVJy+cKCWXMxrLav0C8nb6UIT4mUxuWSoE1YPcCgYEAzTY2
jRNH9WjZ3BWyE3/hFJCiJY1cJmGkD4kMa6cwA6KlfOR2BTfv8b+2F7puhH5VP3Op
+D6OkHqCDxXBdGoV4AaYEOs2xyI2i20ABkfCqjDHL86ysjbiYVnL5SGm+OCiMQxP
Kneq8GoJcjGIJtN7gbrzokTEfGyqnBVNtlOYD88CgYAkqtw9nl+iWRHlEmeSn3VZ
JVehz2wSnWFBI9PAFr/eUhG7bFilWVJwnulu/Zdxkb7GY/6vgiDRbE++sEYyE4AL
UIqEdwEDlzSe9GWmsRqDRiu89jVzzVU6dhtVASQZJs9R9htyJH5ixJfPmE37r4st
TOwpemkO2zcCV7VWHO2VXwKBgCKM0B5scp8U6ikt1e+GhkTKHSKpbCxiIBclgdvW
KDVUevxOErjKNDHFj0jrcJQKGRw3wxQypBobJi6YARwyiEBRm+StVWmtjvVmgqyM
BHBXChI4gMmUiATP844+XjhkyansYp/JyhP0Jmb9g+jVjmPHMCHP5iz0trzUsWLT
LDTtAoGBAJczkunlI5bIhA0yq3pEw51rn5S8yDiTLN+9yesB06PKi0lTwWFWzPhz
db1WOpY9XMCWXuS4kyKqL+w0BX8rV3KIdkpgO6I1ZVDGVUPaL9oWGq3oMTF+tqkJ
9c9u8prFcLoUkkaTnTzfFBascx97uUJn4B/dMbcMC/JMu8M+FQsf
-----END RSA PRIVATE KEY-----
)EOF";


WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

BLEScan* scanner;

unsigned long lastScan    = 0;
unsigned long SCAN_INTERVAL = 5000; // every 5 seconds

struct Peer {
  String id;
  int rssi;
};

Peer peers[20];
int peerCount = 0;

// ===== WiFi PEAP connect (your working logic) =====
void connectWiFi_PEAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  // Configure EAP/PEAP credentials
  esp_eap_client_set_identity((const unsigned char*)identity, strlen(identity));
  esp_eap_client_set_username((const unsigned char*)username, strlen(username));
  esp_eap_client_set_password((const unsigned char*)password, strlen(password));

  esp_err_t err = esp_wifi_sta_enterprise_enable();
  if (err != ESP_OK) {
    Serial.printf("esp_wifi_sta_enterprise_enable() failed: %d\n", err);
  }

  WiFi.begin(ssid);

  // Wait connecting
  Serial.print("Connecting to SUTD_Wifi (PEAP)");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
    // Stop here so you SEE the problem
    while (true) {
      delay(1000);
    }
  }
}

// ===== AWS IoT MQTT connect =====
void connectMQTT() {
  Serial.println("Configuring TLS credentials...");
  secureClient.setCACert(AWS_CERT_CA);
  secureClient.setCertificate(AWS_CERT_CRT);
  secureClient.setPrivateKey(AWS_CERT_PRIVATE);

  mqttClient.setServer(MQTT_ENDPOINT, MQTT_PORT);

  Serial.println("Connecting to AWS IoT MQTT...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ANCHOR1")) {
      Serial.println("MQTT connected!");
    } else {
      Serial.print("MQTT connection failed, state=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

// ===== BLE callback (scan for WB*) =====
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    String name = device.getName().c_str();

    if (name.startsWith("WB")) {  // wristbands only
      if (peerCount < 20) {
        peers[peerCount++] = {name, device.getRSSI()};
      }
    }
  }
};

// ===== Publish JSON to AWS IoT =====
void publishToBackend() {
  if (peerCount == 0) {
    Serial.println("No peers found this cycle.");
    return;
  }

  String json = "{";
  json += "\"anchor_id\":\"ANCHOR1\",";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"peers\":[";

  for (int i = 0; i < peerCount; i++) {
    json += "{";
    json += "\"id\":\"" + peers[i].id + "\",";
    json += "\"rssi\":" + String(peers[i].rssi);
    json += "}";
    if (i < peerCount - 1) json += ",";
  }

  json += "]}";

  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_TOPIC, json.c_str());
    Serial.println("Published: " + json);
  } else {
    Serial.println("MQTT not connected, cannot publish.");
  }
}

// ===== setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  // BLE init
  BLEDevice::init("");

  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  scanner->setActiveScan(true);

  // 1) Connect to SUTD_Wifi using PEAP
  connectWiFi_PEAP();

  // 2) Connect to AWS IoT
  connectMQTT();
}

// ===== loop =====
void loop() {
  mqttClient.loop();

  if (millis() - lastScan > SCAN_INTERVAL) {
    peerCount = 0;
    Serial.println("Starting BLE scan...");
    scanner->start(3);  // scan 3 seconds
    Serial.println("Scan done, publishing...");
    publishToBackend();
    scanner->clearResults();
    lastScan = millis();
  }
}
