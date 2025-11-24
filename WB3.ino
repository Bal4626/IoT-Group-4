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

const char* ANCHOR_ID       = "WB3";          // change per device: "WB1", "WB2", "WB3"
const char* MQTT_CLIENT_ID  = "ESP32_WB3";    // must be unique for each device


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
MIIDWTCCAkGgAwIBAgIUQxl8UBVA3CI/IUNeDTKvWXmxgNEwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyMzA3MDgy
MloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALUEiUGcks84CR9JtSnS
vdbz1L7byJRBIFBZ9ExWLo6imGgfXMWPInRZBnEa7ZxbOSaQ3W7ZL1syzVreprEs
1BkjeUSCLa1oCyO/osZiwgzVzYurDCFUcwc2j+xOySPmQaumBvHqNWEyK6ngu+4J
5TTul5Hk9R6Haz6wqKIBOdi3C8v4PhvZPnYcnB/LNee7+Mq0SvZ8WQ22UegDdv2C
+wkIPXpWv9l7D4K1zpo4XrKUmMu30JMrK+ankwAs/UnRZhLfVLxvgEsRAF6BxWrL
ZxMaKfUdgamPU1OIEk7QCJY7aFFuM3KjcaxUocjh/n+Ci7F0/0hG0n0NmKIc+c27
YR0CAwEAAaNgMF4wHwYDVR0jBBgwFoAUV/x4NKfkIbW6YsFbZGiiu8gMm30wHQYD
VR0OBBYEFA4jnI85oAnrQ5C7JDY3uzskentoMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDHy4zVKYpm9WnXzCWR1btVVgRK
IJOGKDaOUip2rZgWW/zm43VmA58TJ3n5zzgQInvFAIAFVp88uhMIc/8cp8862aV1
c8l/CZu2UbH0WsrdqE1WGa/D3b605ci6GrPl9T6BbAJVIobwehapf/8QGKkfEVkv
XQj38thEa56GOjma+dwSgJ5tbmD653gcvCO3u4H0q/1FuFQTq4hep3Aaa1eGn8TU
OhN4AsoUMNfDgP3my7bhfyHl0RgBIEfnrQ9MOekQuen1x/iYHYYodMO5LlIrok+L
nyiJGkyeU3yB8BPrcMUCmldUfjbKgmEjuEO6p6l8d3XT1eb+MkqHGogApxzG
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAtQSJQZySzzgJH0m1KdK91vPUvtvIlEEgUFn0TFYujqKYaB9c
xY8idFkGcRrtnFs5JpDdbtkvWzLNWt6msSzUGSN5RIItrWgLI7+ixmLCDNXNi6sM
IVRzBzaP7E7JI+ZBq6YG8eo1YTIrqeC77gnlNO6XkeT1HodrPrCoogE52LcLy/g+
G9k+dhycH8s157v4yrRK9nxZDbZR6AN2/YL7CQg9ela/2XsPgrXOmjhespSYy7fQ
kysr5qeTACz9SdFmEt9UvG+ASxEAXoHFastnExop9R2BqY9TU4gSTtAIljtoUW4z
cqNxrFShyOH+f4KLsXT/SEbSfQ2Yohz5zbthHQIDAQABAoIBAFUTxrbxsgZ2ncL4
z47Sn6rxeuIhRkp3uYpKXQmrz8H2JY96fg3UUitA1EhAkcDVFy1LFOfOZ/WDEQVO
dCX9ncoa5BoQ3QsNRX8NyKFZNT73Fd45DEB2AoI3YLl95gdbZy4XoFf+7gu6ms8C
8xDT0chpAw/s/k/U8dDLEtbhoKB7zPRnN7zqGQxjVOZ8yuiteDGXqY7RoXzTPJSN
v6A7bltqGsnYlXcgw+tNg9uJ+ADHz2KdFzbAa6J+CDI1CINPvVlWKh4TFx/rDdeL
Ne/7lfnFIwACVvHgGi8dna6vCgZ7/WbhR4AiwP+TxTSiOwsQLRdh55A5qPzHlJ8d
vniZBzkCgYEA5LHAHufcYstsUsO4TDVZsG5BvfMRJUd+TFdrg/8ctr1wp4CiFxnT
QjrIldJ1zpq32zCimiI2xko4f3YbAYqaPHBThba5ThOVwEVzPOPXTGpmUTSXbXsu
tN18a2mzm0yuipMSp4j0McHaZn2VEJMpe3Wg7kw3wEGTnBstdzWHSf8CgYEAyqGB
tSbXQj0pQMA1ismzrMA/uhbKXMLWpzwqiix3u17R/rJAbNgjYUTKLB86XRK6P30o
pUNt4tX7qXhwvbntb/A1pKqtjykEwIRLipyVRjb66s+bDFmrd/yr0ViHwE2SnlKB
qIZQef3RxlYX0ua4LMchr0LTkeeP+5LmjdyTPOMCgYEAinOCGDPOnJEC12heHuT9
k25spo6NBsCtmZIjKRo9S/SMSxv5BXtBBouH/AwZ/iZ0wGJFToMAHjO42JGr79b1
YTmvlvf0X2xD8M8a4PYe1chkgS0Rr9ovgJfGR1EKxy45TPOVC5BCJg1yJzJIGGy/
ZFrmPt4tslsA/jUF5XGmogkCgYBF1SdYgQxKfb2L3TIWpLDRZmgP/+65zUQg2nFm
sTCKWIFSn6foWwzGZuiuEA0TKnMZkk7Btd1XAZv7qdBg8oTLwSMbnt61qdNDbdSC
7rfbPPN4nRe4R+b6MRAd077lHVbcTV7172QoxrrW+bC4NF9gvzfyqLs1W3KZpuTi
GxR7NQKBgEf1TqboffNNjMoPD2jMRYbuCuIQWvi4BW7+yxMGRHWH5bknhYiTRjN+
sMzoFU2gJ51pGO5NG7SKyLQDaSBuiugj+WTzcuKhPqyiHtV5ewMBojqgGaxQeR9N
r+O40pJPRw8m5U4IAnaT/yESmrCpQIq6liblelrSRp/ILOmwUS/9
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
