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
const char* ANCHOR_ID       = "WB1";        // e.g. "WB1", "WB2", "WB3"
const char* MQTT_CLIENT_ID  = "ESP32_WB1";  // must be unique per device

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
MIIDWTCCAkGgAwIBAgIUXi+UD1zbWanAN4q7RfK55p/QJ1kwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyNTEzMTQz
NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJUmTy6AgbTqPRyEon31
2zcoipDIC7lwNeKe2Z9cDVCwzcEDyQzTt25VNjxd65UrpRvtTYKHmiYz5MwzUvON
iVp8euUnVou/lCXuXJvE3Mw/OP7Nc7HR6sbttzCYWuKBGj3JszEZsJC+MzddgYQC
DwwWGR8ArOF1ErjLcdI1AqPSETfGBk7n6dCuTS/IVfDWfpVI8ZtKvE5VufJf3I7z
KfaR277bQa+YYV2vp3j80JtsNZ1Xx6ywbW2IatNNT3OmzKorce1RUfjQTKuUriv+
9gNVGoyhhxYOpFSOw71Khl6Rmv/z7ApufqRcZgYyfMkK27rvvIhbXqfp7H3evyzc
kycCAwEAAaNgMF4wHwYDVR0jBBgwFoAUWN90VoSZ580QsykSSxZ+0pao7l4wHQYD
VR0OBBYEFNHicFtVAG+iGpfsFx9J/Wdet7ZvMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDGRieKJoGC9XaaqoD5LKsPAl2R
y+YCkQffbIU99wD+TxZsViLBmzDMbw8nPwAWEbPIgkYn/ferxPzRFXx9I9fEfxz/
j8hIY52DpCj4t7b5lVH0yAT4I+tTfUMk3WPzAA3B/CwRyrZKLWb6nn7y0dzBE+Zq
EdGW1Dg/hzgz7wBIvBdKqHJ2I2W7KMj3fMeiAtN1Bf5ZbgrVd2qfsPciR9UHiMgf
3zip0gnevI/0Wz55cZbWDRrU+Q91bAxHYJBCjPFCxwV8o/4stsN72Du+f+vBAI6k
6aaJxcA/rBrCwJOXHfhFKZNm+RKmvq6brg+9yjEPvlb+CAFXAT0suetmTgkf
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAlSZPLoCBtOo9HISiffXbNyiKkMgLuXA14p7Zn1wNULDNwQPJ
DNO3blU2PF3rlSulG+1NgoeaJjPkzDNS842JWnx65SdWi7+UJe5cm8TczD84/s1z
sdHqxu23MJha4oEaPcmzMRmwkL4zN12BhAIPDBYZHwCs4XUSuMtx0jUCo9IRN8YG
Tufp0K5NL8hV8NZ+lUjxm0q8TlW58l/cjvMp9pHbvttBr5hhXa+nePzQm2w1nVfH
rLBtbYhq001Pc6bMqitx7VFR+NBMq5SuK/72A1UajKGHFg6kVI7DvUqGXpGa//Ps
Cm5+pFxmBjJ8yQrbuu+8iFtep+nsfd6/LNyTJwIDAQABAoIBAQCNqrKhH8yTs73S
HaVd8S2EScy70ldYjt2P4IQimsPG9fipMdhAh8R+Vw/J7Qe8S5VrXk42AmkHiljz
IHjU/4YMXZo5mNDasmCcWri5+BYlJpQTacBPkWzoAJj03nHNHQV41OGQbK9J/G8X
UoiXg4EZYNABo5mSGKpsvkjhehqE0Kruk51FJUORASNQi9+7ZQ3DsYawQPYUXOa5
VEF2dGcCBuwGN9+UcaOoBGUfMC8yLmqLgwMeW6p3zggezuvfQsuI4Sh3ZKEhh/ir
e0gDvRqO1tGVPYUZaX4ilSu/i0ZcwXURm1fmYBCCY2QmqnHEupn3lQjei+2oi97l
s9un9W4pAoGBAMYdvCRAKTrwioc0E6yJYcdohf45yrBaS/1Ev3+4FkDSiLmPpDP5
4KvgS4eLFWpc85moetQaDvYvmQ3plkcnlZ/Tind7yBM+Vys/JzG7qArCO5AijEIn
RK3LN6OKTN9WKxXSxCIrQekWrMoptlSEWfv1nL3LUyGuUaezdCguxhm7AoGBAMC6
FUiXgiw4Raz2xzQ0XOdFkjYyo7aSboFx2NMBYwv2uncKoF0Tcv0HR4m98DkoMfpV
vUUo0hjVRPk2wSq+KXf3mJRA4WtZOmsAuJlh9BX0bKpMaS39RENM5suPh9M+AxBZ
D7igApqmr7WdFYDE2hIHcwebUZBBbhfRZoI7uc+FAoGAWzcJjq8zzH0/aqyhekju
if8n4jr9hWCtp/hlKwq4xDOq27S053Fe7gOhAtEYlRYi2lZl5w3wYYzcrTklBbHG
BAZDtGt7XNDRFYpII2/BGOMAxYYCidrGljYAcf2VEyb7Q1r9DrZX7FeSSqoV1qfx
Pj6BYikhJ1q5IbtP+cg/UU0CgYA+gVt7wp+WRd3bHeI3fTAQVdD4wO21vcIXO1wt
+worjCaniwu9hBxMEftSogZyEyY6YWpULswM/8Yx6V82xJvga63Rj0VMspCuuXol
TM6D/FTt2oBWSQjFpwBHTtKnYTpPKANgRozxqVPdXAfoTm/HXyEge3A1j/LryJ8t
Z794nQKBgAOVq05A3KjyjM5g0UfHKPUfJnmdYT8h4Gz5yK7N9FlEZ7Ugl5s+kxev
p2fRgxDjhphX4R0o1To3/T2bjOsvC8MoQmUgT/oR3BYj33qor5guH/uKfz4A7154
mW6JRQbwMuITPDhw8Sp9G9dmUxjzVJKt5etvGjrfRo/hIv4+XCcT
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
