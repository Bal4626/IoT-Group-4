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

const char* ANCHOR_ID       = "WB4";          // change per device: "WB1", "WB2", "WB3"
const char* MQTT_CLIENT_ID  = "ESP32_WB4";    // must be unique for each device


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
MIIDWTCCAkGgAwIBAgIUQSR7mVRfJcbW+Diak1E0SgsCwrAwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyNTE0MTcy
MloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAN4U6JrZsv0eJwXIrI06
pbKUkjBPWJmS9xw+x/I1lhBQYVig7YKVpU4y4C8So4/j4255VJCvXFc9ikal57MY
Z4/QpkjSnaHroBInXqgaQNqRFFiEKr5BuP7xJi5Rdj48lvcEA8NwM5pt2c86hf68
jKe5dJtPeM7JS9E0jhkb7ZDRCIOdGVVc4sC6bIHQWXqEN7EHpBWp6G6jFJmYymIM
N8DwnO1X74l5MpI+GowWdE476rcIUjOw+sq+Twouj1PWFYJmCj730uzEziVZnhvp
XEcr+Kd94G3gRuDze+g3uDQA/o5e9bqZEA1+xzMIfqZQuVmRh1niYy5DJkn0Blql
NcECAwEAAaNgMF4wHwYDVR0jBBgwFoAUWN90VoSZ580QsykSSxZ+0pao7l4wHQYD
VR0OBBYEFFlWNLSgsIxMB0qpI8SG09N4m0CyMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCSGbIk/25NJC3gyPgATzdVhSLG
EtLOSM2336ify7M/4sP0ia/wmiFd0qRzOf+2zipGPM97Nf8rEfaNsi2keqS7+mho
XENsklyVoeQ2xy/BAfLJ8rvBoXiyYxAP93KPiSILvil78DR2NTn6awiz8CcjTAw2
gM8HkQ0HDZKa3mlWK9ZtvE6x/c1pRpeSBolaHadZi+vpeo4GuGRnQGHEcbTkD3aB
fNYg4SJ73DXb2vtnEfD4cfkKz43L3VQotTL1r6TXAkaVtDNCeO1710qF8nZxoQeD
SKlgTO3Bvh65hYNdzl/Xx460CwWDS99CBgLIyfirOVkg94JRbTFGwWKX2HQE
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA3hTomtmy/R4nBcisjTqlspSSME9YmZL3HD7H8jWWEFBhWKDt
gpWlTjLgLxKjj+PjbnlUkK9cVz2KRqXnsxhnj9CmSNKdoeugEideqBpA2pEUWIQq
vkG4/vEmLlF2PjyW9wQDw3Azmm3ZzzqF/ryMp7l0m094zslL0TSOGRvtkNEIg50Z
VVziwLpsgdBZeoQ3sQekFanobqMUmZjKYgw3wPCc7VfviXkykj4ajBZ0TjvqtwhS
M7D6yr5PCi6PU9YVgmYKPvfS7MTOJVmeG+lcRyv4p33gbeBG4PN76De4NAD+jl71
upkQDX7HMwh+plC5WZGHWeJjLkMmSfQGWqU1wQIDAQABAoIBAG6aHSoZp7G11Ztk
0MLZZXJl2dSjf62katsvTScMlvry/ThFlWQGoTgr0EjBUKI1zavUU1BndWahaH88
2wLv8YvHTzlPY2Aj2CmSX+r6gDgaLvbSg/MTCk41zV0eBvSB+nF1F2a7N4f+k45I
SDmQr8BMdY1v6+uYhuDe+rH4nb0T4Fiq2DkqfCXydaG1f4PyN1s5MJlZv4xd3h1W
QcYSptYVPw8iNVPvv9bxn+C9WRn1y/8g8di8TO4nEbYR2CnAqH4jI14h+HyDhUV0
bhWjqI74BMyVhzEXQog1PXjNRV6ueswEicws5STu1wKlH7a+sT3bZyLSjNHUvHtJ
gNFuYAECgYEA7qWu8LZQerdv4DnzquzUPTktMIYzE5U2D+RZ7nPSdK8jm+1vAGa4
xFgjzuOXqiUemXkD75yXP4EoHGv3Otlsvyc66a8/ReNPltvXdPDmym5H2yh43u1z
DFmcuOMHYD7OUYpIbAVCkZherSMait1uOiEQDFlJ2FWxA0BmoZVwyIECgYEA7jrd
b/zzoG9EGqD8PelG0eISaE8LNpez8zs5I/Ik3C+k6lBJZ2+z0/lCBAuYNhlHj+dR
H/qbuPZWTH4DxAuoExfJ9Z/oL86H0HXthkq5C1oXK2nXAZdHWx0+SwAlxvcq9kjv
QHwdcFE38jTK1azNYfftG3vhawG4fxJCMsbzzUECgYEAgmUzNkFXrgXG8kyX8F/h
A+bW4zmgdOXFmV9bKChO6QS9Pzi3uw9li5rhTe0bdJM9qfvug6EbH07R/N2dQ6GD
gtrLnN3tg6FVXoeWrUXnxNoHFx+BE8XIZAoe0QLzh8opb3JQaf1hxy3rJU0V9qRQ
0yA3pZev+q2vv6gTq9x8uIECgYEAhNiql0KVkQe7EKZpQxUM+wl4flZlyGG8ZYzH
BnaHe4nOCyQOavHtc+1xlNPqY3ChaWvy30RfBjvIUAuNigt+BHellaVAUzh2oBbD
B7ovilcIyJrS3P5BOlwKWsSQTG2UtZZiZLFRsUp5SkWKPrGILnOU81iXkZZ6DdJe
u/YaCoECgYAkq9ndXy/tIuQJvAesvIJQlPMbgXkxfhucMLhCociUuFgwxuLoI4Wx
Ja8Bl/wksxOChvP8ikNEee2AvkhVWG+go8bXy3oSntQVXfK1o8aMSNQ4M4ssOthh
xLsW/4qGf7heIVWerGOQWw8F7PxwLnWHQN8cGuO/ZrFQ6nTSFpSxhA==
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
