#include <Arduino.h>
#include <WiFi.h>
#include "fiwareClient.h"

const char *WIFI_SSID = "iPhone de Nacho";
const char *WIFI_PASS = "Nacho4104";

const char *dataTemplate =
    "{\"biomassFluidAmount\":{\"type\":\"Number\",\"value\":9.39},\"biomassPh\":{\"type\":\"Number\",\"value\":7.97},\"biomassTemperature\":{\"type\":\"Number\",\"value\":16.89},\"biomassWeight\":{\"type\":\"Number\",\"value\":45.07},\"environmentGasCO2\":{\"type\":\"Number\",\"value\":3.11},\"environmentGasO2\":{\"type\":\"Number\",\"value\":12.78},\"environmentHumidity\":{\"type\":\"Number\",\"value\":60.28},\"environmentTemperature\":{\"type\":\"Number\",\"value\":26.85},\"id\":\"001\",\"type\":\"composter\"}";

fiwareClient fiware;

// Wifi connection manager
void wifiConnection()
{
  delay(5000);
  // Print wifi connection info
  Serial.println("Conectando a:");
  Serial.println(WIFI_SSID);
  // Connect to Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Wait until successfull connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }
  // Show connection info
  Serial.println("Conectado a Wifi");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void setup()
{
  Serial.begin(115200);
  wifiConnection();
}

void loop()
{
  // Initial CPU pause
  delay(10);

  // Get Fiware token
  String token = fiware.getToken();
  delay(2000);
  ApiResponse response = fiware.sendData("001", dataTemplate);

  delay(30000);
}