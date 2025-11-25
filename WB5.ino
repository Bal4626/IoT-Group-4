#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>

// -------- Phone Hotspot WiFi (WPA2-Personal) --------
const char* WIFI_SSID     = "AndroidAP";   // <-- change to your hotspot SSID
const char* WIFI_PASSWORD = "hvuv5858";    // <-- change to your hotspot password

// -------- AWS IoT config --------
const char* MQTT_ENDPOINT = "a16ii7k74cp6ku-ats.iot.ap-southeast-1.amazonaws.com";
const int   MQTT_PORT     = 8883;
const char* MQTT_TOPIC    = "mall/anchor/scans";

const char* ANCHOR_ID       = "WB5";          // change per device: "WB1", "WB2", "WB3"
const char* MQTT_CLIENT_ID  = "ESP32_WB5";    // must be unique for each device


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
MIIDWTCCAkGgAwIBAgIUSVmudrYqv9m02hEcDhJ9ehR7rUwwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyNTE0Mjkz
N1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMnPpIsspJqtV5g4+yF4
IN3mZm+MMMgnNhrv2+BTtkNKeQNFoLu0V4fcYItXQd7kMkJZVBgm91yf2w3Mzfxc
MpdXzEf9U4WuQMmvdD9m0geLAF7jvaqu3paqvEcfXPIQz8tZy7LmRaBNuHw2dJCF
jyLOLhg44hAmlkOinm44oc+ec7slfu8AsCU/f3UYwwsvAYn2p3FeB/0JwAb63PA3
ZZAAD48T3pJyJFDIMw7nt0vltV5v51ObDY/GyQXD2/cZlS1WK4a/BFb8sUmmKNLN
Wm3gQfAQOz8jOWmQrfEevGQT9XILoupT0UWJBfIRLBC/zMKWZl2bCgz8EyqnYKZD
VPkCAwEAAaNgMF4wHwYDVR0jBBgwFoAUV/x4NKfkIbW6YsFbZGiiu8gMm30wHQYD
VR0OBBYEFOQmNGZj63t2HeuKD0FGhRI4HzslMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA3Ik9AqLvWKNWGDZD+P2A+RncO
VF+adFLDbkwBSU3mXFAaTnqxc5OlHHO3+5PGKYpk3SyV5mWfF72qmQdZtcnKSUBh
3gxY0Th1BmlON5nJalVSpFCkNv4Dg1rUlz2zrduKXPv5uBcMf+b84VZt+5GwWQRf
Ou90pFlnAi1sgQWybSdg+iSp1/ZbWCcsBqFkbSAULefiCcB1J7J6W5Mr2PErKGtb
9Z3emmDBMy/J4fiMXISxLWoMk+T8wh7SDeA1obilzYU4sVK7x7q5LIKeeORjk4cG
UqQX0W8LZzRHM9HJMHF7MGDujx9kordLKSB1famJTifmkqGVVQlnh7Wc8jxm
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAyc+kiyykmq1XmDj7IXgg3eZmb4wwyCc2Gu/b4FO2Q0p5A0Wg
u7RXh9xgi1dB3uQyQllUGCb3XJ/bDczN/Fwyl1fMR/1Tha5Aya90P2bSB4sAXuO9
qq7elqq8Rx9c8hDPy1nLsuZFoE24fDZ0kIWPIs4uGDjiECaWQ6Kebjihz55zuyV+
7wCwJT9/dRjDCy8BifancV4H/QnABvrc8DdlkAAPjxPeknIkUMgzDue3S+W1Xm/n
U5sNj8bJBcPb9xmVLVYrhr8EVvyxSaYo0s1abeBB8BA7PyM5aZCt8R68ZBP1cgui
6lPRRYkF8hEsEL/MwpZmXZsKDPwTKqdgpkNU+QIDAQABAoIBAGRm7qjjC/18MfEk
oCQ1nLxpLRtf9sENaQibvptramkt+uia0m2wOj/4bvvD7JyUkUCbKpBdioFFcnj6
JJzhzbWn05UZPl1qpQbySHZmlCV2jTeoPCtXlpwE02ja/KXdoVO9F++oSomHQTpi
6TkhiWW6iAo6+pUobWe21vVlLXOZsSUUw4fGHPwmqiGtpElTRB434C/pOXgvhazu
lqPB18SKXvK5QMlWaidqOM5vTK0dVA5JVXfFvE7DJDoDRDF2W9j/SKMYBMFUtIR+
gqhT4mru8M0Coy0q3aPNWtJYpCH/BZBchHmmr/RP8AvVViSoZujaIa3mxCVRkj5K
TXie/VECgYEA+nPxil+GO3aP/XKAJfp+d3l4+LBFoC3vXtvyxXeEMoLuzfCy75vg
cNQxNaxecM3sg++ZAifuY0EG2D0xAToXLKJukJsLAFilsKj0gs8v9KelavqVWD8u
Pb1eOBld7CT7sIhNJdlNDyu/PB7xvocJfdlvp9jil6hAX5yWlcMlG+UCgYEAzkfn
B2BdK99wSvHJqZi2pvZSXsmwhsOz4OrNxhBXVwYcHnlvfHmYJrereQasFJeZ0aOf
VOe/wnaDrhFOfUSF31V3SqCzMT9Ew7S3FqNbvKs/w8Mh5U1HXPbBaqQWF+MMvx6Z
onZPaWjYrhI9CUoQKjPl2NAjmL9nu4B2YLoVC4UCgYEA0dpOfBQYREDO252SNMBU
MUWOEEqavGFg3YnMPJstIKnG8LYLKGvivod9iCyaoUhaMh6ThVPyEiZNVujG4/FJ
s7OSkwXmTO76PjEDV4vE8el62E2Iz315AHBovva6VQCUB9HdWqZeRCTywqxnAK7j
TBjhzrETA+INtk9debUepeUCgYAZite1lx4SV6l87jGNWYaMaAXNDOZZ2ZPFbu2p
784QzYXf1COU4y2O5cFKzCO0rQzz43HQPp4FA03M2QHCCyEUhW2vBPULTQ1DunXN
TM8ILqBHqjrhPM9wWHGctdKyhH/Ijo5xinrMz5m8/C8onk4gG6sDTMqLmG/MzNxb
n88JTQKBgQCL1oHqSdwbLFsc69ue414I+FTByHU1VfTViURcKrbW9nEocDulVIRx
AEkLf2bbePEpINJUku0WoJiBYLfBcdP9FVM99xe92ealbOmo9pCJVS9eSbgKLw3/
jmnZq+Qk8rzqUT2d7T4XlBXdOqJnqwO+HhQBV25aL0CWqdbCSwhrIQ==
-----END RSA PRIVATE KEY-----
)EOF";


WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// -------- BLE scanner --------
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

void connectMQTT() {
  Serial.println("Configuring TLS credentials...");
  secureClient.setCACert(AWS_CERT_CA);
  secureClient.setCertificate(AWS_CERT_CRT);
  secureClient.setPrivateKey(AWS_CERT_PRIVATE);

  mqttClient.setServer(MQTT_ENDPOINT, MQTT_PORT);

  Serial.println("Connecting to AWS IoT MQTT...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID)) {   // <--- unique per device
      Serial.println("MQTT connected!");
    } else {
      Serial.print("MQTT connect failed, state=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}


// ===== BLE callback: collect WB* peers =====
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    String name = device.getName().c_str();
    if (name.startsWith("WB")) {  // wristbands only
      if (peerCount < 20) {
        peers[peerCount++] = { name, device.getRSSI() };
      }
    }
  }
};

// ===== Publish BLE peers to AWS IoT =====
void publishToBackend() {
  if (peerCount == 0) {
    Serial.println("No peers found this cycle.");
    return;
  }

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

  // BLE init
  BLEDevice::init("");
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  scanner->setActiveScan(true);

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
    Serial.println("Scan done, publishing...");
    publishToBackend();
    scanner->clearResults();
    lastScan = millis();
  }
}
