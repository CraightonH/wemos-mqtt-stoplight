#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

bool CYCLE = false;
long startOfCycleTime = millis();
long periodicTimer = millis();
int period = 300000;
bool FLASH = false;
bool DOOR_OPEN = false;

String devID = "esp8266-stoplight";
const int led = 13;
const int RED_LIGHT = D4;
const int YEL_LIGHT = D3;
const int GRE_LIGHT = D2;
const char* parkDistanceAllTopic = "/garage/park/distance/#";
const char* parkDistanceTopic = "/garage/park/distance";
const char* parkDistancePTopic = "/garage/park/distance/p";
const char* garageDoorAllTopic = "/garage/door/#";
const char* garageDoorTopic = "/garage/door";
const char* garageDoorPTopic = "/garage/door/p";
const char* stoplightStatusTopic = "/garage/stoplight/status";
const char* stoplightStatusPTopic = "/garage/stoplight/status/p";
const char* stoplightActionTopic = "/garage/stoplight/action";
const char* logInfoTopic = "/log/info";
char message_buff[100];

WiFiClient wifiClient;
PubSubClient client("192.168.1.239", 1883, wifiClient);

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
    FLASH = false;
    setLED("red");
}

void yellowOn() {
    CYCLE = false;
    FLASH = false;
    setLED("yellow");
}

void greenOn() {
    CYCLE = false;
    FLASH = false;
    setLED("green");
}

void off() {
    CYCLE = false;
    FLASH = false;
    setLED("off");
}

void startTimer() {
  startOfCycleTime = millis();
}

void cycle() {
  CYCLE = true;
  FLASH = false;
  startTimer();
}

void pubLightPeriodic() {
  //pubDebug("Run pubLightPeriodic()");
  if (millis() > periodicTimer + period) {
    //pubDebug("timer due");
    periodicTimer = millis();
    pubLight(stoplightStatusPTopic);
  }
}

void pubLight(const char* topic) {
  //pubDebug("Run pubLight()");
  client.publish(topic, getLEDStatus().c_str());
}

void handleDistance(int distance) {
  if (DOOR_OPEN) {
    if (distance <= 10) {
      if (!FLASH) {
        startTimer();
      }
      FLASH = true;
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
  } else {
    off();
  }
}

void updateGarageDoor(String message) {
  if (message == "open") {
    DOOR_OPEN = true;
  } else {
    DOOR_OPEN = false;
    off();
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
  pubDebug(dbgMessage);
  if (strTopic == parkDistanceTopic || strTopic == parkDistancePTopic) {
    handleDistance(message.toInt());
  } else if (strTopic == garageDoorTopic || strTopic == garageDoorPTopic) {
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
  } else {
    pubDebug(String(parkDistanceTopic));
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
    if (client.connect((char*) devID.c_str(), "mqtt", "mymqttpassword")) {
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
      client.subscribe(parkDistanceAllTopic);
      client.subscribe(garageDoorAllTopic);
    } else {
      Serial.println("MQTT connection failed");
      delay(5000);
    }
  }
}

void pubDebug(String message) {
  client.publish("/log/debug", message.c_str());
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
}

void loop(void) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  pubLightPeriodic();
  if (FLASH) {
    String prevColor = getLEDStatus();
    if ((millis() < startOfCycleTime + 1000) && prevColor == "red") {
      setLED("off");
    } else if ((millis() >= startOfCycleTime + 1000 && millis() < startOfCycleTime + 2000)) {
      setLED("red");
    } else if (millis() >= startOfCycleTime + 2000) {
      startTimer();
    }
  } else if (CYCLE) {
    if (millis() < startOfCycleTime + 4000) setLED("red");
    if (millis() > startOfCycleTime + 4000 && millis() < startOfCycleTime + 8000) setLED("green");
    if (millis() > startOfCycleTime + 8000 && millis() < startOfCycleTime + 12000) setLED("yellow");
    if (millis() > startOfCycleTime + 12000) cycle();
  }
}
