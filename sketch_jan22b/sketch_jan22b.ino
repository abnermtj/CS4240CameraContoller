#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <FS.h>

// ---------------------------- PINOUT -------------------------------
#define BUTTON_PIN D1
#define LED_RED_PIN D8
#define LED_GREEN_PIN D7
#define LED_BLUE_PIN D6
#define ROT_ENCODER_OUTPUT_A D5
#define ROT_ENCODER_OUTPUT_B D3
#define VIBRATION_PIN D2

// ---------------------------- ROT ENCONDER -------------------------------
int rotationA;
int prevRotationA;

// ---------------------------- BUTTON -------------------------------
int prevButtonState = 0;
int curButtonState = 0;

// ---------------------------- Wifi -------------------------------
#define SEND_DELAY_MS 1
const char* ssid = "Abner";         //put your wifi network name here
const char* password = "Abner123";  //put your wifi password here

WiFiServer server(80);
WiFiClient client;

// Get these values from the ipconfig
IPAddress ip(192, 168, 86, 26);
IPAddress gateway(192, 168, 86, 1);
IPAddress subnet(255, 255, 255, 0);

// WiFi Manager
WiFiClient wifiClient;
WiFiManager wifiManager;
bool configPortal = false;
char username[20] = "user1";
WiFiManagerParameter custom_username("username", "username", username, 20);
char broadcast_ssid[50] = "CS4240 yay";

const char* WIFI_AP_SSID = "CS4240 yay";
const char* WIFI_AP_PASSWORD = "CS4240-123";

int prevSendTime = 0;

struct packet {
  bool isButtonPressed;
  int rotEnconderStatus;
} sendPacket;

// --- Vibration
#define BUTTON_DOWN_VIBRATION_TIME_MS 120

int vibrationState;
int vibrationStartTime;

void loopVibration() {
  switch (vibrationState) {
    case 0:
      break;
    case 1:
      if (millis() - vibrationStartTime > BUTTON_DOWN_VIBRATION_TIME_MS) {
        vibrationState = 0;
        digitalWrite(VIBRATION_PIN, LOW);
      }
      break;
    default:
      break;
  }
}

// store username in file after configuration is updated
void saveConfigCallback() {
  Serial.println("WiFi config: username updated");

  // copy username into variable
  strcpy(username, custom_username.getValue());

  // save username to filesystem
  File usernameFile = SPIFFS.open("/username.txt", "w");
  int bytesWritten = usernameFile.print(username);
  if (bytesWritten > 0) {
    Serial.println("username saved successfully");
  } else {
    Serial.println("username save failed");
  }
  usernameFile.close();

  // if refreshed through config portal
  if (configPortal) {
    ESP.restart();
  }
}


void readUsername() {
  if (SPIFFS.exists("/username.txt")) {
    File usernameFile = SPIFFS.open("/username.txt", "r");
    if (usernameFile) {
      size_t filesize = usernameFile.size();
      usernameFile.readBytes(username, filesize);
      usernameFile.close();
    } else {
      Serial.println("error opening file for reading");
    }
  } else {
    Serial.println("username file does not exist, will be created after connecting to WiFi");
  }
}

void loopButton() {
  curButtonState = digitalRead(BUTTON_PIN);
  sendPacket.isButtonPressed = curButtonState;
  // Serial.print("Button: ");
  // Serial.println(curButtonState);

  if (curButtonState != prevButtonState) {
    if (curButtonState == HIGH) {
      Serial.println("Button: just released");
      digitalWrite(LED_BLUE_PIN, LOW);
    } else {
      Serial.println("Button: just pressed");
      digitalWrite(LED_BLUE_PIN, HIGH);
      analogWrite(VIBRATION_PIN, 240);
      vibrationStartTime = millis();
      vibrationState = 1;
    }
  }

  prevButtonState = curButtonState;
}

void loopRotEncoder() {
  rotationA = digitalRead(ROT_ENCODER_OUTPUT_A);
  if (rotationA != prevRotationA) {
    if (digitalRead(ROT_ENCODER_OUTPUT_B) != rotationA) {
      Serial.println("Rotation Encoder: Clockwise Rotation");
      sendPacket.rotEnconderStatus = 2;
    } else {
      Serial.println("Rotation Encoder: Anti-Clockwise Rotation");
      sendPacket.rotEnconderStatus = 1;
    }
    prevRotationA = rotationA;
  }
}

void loop() {
  loopButton();
  loopRotEncoder();
  loopVibration();
  wifiManager.process();

  client = server.accept();
  int curTime = millis();
  if (client && (curTime - prevSendTime >= SEND_DELAY_MS)) {
    prevSendTime = curTime;

    // Packet structure
    // Bit 2-3 Rot encoder status
    // Bit 1 Button status
    int packet = (sendPacket.rotEnconderStatus << 1) | (sendPacket.isButtonPressed);
    client.print(packet);

    sendPacket.rotEnconderStatus = 0;
  }
}

inline void initVibration() {
  pinMode(VIBRATION_PIN, OUTPUT);
}
inline void initRotEncoder() {
  pinMode(ROT_ENCODER_OUTPUT_A, INPUT);
  pinMode(ROT_ENCODER_OUTPUT_B, INPUT);

  prevRotationA = digitalRead(ROT_ENCODER_OUTPUT_A);
}

inline void initWifi() {
  // set up filesystem and read username
  SPIFFS.begin();
  readUsername();

  // WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  // Serial.print("Wifi: Connecting");

  // connect to WiFi using WiFiManager
  // stores credentials internally in the wemos, and if fails to connect, creates AP with captive portal
  wifiManager.setSaveConfigCallback(saveConfigCallback);  // only called when user updates WiFi credentials (if wifi credentials are saved, not called)
  wifiManager.addParameter(&custom_username);
  if (!wifiManager.autoConnect(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    ESP.restart();
  }
  delay(1000);

  strncat(broadcast_ssid, username, 32);

  WiFi.mode(WIFI_AP_STA);
  configPortal = true;
  wifiManager.setBreakAfterConfig(saveConfigCallback);
  wifiManager.setConfigPortalBlocking(false);                       // use this as softAP which we were using previously
  wifiManager.startConfigPortal(broadcast_ssid, WIFI_AP_PASSWORD);  // TODO: ESP crashes if changing SSID on this portal (will fix)
  Serial.printf("Broadcasting for wemos-presence on SSID: %s\n", broadcast_ssid);
  // Serial.print("Wifi: Connected, IP address: ");
    WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }


  Serial.print("Wifi: Connected, IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
}

inline void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Button: Setup done");
}

inline void initLEDs() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  digitalWrite(LED_GREEN_PIN, HIGH);
  Serial.println("LED: Setup done");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  initLEDs();
  initButton();
  initWifi();
  initVibration();

  digitalWrite(LED_GREEN_PIN, LOW);

  Serial.println("Setup done");
}