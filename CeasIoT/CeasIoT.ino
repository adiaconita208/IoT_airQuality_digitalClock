/*
 * SKETCH VERSION: 20230423
 * Arduino IDE 1.8.19
 * ESP Community 3.1.1 
 * Adafruit GFX 1.11.5 https://github.com/adafruit/Adafruit-GFX-Library
 * Adafruit PCD8544 2.0.1 https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
 * Time 1.6.1 https://playground.arduino.cc/Code/Time/
 * Adafruit DHT 1.4.4 https://github.com/adafruit/DHT-sensor-library
 * Sparkfun CCS811 2.0.3 https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library
 * ThingsBoard 0.9.5 https://github.com/thingsboard/thingsboard-arduino-sdk
 *    Dependencies: 
 *    PubSub 2.8.0 https://pubsubclient.knolleary.net/
 *    ArduinoJSON 6.19.4 https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
*/

//----------- SERIAL DEBUG -------------------
#define debug

//----------- WIFI ---------------------------
#include <ESP8266WiFi.h>
char ssid[] = "";
char pass[] = "";

//----------- DISPLAY -----------------------
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
const int8_t RST_PIN = D0;
const int8_t CE_PIN = D1;
const int8_t DC_PIN = D6;
//const int8_t DIN_PIN = D7;  // Uncomment for Software SPI
//const int8_t CLK_PIN = D5;  // Uncomment for Software SPI
const int8_t BL_PIN = D8;   // BackLight
Adafruit_PCD8544 display = Adafruit_PCD8544(DC_PIN, CE_PIN, RST_PIN);

//----------- DHT11 ---------------------------
#include "DHT.h"
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//----------- CCS811 --------------------------
#include <SparkFunCCS811.h>
#define CCS811_ADDR 0x5A
CCS811 myCCS811(CCS811_ADDR);
uint16_t result = 0;

//----------- ThingsBoard ---------------------
WiFiClient espClient;
#define THINGSBOARD_ENABLE_PROGMEM 0
#include <ThingsBoard.h>
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 128U;
ThingsBoardSized<MAX_MESSAGE_SIZE> tb(espClient, MAX_MESSAGE_SIZE);
#define THINGSBOARD_SERVER  ""
#define TOKEN               ""

// -------------- NTP -------------------------
#include <WiFiUdp.h>
#include <TimeLib.h>
unsigned int localPort = 8888;  
static const char ntpServerName[] = "ro.pool.ntp.org";
WiFiUDP Udp;
const int timeZone = 3;
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
time_t lastsyncr;

// --------------- OTA ---------------------
#include <ArduinoOTA.h>
#define HOSTNAME "CeasIoT-"

// -------------- MISC --------------------
boolean myiot_ok; 
int myiot_ora; 
int myiot_minut;
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 5L * 60L * 1000L;
byte boot = 0;

void setup() {
  #ifdef debug
    Serial.begin(115200);
  #endif
  // DHT11 init
  dht.begin();
  delay(100);
  //CCS811 init
  Wire.begin(D3,D2);
  Wire.setClock(400000);
  Wire.setClockStretchLimit(1000);
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus(Wire);
  #ifdef debug
    Serial.println();
    Serial.println();
    Serial.print("[sensors_init] CCS811 begin exited with: ");
    //Pass the error code to a function to print the results
    printDriverError( returnCode );
    Serial.println();
    //This gets the latest baseline from the sensor
    result = myCCS811.getBaseline();
    Serial.print("[sensors_init] baseline for this sensor: 0x");
    if (result < 0x100) Serial.print("0");
    if (result < 0x10) Serial.print("0");
    Serial.println(result, HEX);
  #endif
  delay(1000);
  // DISPLAY init
  display.begin();
  analogWrite(BL_PIN,1023); // Backlight on
  display.setContrast(60);  // Adjust for your display
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.setTextSize(1);
  display.clearDisplay();
  display.display();
  // WiFi init
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  display.println("Hostname:" + hostname);
  display.display();
  #ifdef debug
    Serial.println("Hostname:" + hostname);
  #endif
  WiFi.begin(ssid, pass);
  delay(5000);
  while (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(17,16);
    display.setTextSize(2);
    display.print("!WiFi");
    display.display();
    delay(1000);
    #ifdef debug
      Serial.print(".");
    #endif
  }
  display.print("Connected to ");
  display.println(ssid);
  display.print("IP:");
  display.println(WiFi.localIP());
  #ifdef debug
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
  // NTP init
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  // OTA init
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  myiot_ora = -1;
  boot = 1;
}

