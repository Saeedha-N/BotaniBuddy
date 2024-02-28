#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Oled.h"
#include "DHT.h"
// #include <PubSubClient.h>

// #include <HTTPClient.h>
// //#include <UrlEncode.h>

#define DHTPin 33
#define DHTTYPE DHT22

char out[128];

int ExTemp = 0;
int ExHumidity = 0;
int SoilMoisture=0;
int ldrValue = 0;

const int ResSenPin = 34;

//const int buzzerPin = 18;

const char* ssid = "NP";
const char* password = "nuths11011";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

const char* mqttTopic = "EnigmaSmartPot";
const char* subscribeTopic = "yourname/Enigmapot";

WiFiClient espClient;
PubSubClient client(espClient); 

DHT dht(DHTPin, DHTTYPE);



void setup() {

  dht.begin();

  Serial.begin(115200);
  WiFi.begin(ssid, password);  //initialize WiFi object

  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {  //make sure wifi connection is establish, if not until connection establishes loop executes
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.print(WiFi.localIP());

  /////////////////////////////////////////MQTT Connection//////////////////////////////////////////////////////////

  client.setServer(mqttServer, mqttPort);
  //client.setCallback(callback);

  while (!client.connected()) {  //make sure mqtt connection is establish, if not until connection establishes loop executes
    Serial.print("Connecting to MQTT..");
    if (client.connect("ESP32ClientEnigma")) {  //try to make unique since connecting to a global broker
      Serial.print("Connected to MQTT");
      client.subscribe(subscribeTopic);  //for subscription
    } else {
      Serial.print("MQTT Failed to connect");
      delay(5000);
    };
  }
  // put your setup code here, to run once:
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR);
}

void loop() {
 
  client.loop(); 
  // //////////////////////////////////////JSON Objects///////////////////////////////////////////////
  int h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  int t = dht.readTemperature();
  // Compute heat index in Celsius (isFahreheit = false)
  int hic = dht.computeHeatIndex(t, h, false);


  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

    // Read soil moisture in % from resistive soil moisture sensor
  int SoilMoisture = map(analogRead(ResSenPin), 0, 4095, 0, 100);
  SoilMoisture = 100 - SoilMoisture;


  StaticJsonDocument<256> doc;

  doc["EnvTemp"] = t;
  doc["EnvHum"] = h;
  doc["HeatIndex"] = hic;
  doc["SoilMoisture"] = SoilMoisture;

  serializeJson(doc, out);
  // convert float value to string, all data should be string in mqtt
  char valueStr[20];                                // Increase the size to accommodate the null terminator and decimal part
  snprintf(valueStr, sizeof(valueStr), "%.2f", t);  // %.2f limits the decimal places to 2
  snprintf(valueStr, sizeof(valueStr), "%.2f", h);
  snprintf(valueStr, sizeof(valueStr), "%.2f", hic);
  snprintf(valueStr, sizeof(valueStr), "%d", SoilMoisture);
  if (client.connected()) {
    client.publish(mqttTopic, out);  //publish the value(string) valueStr to the topic mqttTopic
    Serial.println("Published to MQTT: " + String(out));
    delay(500);
   
  }

  if ((SoilMoisture <= 5)) {
    Dyingsunny();  
  }

  else if ((5 < SoilMoisture) && (SoilMoisture <= 15)) {
    StillAlive();  
  }

  else if ((15 < SoilMoisture) && (SoilMoisture <= 20)) {
    NeedWater(); 
  }

  else if ((25 < SoilMoisture) && (SoilMoisture <= 35)) {  
    TummyFull();  
  }

  else if ((35 < SoilMoisture) && (SoilMoisture <= 55)) {
    Satisfied();  
  }

  else if ((55 < SoilMoisture)) {
    TooFullCool();  
  }

  else {  
    Error();
  }
}