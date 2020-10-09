/*
  ArduinoMqttClient - WiFi Simple Sender

  This example connects to a MQTT broker and publishes a message to
  a topic once a second.

  The circuit:
  - Arduino MKR 1000, MKR 1010 or Uno WiFi Rev.2 board

  This example code is in the public domain.
*/

#include <ArduinoMqttClient.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoJson.h>
#include <DHT.h>
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
  #include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
  #include <WiFi101.h>
#elif defined(ARDUINO_ESP8266_ESP12)
  #include <ESP8266WiFi.h>
#endif

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.

WiFiClient wifiClient;
BearSSLClient sslClient(wifiClient);
MqttClient mqttClient(sslClient);

const char broker[] = "a3oftcoi6wgbdt-ats.iot.eu-west-1.amazonaws.com";
int        port     = 8883;
const char topic[]  = "arduino/simple";
const char certificate[] = R"(-----BEGIN CERTIFICATE-----
MIICozCCAYugAwIBAgIVAPBWFGcCtVb3vDjVgjI++31+TKogMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMDEwMDkxMjUx
MDhaFw00OTEyMzEyMzU5NTlaMDIxEzARBgNVBAoTCk9tZWdhcG9pbnQxGzAZBgNV
BAMTEjAxMjM0MEExMDVBMkJBNURFRTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IA
BArq/0CTDI7QymfhfWJybdljDWLxuI1iDg7tzJRyEYw1wbRbVBWghSWnWWDg1PBt
oWqGFNsBLh8UtRNUe+00e8mjYDBeMB8GA1UdIwQYMBaAFJDdCoFq4qbRne4S0sKE
kh9Rz8XqMB0GA1UdDgQWBBQ4B2r6BMgOawPdx7+wCuK5iKshhTAMBgNVHRMBAf8E
AjAAMA4GA1UdDwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAT2iGvW3WJiAM
Vbg2PBVmcLZqCpHUjEXAT3VajGOHmo+O+WkSBMBRCDtsPj7IZmFQJubNhDe5gIDN
gHkCUdPNcQ1Y8VtFsvc7IT7JrF90K80nL8ex4IoOWxMyhFnnkigx+S1VQHTGlMu+
B0Q6bHVipBSwh8Gk1kBSvq4EDRK0wHYXjJcP0Y9BOstN0XWPVSs4/q9oxhYboxhR
OH+mf27HStgSy1B+BvHodAtiLLuN/WRzM/Ceh2sC5K5MmAXYPW/MVdxaU5225rdo
cZ9w05aQNXt3hyEXvGb45ARsbjFku9xOm+LjLjTjsjpv2fXR2FMKwTKejmj7CMW0
u4Zl0OGMdA==
-----END CERTIFICATE-----
)";

const long interval = 1000;
unsigned long previousMillis = 0;
unsigned long lastMillis = 0;

int count = 0;

//Constants
#define DHTPIN 0     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
//Variables
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value

void setup() {
  Serial.begin(115200);  //
  while (!Serial);
  if (!ECCX08.begin())  {
    Serial.println("No ECCX08 present!");    
    while (1); 
  }

  dht.begin();
  
  // Set a callback to get the current time
  // used to validate the servers certificate  
  ArduinoBearSSL.onGetTime(getTime);
  
  // Set the ECCX08 slot to use for the private key  
  // and the accompanying public certificate for it  
  sslClient.setEccSlot(0, certificate);
  
  // Optional, set the client id used for MQTT,  
  // each device that is connected to the broker  
  // must have a unique client id. The MQTTClient will generate  
  // a client id for you based on the millis() value if not set  
  //  // 
  mqttClient.setId("clientId");
  
  // Set the message callback, this function is  
  // called when the MQTTClient receives a message  
  mqttClient.onMessage(onMessageReceived);
}

void onMessageReceived(int messageSize){
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic ‘");
  Serial.print(mqttClient.messageTopic());
  Serial.print("’, length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  // use the Stream interface to print the contents
  while (mqttClient.available())
  {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();
  Serial.println();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }
  if (!mqttClient.connected())
  {
    // MQTT client is disconnected, connect
    connectMQTT();
  }
  // poll for new MQTT messages and send keep alives
  mqttClient.poll();
  // publish a message roughly every 60 seconds.
  if (millis() - lastMillis > 60000)
  {
    lastMillis = millis();
    readTemperature();
    publishMessage();
  }
}

void readTemperature() {
  //Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
}

unsigned long getTime() {
  // get the current time from the WiFi module
  return WiFi.getTime();
}

void connectMQTT(){
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");
  
  while (!mqttClient.connect(broker, 8883))  {
    // failed, retry
    Serial.print("MQTT Failed!\n");
    delay(5000);
  }
  Serial.println();
  Serial.println("You’re connected to the MQTT broker");
  Serial.println();
  
  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void connectWiFi(){
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED)
  {
    // failed, retry
    Serial.print("Wifi Failed!\n");
    delay(5000);
  }
  Serial.println();
  Serial.println("You’re connected to the network");
  Serial.println();
}

void publishMessage(){
  String messageTopic = "arduino/outgoing";
  Serial.println("Publishing message to " + messageTopic);
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(messageTopic);
  DynamicJsonDocument doc(2048);
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  serializeJsonPretty(doc, Serial);
  Serial.println();
}
