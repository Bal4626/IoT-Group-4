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
MIIDWTCCAkGgAwIBAgIUc0QZYwTRgDa6Nu+IRyNhTaUZY+0wDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEyNTEzMzY1
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKL3S83+yWviSuPOKTou
3DiedDAPpFqNsDctewKsXFgl1Xi3v+2tPyMv17dWr9lry3Uqw1+FKKoLoxJrmzPx
w4aMg5SL6y2IHcsDwedFKvDBXiXYYPWv7XQgHWy8dL1rjpq2k4lU/VRZJ/Yvq56L
1mJ+SWHpj6vUC59dTH4gw+cYUGIKmD3iaqd13PABxCSPuc0iD2jLIiDj1M5Vu9YZ
Yu1V2Dij4CcJEkEfZEMJQ6QdzAgTKQmQmk4Uw7AO34NXpm8hiwJ0r71/G01Fx88o
D/aw/7yGKJw2liGpE0UWEnp1NK5nm7kBXAXaRImHaEdJ/cQnygrpKrklO0GycDI0
HjkCAwEAAaNgMF4wHwYDVR0jBBgwFoAU6ueMFasFhbmy/L4rW1FkEHGhHK8wHQYD
VR0OBBYEFMcureFi/7YkyZx0D93dFFtu4TljMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQB+cExJuWRed2qpCuCuLA+V5gVh
nId7F2YNnzFS2cO4vxnZfhBYfMpOgWggzJqj3iCrC71mxub0449iezAbQiSNFfnv
/FDMp1NGXUjQ2rCJo3D6shthmusV1L+68+/5DWvSQHSmWKHrCGK5Yj0sTk87W1Gv
mVKto/SNedCTP5LQysE79YLwVxxtMd50bJKiVTmSP37NSchn6+xsv5o0DWvcUB0O
r0fhPWZnDAiIAf1zPo1pOARSQcniWIYiqjGYTal5/GCtOniafMmKQ99Fo1cQ234X
1YvVTcaW4EkarGGpuSdrD9vQrq40pZCIR07Bb/7tj7dA6q/SVezbmVuskLfr
-----END CERTIFICATE-----
)EOF";

// Put your private key (.key) here
const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAovdLzf7Ja+JK484pOi7cOJ50MA+kWo2wNy17AqxcWCXVeLe/
7a0/Iy/Xt1av2WvLdSrDX4UoqgujEmubM/HDhoyDlIvrLYgdywPB50Uq8MFeJdhg
9a/tdCAdbLx0vWuOmraTiVT9VFkn9i+rnovWYn5JYemPq9QLn11MfiDD5xhQYgqY
PeJqp3Xc8AHEJI+5zSIPaMsiIOPUzlW71hli7VXYOKPgJwkSQR9kQwlDpB3MCBMp
CZCaThTDsA7fg1embyGLAnSvvX8bTUXHzygP9rD/vIYonDaWIakTRRYSenU0rmeb
uQFcBdpEiYdoR0n9xCfKCukquSU7QbJwMjQeOQIDAQABAoIBACGYyAj82Puj2Hqk
ev1iwcXubNenseN1SHnnp2+CpzsI+bx8bVYLnHOdnmROu8wgG/7YxRSBOEPuSD04
7/NCiWPIviu2uNJXOIUtDiGmsdaWltlIhZzBGzwQjuBm7c5LSUf+EeaDHgLwqntn
koMKIzWFAY3s/9CV05uUOXvbN2gB1+gViu6OOf+axIHEoxq6wBjKWp/yxHB++XHL
Q4REUEerAxZaL40nvDtpVmYKaDXXPkHioEj5+cu91XCkmkpuBH+JvPjxr5uN0ZPE
YHp91UGvftyY/PsvYyDeLsSBhQrvWMYJOpgJRa+Pr5wvJJPjcnTRcbKlHBWnYcJZ
jzNAPjECgYEA1j93jig8UXq22VXwp+UlrtIzWNNl5dVbawdqfZDP4Z9qZe4gt5to
ztn//+xYkWG+LPKXwemIhlFgf5brUFN8VXQNxfJNZ0ydgRWWDD9H8O8/jdZc7iyO
fE65hgDrdH+Ix6ifLVyTywyyS7r+X3kYe2L+8wM6EVTul3H/NIruPW0CgYEAwrlx
6zeZi2mhBJ1mHIGLOSn9VTTejJb0gou7fVY/lc5SUTOoDRCmsrd8uyYmfQMUh9Bd
Gj5ckcQiRZrfLdCjucTdJ0FUKMGSGJr47BuqFm+fD2h3TM/dfTLrAuX96NVOKagp
xYO/JAJbizd9hND/RdsqHl2iM3e9T6HvkgnsoH0CgYAfZzIY7iDZOIlwlb4ufdHm
K/1GucWnyHwb1fxv5zfxNDKeOiclgmv8VcvGMVhSBuefIvfHfnl6ZpckXHSlU3dX
9yjFqNxblclIUGer5+KdmBPSsGbyqilAYzGiSKQ6cS8VRMwAFB+9lWDU5NXAHLnP
7BS+feyW+cH3BMiJZQvCiQKBgQCD3AYLbSQZpta1hExwQzze4dM9zsZhV5uNh0fY
EP6P2B4gBKXo6vynCt9479WiU+KkMXHKYq5WuwUmM0CsCpvEIU5M1c1CqAnhhZfI
ctdTyBRi36/hoOTwcfl4hfvRnhgG12wp5xQZM0uAU3w44znpuQald2xuA8h7U0Ii
l3vYFQKBgBFeB5C0OLLLmWkltQ74eeaFrv1F1+b6n1sgAmuGMfaAz05NkhJ5/1fa
+F44YxHt+dOKgs9iZ4q5fZQ5F/2tByC6Fflvt5SyuMOwIGAu1cIXjy3gPkN3Yg01
zYTzSVjI4KkxTPOl1q+DOYLOCyKWV8/BYxgAOGOaOJaWXK7RwMHn
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
