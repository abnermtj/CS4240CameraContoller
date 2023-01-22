#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqtt_Generic.h>  // Arduino library Marvin ROGER 1.8.0

// ---------------------------- PINOUT -------------------------------
#define BUTTON_PIN D1
#define LED_RED_PIN D8
#define LED_GREEN_PIN D7
#define LED_BLUE_PIN D6
#define ROT_ENCODER_OUTPUT_A D5
#define ROT_ENCODER_OUTPUT_B D3

// ---------------------------- ROT ENCONDER -------------------------------
int rotationA;
int prevRotationA;

// ---------------------------- BUTTON -------------------------------
int prevButtonState = 0;
int curButtonState = 0;

// ---------------------------- MQTT -------------------------------
const char* MQTT_HOST = "52.221.121.128";
const int MQTT_PORT = 1883;

const char* GeneralTopic = "general";
const char* ButtonTopic = "button";
const char* EncoderTopic = "encoder";

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

////////////////////////////
// Helper Functions
////////////////////////////

String float_to_string(float input) {
  char payload[10];
  sprintf(payload, "%.2f", input);
  return payload;
}

void onMqttConnect(bool sessionPresent) {
  Serial.print("Connected to MQTT broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(", port: ");
  Serial.println(MQTT_PORT);

  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  MqttPublish("Connecting on WeMOS", GeneralTopic);
  Serial.println("Test Publishing at QoS 0");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  (void)reason;

  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void MqttPublish(char* MQTT_message, const char* MQTT_Topic) {
  char pubTopic[50];
  sprintf(pubTopic, "%s/%s/", GeneralTopic, MQTT_Topic);
  mqttClient.publish(pubTopic, 0, true, MQTT_message);
}

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total) {
  (void)payload;

  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}


void loopButton() {
  curButtonState = digitalRead(BUTTON_PIN);
  // Serial.println(curButtonState);
  if (curButtonState != prevButtonState) {
    if (curButtonState == HIGH) {
      Serial.println("Button: just released");
      MqttPublish("Button: just released", ButtonTopic);
      digitalWrite(LED_BLUE_PIN, LOW);
    } else {
      Serial.println("Button: just pressed");
      MqttPublish("Button: just pressed", ButtonTopic);
      digitalWrite(LED_BLUE_PIN, HIGH);
    }
  }

  prevButtonState = curButtonState;

  char buttonData[100];
  sprintf(buttonData, "%d", curButtonState);
  MqttPublish(buttonData, ButtonTopic);
}

void loopRotEncoder() {
  rotationA = digitalRead(ROT_ENCODER_OUTPUT_A);
  if (rotationA != prevRotationA) {
    //  The encoder is rotating clockwise
    if (digitalRead(ROT_ENCODER_OUTPUT_B) != rotationA) {
      Serial.println("Rotation Encoder: Clockwise Rotation");
    } else {

      Serial.println("Rotation Encoder: Anti-Clockwise Rotation");
    }
  }
  prevRotationA = rotationA;
}

void loop() {
  loopButton();
  loopRotEncoder();
}


inline void initRotEncoder() {
  pinMode(ROT_ENCODER_OUTPUT_A, INPUT);
  pinMode(ROT_ENCODER_OUTPUT_B, INPUT);

  prevRotationA = digitalRead(ROT_ENCODER_OUTPUT_A);
}

void connectToMqtt() 
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void initMqtt() {
  connectToMqtt();

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  Serial.println("MQTT: Setup done");
}

inline void initWifi() {
  WiFi.begin("tolentino", "Abner123");
  Serial.print("Wifi: Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Wifi: Connected, IP address: ");
  Serial.println(WiFi.localIP());
}


inline void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Button: Setup done");
}

inline void initLEDs() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  digitalWrite(LED_GREEN_PIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                        // wait for a second
  digitalWrite(LED_GREEN_PIN, LOW);   // turn the LED off by making the voltage LOW
  Serial.println("LED: Setup done");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  initLEDs();
  initButton();
  initWifi();
  initMqtt();

  Serial.println("Setup done");
}