# IoT Air Quality Digital Clock

Our IoT Air Quality Digital Clock is an ambitious project that aims to help people monitor and
improve the air quality from the spaces they live in. Our device sends data about Temperature,
Humidity, TVOC and CO2 levels to Thingsboard, a platform where you cand design your own dashboard
and even add a telegram bot to alert you through text messages.

## Installation

Use the "git clone" command to copy our repository and upload it on your own device using Arduino IDE.
```bash
git clone
```
Make sure you add your own Wi-Fi network, password and your own ThingsBoard token. Congrats, your
device is ready to use!
``` c
#include <ESP8266WiFi.h>
char ssid[] = "Your_WIFI_address";
char pass[] = "Your_WIFI_password";
```
``` c
WiFiClient espClient;
#define THINGSBOARD_ENABLE_PROGMEM 0
#include <ThingsBoard.h>
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 128U;
ThingsBoardSized<MAX_MESSAGE_SIZE> tb(espClient, MAX_MESSAGE_SIZE);
#define THINGSBOARD_SERVER "Your_Server_IP"
#define TOKEN "Your_ThingsBoard_Token"
```

## Usage

The device is very easy to use. Just make sure you keep it connected to a power source and to a Wi-Fi network.
For best results find a circulated area to put the device, so as the air flow will give more accurate data.