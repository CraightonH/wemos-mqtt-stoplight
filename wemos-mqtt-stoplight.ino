#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

bool CYCLE = false;
long startOfCycleTime = millis();
long periodicTimer = millis();
bool FLASH_RED = false;
bool DOOR_OPEN = false;

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
const char* logInfoTopic = "/log/info";
char message_buff[100];

WiFiClient wifiClient;
PubSubClient client("10.37.43.70", 1883, wifiClient);

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

void handleDistance(int distance) {
  if (DOOR_OPEN) {
    if (distance <= 10) {
      if (FLASH_RED) {
        FLASH_RED = false;
        redOn();
      } else {
        FLASH_RED = true;
        off();
      }
    }
    if (distance > 10 && distance <= 20) {
      redOn();
    }
    if (distance > 20 && distance <= 30) {
      yellowOn();  
    }
    if (distance > 30 && distance <=50) {
      greenOn();
    }
    if (distance > 50) {
      off();
    }
  }
}

void updateGarageDoor(String message) {
  if (message == "open") {
    DOOR_OPEN = true;  
  } else {
    DOOR_OPEN = false;  
  }
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
  client.publish(logInfoTopic, dbgMessage.c_str());
  if (strTopic == parkDistanceTopic) {
    handleDistance(message.toInt());
  } else if (strTopic == garageDoorTopic) {
    updateGarageDoor(message);
  } else if (strTopic == stoplightActionTopic) {
    if (message == "red") {
      redOn();  
    } else if (message == "yellow") {
      yellowOn();  
    } else if (message == "green") {
      greenOn();  
    } else if (message == "off") {
      off();
    } else if (message == "cycle") {
      cycle();
    }
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
      if (client.publish(logInfoTopic, message.c_str())) {
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
