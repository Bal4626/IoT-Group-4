#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// -------- Phone Hotspot WiFi (WPA2-Personal) --------
const char* WIFI_SSID     = "AndroidAP";   // <-- change to your hotspot SSID
const char* WIFI_PASSWORD = "hvuv5858";    // <-- change to your hotspot password

// -------- AWS IoT config --------
const char* MQTT_ENDPOINT = "a16ii7k74cp6ku-ats.iot.ap-southeast-1.amazonaws.com";
const int   MQTT_PORT     = 8883;
const char* MQTT_TOPIC    = "mall/anchor/scans";

// -------- Wristband identity (CHANGE PER DEVICE) --------
// For WB1 board:
const char* ANCHOR_ID       = "WB2";        // e.g. "WB1", "WB2", "WB3"
const char* MQTT_CLIENT_ID  = "ESP32_WB2";  // must be unique per device

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
MIIDWTCCAkGgAwIBAgIUfQdtZEgi+mpA2oc1iN6oGZkDSHcwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyMzA3MDQx
OFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANqOJwlkONH8nNAE50lp
JGEpY2XvivJLCHZ/XfrLV3KivJphXLUVz/RvFMKFcjBYX6swT3olih/eQ636Z0ZK
S2SM4ax0/lc397BMxuyX8lpsOFl/H8Y95CAThNDbYXpyKWgEU5Vipzi/qc0T5TEP
W9edFMCA5Voar8BWeFJ3HCNFzfwWJ2Ziq47KvrEEaOFx9oDPs5744kW8tElop4kA
hdcPDqch/dliVyE7P9Vu2gNITVs4VpnSBJexQhaDW4l6D5sbFj3Bs4im+B04kZvO
0UVOvD1RiaNkwnFIzb9+OkZ/TuaICT+UfrPUSwpF8VojhCmKe9yNl0GKI8b61QL4
zukCAwEAAaNgMF4wHwYDVR0jBBgwFoAUV/x4NKfkIbW6YsFbZGiiu8gMm30wHQYD
VR0OBBYEFCiVy7klHwBkBGEJmQulQYtdLANTMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCMRSGAxUApvQUmGpk+xZnZjmWC
HqIW4Nb8PBiQtcaNWX+HqNYffofhlSnRGx1MUrRCeYouOGZt3K3srNV3YlMgiHFV
yIDd+4ocwZp0kTZXci+mO2k6iaqdY7OVI3UCA+r+orLtigdJme/s7HL0pkmjwK/Q
USAA/RoIstgj1F/GrJTWXTPL6PkJD2zmgRhL5Q2RUnQ36Oy6vIz29NoEbwrCZE7q
Eke+MJYHULDCzh8pOX87jOLNj9pguZJD38COwRAZ6/mh0NZRTFXYkSv638mUsuCT
P8FevaK6eXzKHIEVpDgSghjqY0I0HRXjifQ3cG69LGtUX8oTVBTX/wtpe2QK
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEA2o4nCWQ40fyc0ATnSWkkYSljZe+K8ksIdn9d+stXcqK8mmFc
tRXP9G8UwoVyMFhfqzBPeiWKH95DrfpnRkpLZIzhrHT+Vzf3sEzG7JfyWmw4WX8f
xj3kIBOE0NthenIpaARTlWKnOL+pzRPlMQ9b150UwIDlWhqvwFZ4UnccI0XN/BYn
ZmKrjsq+sQRo4XH2gM+znvjiRby0SWiniQCF1w8OpyH92WJXITs/1W7aA0hNWzhW
mdIEl7FCFoNbiXoPmxsWPcGziKb4HTiRm87RRU68PVGJo2TCcUjNv346Rn9O5ogJ
P5R+s9RLCkXxWiOEKYp73I2XQYojxvrVAvjO6QIDAQABAoIBAEC+8hOXLDoRIweB
HF+kFVHvZ/Hv1/LVhalNLDM7ACoGyebWRxeiYZvhOH4z+FMYKydUxYbDDjtIpjYp
Q2UQZ2tfTQ/BDQQboXYKGLhIf1SIUN06itg4x5c1ERiqlSMoJNoaxgQwWKe4J/6I
EVLKYQPsFhS9WrSpyT0qoLSVcrGD1RbUAHYJy8Gd2I7n9vFCIBlC0Jot6ik0ZmuR
Ctqmm6z9NN3dpG2to+AtyC+cGy9rwpChane2CzNgPnwm3JwN7zxXO5AvZjGxHBwh
pYkpRWqixlA+bYvTxGfukF3yGlj5UUhqrl5uLG0Eu6Suu3zwP4Dnxhk39x2v8yMx
k6W6pd0CgYEA+ic19Wak5ot36bFvoUXdgkktYCfFSArkL+AGEQqIVN0FYg9Hyi4E
WUvXXVY9JfyuMQ3xvSkk1UCGgSVDeV3jQTXzSPauQjWRktqcgC0R+KfgC48Oqi5O
RLDQeWF1wPJJy9teIWmLL7dN0nmpvTkmeH3XzbYvekVINox4FmLswgsCgYEA36ng
RUajJoCh4LykzWSv4B+0BeTzzj7tCgJpenp+mGJWjHIOM8iqMo1jPz1hbMbPdcL1
TvB6g3Wo3QXNK9ObZ3ouMrxnEw8UvScbhnihJgriWwoaV8X6VjLh8WONPJg9BjtA
mdq/6z9VxHuI1OSdXU0dCTJ1Q0tbiRuYFMbBn1sCgYEAsG+Xj06mYDVejmWn5ofw
hEQ4zOlALGACMgJARSS4o6KAeXltmRx71L+5VwvMdU6TTeYX6uNqJz8rn0s5HBEs
iIIU5hGXNvuxgXLPj1g6pLdDPuPX5BBlA+3yHKqw8zXw9ufyrH5UvYL8UMefAOee
Sr33yBAHzKWxGUhaKu/7LaECgYACH2yfe7IktWfT6DZwNJpuuB5PSAMAXByH0coK
fYsiCxzjLFnyJ99lp23ftcds7t4dMgJ+A05GXSGDJ4Dw/4skCkbR+HPJ/ISn3u0l
85Mj90p6rrfdSWW817LJRbabcp5xprJQQ4E6zFkWCk4cJydPMWIrKi1uvBJcVMS2
vcQsrQKBgEvSBnCQ1oSZ7NdiWjfLn20oAnVcOBfGs0QLEW/RU4c4Cr86tdjgWGwk
1Iaabn3VfxxotKV79xNTFwNSsQJpJqilCUzhMhb/S6J1nRaR9WQmUuWN30qbQYaG
DPu90xhP5xhjluinXSIu5oQ+ofZtQa5xigx00B3/g6KvPM6dnpMO
-----END RSA PRIVATE KEY-----
)EOF";

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// -------- BLE scanner + advertising --------
BLEScan* scanner;

