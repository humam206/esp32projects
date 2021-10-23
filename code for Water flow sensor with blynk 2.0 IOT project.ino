/*

  Blynk Flow Sensor Water Meter- Calculates Total Water Consumption.

  Original Source: https://github.com/pkarun/Blynk-Flow-Sensor-Water-Meter
  in code from original source was copatible only in Blynk 1.0, so i just modified some of line code
  which compatible with Blynk 2.0 as current version. 
*/

#define BLYNK_PRINT Serial           // Uncomment for debugging 

#include "settings.h"           
#include "secret.h"                   // <<--- UNCOMMENT this before you use and change values on config.h tab
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <ESPmDNS.h>  // For OTA with ESP32
#include <WiFiUdp.h>  // For OTA
#include <ArduinoOTA.h>  // For OTA

BlynkTimer timer;

volatile long pulseCount = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
float totalLitres;
float totalLitresold;
float cost;
unsigned long oldTime;


BLYNK_CONNECTED() { // runs once at device startup, once connected to server.

  Blynk.syncVirtual(VPIN_TOTAL_LITERS); //gets last know value of V1 virtual pin

}


// Restores last know value of V1 virtual pin which we got it from blynk server
BLYNK_WRITE(VPIN_TOTAL_LITERS)
{
  totalLitresold = param.asFloat();

}

BLYNK_WRITE(VPIN_RESET) {  // reset all data with button in PUSH mode on virtual pin V4
  int resetdata = param.asInt();
  if (resetdata == 0) {
    Serial.println("Clearing Data");
    Blynk.virtualWrite(VPIN_TOTAL_LITERS, 0);
    Blynk.virtualWrite(VPIN_FLOW_RATE, 0);
    Blynk.virtualWrite(VPIN_COST, 0);
    
    flowRate = 0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    totalLitres = 0;
    totalLitresold = 0;
    cost = 0;
    
  }
}



void pulseCounter()
{
  pulseCount++;
}

void flow()
{

  if ((millis() - oldTime) > 1000)   // Only process counters once per second
  {
    detachInterrupt(PULSE_PIN);
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / FLOW_CALIBRATION;
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    totalLitres = totalLitresold + totalMilliLitres * 0.001;
    cost = (totalLitres / 1000)* 2000;
    unsigned int frac;

    // Print the flow rate for this second in liters / minute
    Serial.print("flowrate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable

    Serial.print(".");             // Print the decimal point
    frac = (flowRate - int(flowRate)) * 10; // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");

    Serial.print("  Current Liquid Flowing: ");  // Print the number of liters flowed in this second
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");

    Serial.print("  Output Liquid Quantity: ");  // Print the cumulative total of liters flowed since starting
    Serial.print(totalLitres);
    Serial.println("L");

    Serial.print("  Jumlah Biaya Konsumsi : ");  // Print the cumulative total of cost since starting
    Serial.print(cost);
    Serial.println("Rp");

    pulseCount = 0;  // Reset the pulse counter so we can start incrementing again

    attachInterrupt(PULSE_PIN, pulseCounter, FALLING);    // Enable the interrupt again now that we've finished sending output
  }

}

void sendtoBlynk()  // In this function we are sending values to blynk server
{
  Blynk.virtualWrite(VPIN_TOTAL_LITERS, totalLitres);          // Total water consumption in liters (L)
  Blynk.virtualWrite(VPIN_FLOW_RATE, flowRate);            // Displays the flow rate for this second in liters / minute (L/min)
  //  Blynk.virtualWrite(VPIN_FLOW_RATE, flowMilliLitres);  // Displays the number of liters flowed in second (mL/Sec)
  Blynk.virtualWrite(VPIN_COST, cost); // Total cost of water consumption in liters (Rp)
}

void setup()
{
  Serial.begin(57600);
  Blynk.begin(AUTH, WIFI_SSID, WIFI_PASS);
  ArduinoOTA.setHostname(OTA_HOSTNAME);  // For OTA - Use your own device identifying name
  ArduinoOTA.begin();  // For OTA

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  cost              = 0;
  oldTime           = 0;
  totalLitresold    = 0;

  pinMode(PULSE_PIN, INPUT);  // Initialization of the variable "PULSE_PIN" as INPUT (D2 pin)

  attachInterrupt(PULSE_PIN, pulseCounter, FALLING);

  timer.setInterval(10000L, sendtoBlynk); // send values blynk server every 10 sec

}


void loop()
{

  Blynk.run();
  ArduinoOTA.handle();  // For OTA
  timer.run();
  flow();

}
