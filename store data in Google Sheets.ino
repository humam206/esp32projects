/*

  this is an additional program to store your data projects in your Google Sheets Cloud using the IFTTT Platform.
  don't forget to set and upload your library's secret.h and settings.h.
 
*/

#define BLYNK_PRINT Serial           // Uncomment for debugging 

#include "settings.h"           
#include "secret.h"                   // <<--- UNCOMMENT this before you use and change values on config.h tab
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <ESPmDNS.h>  // For OTA with ESP32
#include <WiFiUdp.h>  // For OTA
#include <ArduinoOTA.h>  // For OTA

BlynkTimer timer;
String server = "http://maker.ifttt.com";
String eventName = "YourNameEvent";
String IFTTT_Key = "yourIFTTT_key";
String IFTTTUrl = "yourIFTTTUrl";

float value1;
float value2;
float value3;

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
    Serial.print("L");

    Serial.print("  Total Cost : ");  // Print the cumulative total of cost since starting
    Serial.print(cost);
    Serial.println("Rp");

    value1 = flowRate;
    value2 = totalLitres;
    value3 = cost;

    pulseCount = 0;  // Reset the pulse counter so we can start incrementing again

    attachInterrupt(PULSE_PIN, pulseCounter, FALLING);    // Enable the interrupt again now that we've finished sending output
  }

}

// program for send data to Blynk
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

// programm for send data to google sheets
void sendDataToSheet(void)
{
  String url = server + "/trigger/" + eventName + "/with/key/" + IFTTT_Key + "?value1=" + String((float)value1)+ "&value2=" + String((float)value2) + "&value3=" + String((float)value3); 
  Serial.println(url);
  //start to send data to IFTTT
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  http.begin(url); //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
     }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void loop()
{

  Blynk.run();
  ArduinoOTA.handle();  // For OTA
  timer.run();
  flow();
  sendDataToSheet();
  delay(2000);

}
