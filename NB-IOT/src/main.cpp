/* 
Created by Ahmad Radhy
email: ahmadradhyfisika@gmail.com
*/

#define TINY_GSM_MODEM_SIM7000

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

void reconnect();

struct senseData
{
  float temperature = 0.0F;
  float humidity = 0.0F;
};

static char dataEnv[256];
static senseData data;
StaticJsonDocument <256> doc;

#define DHTPIN 8
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


#define SerialAT Serial1
#define GSM_PIN ""

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

#define TOKEN ""
#define CLIENTID ""
const char broker[] = "";
const char publishTopic[] = "";
long lasdData = 0;


void setup() {

  Serial.begin(9600);
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Wait...");
  SerialAT.begin(9600);
  delay(2000);
  modem.setNetworkMode(38); //38-LTE, 13-GSM
  modem.setPreferredMode(2); //2-NB-IoT, 1-CAT, 3-GPRS

  Serial.println("Initializing modem......");
  modem.restart();
  delay(2000);

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem info: ");
  Serial.println(modemInfo);

  if (GSM_PIN && modem.getSimStatus() != 3)
  {
    modem.simUnlock(GSM_PIN);
  }

  modem.gprsConnect(apn, gprsUser, gprsPass);

  Serial.print("Waiting for network....");

  if (!modem.waitForNetwork())
  {
    Serial.println("Network Fail");
    delay(10000);
    return;
  }

  Serial.println("Success");

  if (modem.isNetworkConnected())
  {
    Serial.println("Network connected");
    Serial.print("Signal strength: ");
    Serial.println(modem.getSignalQuality());
  }

  Serial.print(F("Connecting to "));
  Serial.print(apn);

  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    Serial.print(" Fail");
    delay(10000);
    return;
  }

  Serial.println(" Success");
  if (modem.isGprsConnected())
  {
    Serial.println("GPRS connected");
  }

  mqtt.setServer(broker, 1883);
   
}

void loop() {

  if (!mqtt.connected())
  {
    reconnect();
  }

  mqtt.loop();
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h))
  {
    Serial.println("Failed to read sensor");
    return;
  }

  data.temperature = t;
  data.humidity = h;

  doc["temperature"] = data.temperature;
  doc["humidity"] = data.humidity;
  serializeJsonPretty(doc, dataEnv);

  long now = millis();

  if (now - lasdData > 5000) //updated sensor to the cloud every 5 seconds
  {
    lasdData = now;
    Serial.println(dataEnv);
    mqtt.publish(publishTopic, dataEnv);
  }
  
}

void reconnect(){

  while (!mqtt.connected())
  {
    Serial.println("Attempting to connect the MQTT");

    if (mqtt.connect(CLIENTID, TOKEN, NULL))
    {
      Serial.println("Connected to the MQTT");
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      Serial.println("Failed!");
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Retrying in 5 seconds");
      delay(5000);
    }
    
    
  }
  
}