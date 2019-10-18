#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

bool CYCLE = false;
long startOfCycleTime = millis();
long periodicTimer = millis();

String devID = "esp8266-stoplight";//getMAC();
const int led = 13;
const int RED_LIGHT = D4;
const int YEL_LIGHT = D3;
const int GRE_LIGHT = D2;
const char* parkDistanceTopic = "/garage/park/distance/#";
const char* garageDoorTopic = "/garage/door/#";
const char* stoplightStatusTopic = "/garage/stoplight/status";
const char* stoplightStatusPTopic = "/garage/stoplight/status/p";
const char* stoplightActionTopic = "/garage/stoplight/action";
const char* debugTopic = "/device/debug";
char message_buff[100];

WiFiClient wifiClient;
PubSubClient client("192.168.1.61", 1883, wifiClient);

void setLED(String light) {
  String prevState = getLEDStatus();
  digitalWrite(RED_LIGHT, (light == "red"));
  digitalWrite(YEL_LIGHT, (light == "yellow"));
  digitalWrite(GRE_LIGHT, (light == "green"));
  String currState = getLEDStatus();
  if (prevState != currState) {
    pubLight(stoplightStatusTopic);
  }
}

String getLEDStatus() {
  if (digitalRead(RED_LIGHT)) return "red";
  if (digitalRead(YEL_LIGHT)) return "yellow";
  if (digitalRead(GRE_LIGHT)) return "green";
  return "off";
}

void redOn() {
    CYCLE = false;
    setLED("red");
}

void yellowOn() {
    CYCLE = false;
    setLED("yellow");
}

void greenOn() {
    CYCLE = false;
    setLED("green");
}

void off() {
    CYCLE = false;
    setLED("off");
}

void cycle() {
    CYCLE = true;
    startOfCycleTime = millis();
}

void pubLightPeriodic() {
    if (millis() > periodicTimer + 20000) {
      periodicTimer = millis();
      pubLight(stoplightStatusPTopic);
    }
}

void pubLight(const char* topic) {
    client.publish(topic, getLEDStatus().c_str());
}

String getMAC() {
  String result;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void handleDistance() {
  
}

void handleGarageDoor() {
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  for(i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String message = String(message_buff);
  String strTopic = String(topic);
  String dbgMessage = String("(");
  dbgMessage += devID;
  dbgMessage += ") ";
  dbgMessage += strTopic;
  dbgMessage += ": ";
  dbgMessage += message;
  client.publish(debugTopic, dbgMessage.c_str());
  if (strTopic == parkDistanceTopic) {
    handleDistance();
  } else if (strTopic == garageDoorTopic) {
    handleGarageDoor();
  } else if (strTopic == stoplightActionTopic) {
    setLED(message);
  }
}

void findKnownWiFiNetworks() {
  ESP8266WiFiMulti wifiMulti;
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("BYU-WiFi", "");
  wifiMulti.addAP("Hancock2.4G", "Arohanui");
  Serial.println("");
  Serial.print("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    wifiMulti.run();
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID().c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while(!client.connected()) {
    if (client.connect((char*) devID.c_str())) {
      Serial.println("Connected to MQTT server");
      String message = devID;
      message += ": ";
      message += WiFi.localIP().toString();
      if (client.publish(debugTopic, message.c_str())) {
        Serial.println("published successfully");
      } else {
        Serial.println("failed to publish");
      }
      client.subscribe(stoplightActionTopic);
      client.subscribe(parkDistanceTopic);
      client.subscribe(garageDoorTopic);
    } else {
      Serial.println("MQTT connection failed");
      delay(5000);
    }
  }
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  
  findKnownWiFiNetworks();  

  pinMode(RED_LIGHT, OUTPUT);
  pinMode(YEL_LIGHT, OUTPUT);
  pinMode(GRE_LIGHT, OUTPUT);
  client.setCallback(callback);
  
  //cycle();
}

void loop(void) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  pubLightPeriodic();
  if (CYCLE) {
    if (millis() < startOfCycleTime + 4000) setLED("red");
    if (millis() > startOfCycleTime + 4000 && millis() < startOfCycleTime + 8000) setLED("green");
    if (millis() > startOfCycleTime + 8000 && millis() < startOfCycleTime + 12000) setLED("yellow");
    if (millis() > startOfCycleTime + 12000) cycle();
  }
}
