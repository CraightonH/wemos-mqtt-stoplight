#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

bool CYCLE = true;
long startOfCycleTime = millis();

const int led = 13;
const int RED_LIGHT = D4;
const int YEL_LIGHT = D3;
const int GRE_LIGHT = D2;

void setLED(String light) {
  digitalWrite(RED_LIGHT, (light == "red"));
  digitalWrite(YEL_LIGHT, (light == "yellow"));
  digitalWrite(GRE_LIGHT, (light == "green"));
}

String getLEDStatus() {
  if (digitalRead(RED_LIGHT)) return "red";
  if (digitalRead(YEL_LIGHT)) return "yellow";
  if (digitalRead(GRE_LIGHT)) return "green";
  return "off";
}

void getStatus() {

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

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  
  findKnownWiFiNetworks();  

  pinMode(RED_LIGHT, OUTPUT);
  pinMode(YEL_LIGHT, OUTPUT);
  pinMode(GRE_LIGHT, OUTPUT);
  cycle();
}

void loop(void) {
  if (CYCLE) {
    if (millis() < startOfCycleTime + 4000) setLED("red");
    if (millis() > startOfCycleTime + 4000 && millis() < startOfCycleTime + 8000) setLED("green");
    if (millis() > startOfCycleTime + 8000 && millis() < startOfCycleTime + 12000) setLED("yellow");
    if (millis() > startOfCycleTime + 12000) cycle();
  }
}