unsigned long lastScan      = 0;
unsigned long SCAN_INTERVAL = 5000; // ms

struct Peer {
  String id;
  int rssi;
};

Peer peers[20];
int peerCount = 0;

// ===== WiFi connect (HOTSPOT) =====
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 40) { // ~20s
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  int st = WiFi.status();
  Serial.print("WiFi status after connect: ");
  Serial.println(st);

  if (st == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi FAILED to connect.");
    while (true) { delay(1000); }
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
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT connected!");
    } else {
      Serial.print("MQTT connect failed, state=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

// ===== BLE callback: collect WB* peers (except self) =====
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) override {
    std::string devNameStd = device.getName();
    if (devNameStd.empty()) return;

    String name = String(devNameStd.c_str());

    // Only consider WB* devices
    if (!name.startsWith("WB")) return;

    // Ignore self
    if (name == ANCHOR_ID) return;

    if (peerCount < 20) {
      peers[peerCount++] = { name, device.getRSSI() };
    }
  }
};

// ===== Publish BLE peers to AWS IoT =====
void publishToBackend() {
  String json = "{";
  json += "\"anchor_id\":\"" + String(ANCHOR_ID) + "\",";
  json += "\"timestamp\":" + String(millis()) + ",";
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

  // --- BLE init: advertise THIS wristband as ANCHOR_ID (e.g. "WB2") ---
  BLEDevice::init(ANCHOR_ID);   // sets the advertised name

  BLEServer* pServer = BLEDevice::createServer(); // not used further, but needed by some stacks
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  // Optional UUID just so it's not empty
  BLEUUID serviceUUID("12345678-1234-5678-1234-56789abcdef0");
  pAdvertising->addServiceUUID(serviceUUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  Serial.print("Started advertising as ");
  Serial.println(ANCHOR_ID);

  // --- BLE scanner init ---
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  scanner->setActiveScan(true);  // active scan to grab names quickly

  // 1) Connect to hotspot WiFi
  connectWiFi();

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
    Serial.print("Scan done, found peers: ");
    Serial.println(peerCount);
    publishToBackend();
    scanner->clearResults();
    lastScan = millis();
  }
}