void loop() {
    uint16_t tvoc, eco2;
    #ifdef debug
      if(timeStatus()==timeNotSet) Serial.println("Time Not Set");
      if(timeStatus()==timeNeedsSync) Serial.println("Time Needs Sync");
      //if(timeStatus()==timeSet) { Serial.print("Time Set at "); printDigits(hour(lastsyncr),false); printDigits(minute(lastsyncr),true); Serial.println();}
    #endif
    ArduinoOTA.handle();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float hic = dht.computeHeatIndex(t, h, false);
    delay(100);
    #ifdef debug 
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print("%  Temperature: ");
      Serial.print(t);
      Serial.print("°C ");
      Serial.print("Heat index: ");
      Serial.print(hic);
      Serial.println("°C ");
    #endif
    if (myCCS811.dataAvailable())  {
        //This sends the temperature data to the CCS811
        myCCS811.setEnvironmentalData(h, t);
        //Calling this function updates the global tVOC and eCO2 variables
        myCCS811.readAlgorithmResults();
        delay(100);
        tvoc=myCCS811.getTVOC();
        eco2=myCCS811.getCO2();
        result = myCCS811.getBaseline();
      }
    else if (myCCS811.checkForStatusError())
    {
      //If the CCS811 found an internal error, print it.
     #ifdef debug
      printSensorError();
     #else
      ;
     #endif
    }
    display.clearDisplay();
    if (second()<30) {
      display.drawRoundRect(2,2,80,44,3,BLACK);
      display.setTextColor(BLACK);
      display.setCursor(13,10);
      display.setTextSize(2);
      printDigits(hour(),false);
      printDigits(minute(),true);
      display.println();
      display.setTextSize(1);
      display.setCursor(13,26);
      display.print(day());
      display.print("/");
      display.print(month());
      display.print("/");
      display.print(year()); 
      display.println();
      display.display();  
    }
    else {
      display.fillScreen(BLACK);
      display.drawRoundRect(2,2,80,44,3,WHITE);
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(22,10);
      display.print((int)t);
      display.print((char)247);
      display.println("C");
      display.setTextSize(2);
      if (myiot_ora!=-1) {
        if (myiot_ok) {  
          display.setCursor(30,26);
          display.println("OK");    }
        else {
          display.setCursor(20,26);
          display.println("FAIL"); }
//        display.setCursor(30,32);
//        display.setTextSize(1);
//        printDigits(myiot_ora,false);
//        printDigits(myiot_minut,true);
      }
      display.display();
    }
    if (millis() - lastConnectionTime > postingInterval) {
      if (IoTpublish(t,h,tvoc,eco2,result)>0)
        { myiot_ok=true; }
      else
        { myiot_ok=false; }
      myiot_ora = hour();
      myiot_minut = minute();
    }
}

int IoTpublish(float temperature, float humidity, int tvoc, int eco2, int baseline) {
  if (!tb.connected()) {
      #ifdef debug
        Serial.print("[thingsboard_post] Connecting to: ");
        Serial.print(THINGSBOARD_SERVER);
        Serial.print(" with token ");
        Serial.println(TOKEN);
      #endif
      int con = tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
      if (!con) {
        #ifdef debug
          Serial.println("[thingsboard_post] Failed to connect: " + String(con,HEX));
        #endif
        return 0;
      }
    }
    #ifdef debug
      Serial.println("[thingsboard_post] Sending data...");
      Serial.println("[thingsboard_post] CCS811 data:");
      Serial.print("[thingsboard_post] CO2 concentration : ");
      Serial.print(eco2);
      Serial.println(" ppm");
      Serial.print("[thingsboard_post] TVOC concentration : ");
      Serial.print(tvoc);
      Serial.println(" ppb");
      Serial.print("[thingsboard_post] baseline for the sensor: 0x");
      if (result < 0x100) Serial.print("0");
      if (result < 0x10) Serial.print("0");
      Serial.println(result, HEX);
      Serial.print("[thingsboard_post] Temperature: ");
      Serial.print(temperature, 2);
      Serial.println(" degrees C");
      Serial.print("[thingsboard_post] %RH: ");
      Serial.print(humidity, 2);
      Serial.println(" %");
    #endif
    const int data_items = 5;
    Telemetry data[data_items];
    data[0] = { "temperature", temperature };
    data[1] = { "humidity", humidity };
    data[2] = { "tvoc", tvoc };
    data[3] = { "eco2", eco2 };
    data[4] = { "baseline", baseline };
    tb.sendTelemetry(data, data_items);
    if (boot==1) {
        tb.sendTelemetryInt("boot", boot);
        boot = 0;
    }
    tb.loop();
    lastConnectionTime = millis();
    return 1;
}

void printDigits(int digits, boolean dots){
  // utility for digital clock display: prints preceding colon and leading 0
  if (dots) display.print(":");
  if(digits < 10)
    display.print('0');
  display.print(digits);
}

//---------------------------------------------------------------
//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void printDriverError( CCS811Core::CCS811_Status_e errorCode ) {
  switch ( errorCode )
  {
    case CCS811Core::CCS811_Stat_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::CCS811_Stat_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::CCS811_Stat_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::CCS811_Stat_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::CCS811_Stat_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}

////printSensorError gets, clears, then prints the errors
////saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}

/*-------- NTP code ----------*/
time_t getNtpTime()
{
  IPAddress timeServerIP;
  WiFi.hostByName(ntpServerName, timeServerIP);
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      lastsyncr = (time_t) (secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return lastsyncr;
    }
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
