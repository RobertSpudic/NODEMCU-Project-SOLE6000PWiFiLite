#include <Arduino.h>
#include "Credentials.h"

// definiraj naziv skecha i verziju
#define ArduinoSkechIme "SOLE6000PWiFiLite.ino"
#define ArduinoSkechVerzija "0.1.0"
#define BlynkProject "SOLE6000PWiFiLite"
#define BlynkProjectVerzija "0.1.0"

// definiraj Blynk virtualne pinove
#define BlynkVirtualPinGumb_ON V1
#define BlynkVirtualPinGumb_OFF V2
#define BlynkVirtualPinGumb_Auto V3
#define BlynkVirtualPinGumb_Prog V4
#define BlynkVirtualPinGumb_Minus V5
#define BlynkVirtualPinGumb_Plus V6
#define BlynkVirtualPinTerminal V11
#define BlynkVirtualPinEvent_1 V21
#define BlynkVirtualPinEvent_2 V22
//#define BlynkVirtualPinVrijeme V31
//#define BlynkVirtualPinDatum V32

// definiraj IR pinove
#define IRSendPin 4

// definiraj IR kodove
#define IRKod_GORIONIK 0x807F807F
#define IRKod_TIMER 0x807F906F
#define IRKod_PLAMEN 0x807F8877
#define IRKod_TEMPminus 0x807F28D7
#define IRKod_TEMPplus 0x807F6897

// definiraj httpupdate linkove
const char *FirmwareDownloadLink1 = CREDENTIAL_HTTPUPDATE_FIRMWARE_DOWNLOAD_LINK_1;
const char *FirmwareDownloadLink2 = CREDENTIAL_HTTPUPDATE_FIRMWARE_DOWNLOAD_LINK_2;
const char *FirmwareDownloadLink3 = CREDENTIAL_HTTPUPDATE_FIRMWARE_DOWNLOAD_LINK_3;
const char *DownloadLinkFingerprint = CREDENTIAL_HTTPUPDATE_DOWNLOAD_LINK_FINGERPRINT;

/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  You can send/receive any data using WidgetTerminal object.

  App project setup:
    Terminal widget attached to Virtual Pin BlynkVirtualPinTerminal
 *************************************************************

  Blynk can provide your device with time data, like an RTC.
  Please note that the accuracy of this method is up to several seconds.

  App project setup:
    RTC widget (no pin required)
    Value Display widget on BlynkVirtualPinVrijeme
    Value Display widget on BlynkVirtualPinDatum

  WARNING :
  For this example you'll need Time keeping library:
    https://github.com/PaulStoffregen/Time

  This code is based on an example from the Time library:
    https://github.com/PaulStoffregen/Time/blob/master/examples/TimeSerial/TimeSerial.ino
 *************************************************************/

/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <FS.h> //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in config.json, they are overwritten.
char blynk_token[34] = CREDENTIAL_BLYNK_AUTH_TOKEN;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
//char auth[] = CREDENTIAL_BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = CREDENTIAL_MY_WIFI_SSID;
char pass[] = CREDENTIAL_MY_WIFI_PASSWORD;

// Attach virtual serial terminal to Virtual Pin
WidgetTerminal terminal(BlynkVirtualPinTerminal);

BlynkTimer timer;

WidgetRTC rtc;

IRsend irsend(IRSendPin); // Set the GPIO to be used to sending the message.

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// httpupdate calback
void update_started()
{
  Serial.println("MCU:  HTTP update zapoceo...");
  terminal.println("MCU:  HTTP update zapoceo...");

  // Ensure everything is sent
  terminal.flush();
}

void httpUpdate()
{
  // if WiFi connected
  if (WiFi.status() == WL_CONNECTED)
  {
    vremenskiZig();
    Serial.println("MCU:  Trazim update");
    terminal.println("MCU:  Trazim update");

    // Ensure everything is sent
    terminal.flush();

    WiFiClient client;

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    //ESPhttpUpdate.onEnd(update_finished);
    //ESPhttpUpdate.onProgress(update_progress);
    //ESPhttpUpdate.onError(update_error);

    ESPhttpUpdate.update(FirmwareDownloadLink1, "", DownloadLinkFingerprint);
    ESPhttpUpdate.update(FirmwareDownloadLink2, "", DownloadLinkFingerprint);
    ESPhttpUpdate.update(FirmwareDownloadLink3, "", DownloadLinkFingerprint);
    // Or:
    //t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

    Serial.println("MCU:  Nema novih updatova");
    terminal.println("MCU:  Nema novih updatova");

    // Ensure everything is sent
    terminal.flush();
  }
}

// Digital clock display of the time
void vremenskiZig()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details

  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + "." + month() + "." + year() + ".";

  Serial.print("\n@ " + currentTime + "  " + currentDate + "\n");
  terminal.print("\n@ " + currentTime + "  " + currentDate + "\n");
  terminal.flush();

  // Send time to the App
  //Blynk.virtualWrite(BlynkVirtualPinVrijeme, currentTime);
  // Send date to the App
  //Blynk.virtualWrite(BlynkVirtualPinDatum, currentDate);
}

// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(BlynkVirtualPinTerminal)
{
  // if you type "..." into Terminal Widget - it will
  if (String("clt") == param.asStr())
  {
    // ocisti terminal
    terminal.clear();
  }
  else if (String("dlw") == param.asStr())
  {
    vremenskiZig();
    Serial.println("MCU:");
    Serial.println("WiFi lozinka je izbrisana iz memorije");
    Serial.println("i MCU ce biti resetiran.");
    Serial.println();
    Serial.println("Potrebno je ponovno postaviti WiFi vezu");
    Serial.println("za povezivanje MCU-a s internetom.");
    terminal.println("MCU:");
    terminal.println("WiFi lozinka je izbrisana iz memorije");
    terminal.println("i MCU ce biti resetiran.");
    terminal.println();
    terminal.println("Potrebno je ponovno postaviti WiFi vezu");
    terminal.println("za povezivanje MCU-a s internetom.");
    terminal.flush();

    // ocisti WiFi credentials
    wifiManager.resetSettings();

    delay(1500);
    // resetiraj MCU
    ESP.restart();
  }
  else if (String("dlb") == param.asStr())
  {
    vremenskiZig();
    Serial.println("MCU:");
    Serial.println("Blynk token i WiFi lozinka su izbrisani");
    Serial.println("iz memorije a MCU ce biti resetiran.");
    Serial.println();
    Serial.println("Potrebno je ponovno postaviti token i");
    Serial.println("WiFi vezu za rad MCU-a putem interneta.");
    terminal.println("MCU:");
    terminal.println("Blynk token i WiFi lozinka su izbrisani");
    terminal.println("iz memorije a MCU ce biti resetiran.");
    terminal.println();
    terminal.println("Potrebno je ponovno postaviti token i");
    terminal.println("WiFi vezu za rad MCU-a putem interneta.");
    terminal.flush();

    // ocisti Blynk token
    SPIFFS.format();

    // ocisti WiFi credentials
    wifiManager.resetSettings();

    delay(1500);
    // resetiraj MCU
    ESP.restart();
  }
  else if (String("rst") == param.asStr())
  {
    vremenskiZig();
    Serial.println("MCU: resetiram MCU");
    terminal.println("MCU: resetiram MCU");
    terminal.flush();

    delay(1500);
    // resetiraj MCU
    ESP.restart();
  }
  else if (String("upd") == param.asStr())
  {
    // pokreni httpupdate
    httpUpdate();
  }
  else if (String("pin") == param.asStr())
  {
    // izlistaj koristene pinove
    vremenskiZig();
    terminal.println("Blynk Virtual pinovi:");
    terminal.println("Gumb ON   - V1");
    terminal.println("Gumb OFF  - V2");
    terminal.println("Gumb Auto - V3");
    terminal.println("Gumb Prog - V4");
    terminal.println("Gumb -    - V5");
    terminal.println("Gumb +    - V6");
    terminal.println("TERMINAL  - V11");
    terminal.println("EVENT 1   - V21");
    terminal.println("EVENT 2   - V22");
    terminal.println();
    terminal.println("MCU pinovi:");
    terminal.println("TX Serial - TX (22)");
    terminal.println("RX Serial - RX (21)");
    terminal.println("IR Send   - D2 (4)");
    terminal.flush();
  }
  else if (String("irc") == param.asStr())
  {
    // izlistaj IR kodove
    vremenskiZig();
    terminal.println("IR kodovi:");
    terminal.println("GORIONIK");
    terminal.println("NEC(0x807F807F)");
    terminal.println();
    terminal.println("TIMER");
    terminal.println("NEC(0x807F906F)");
    terminal.println();
    terminal.println("PLAMEN");
    terminal.println("NEC(0x807F8877)");
    terminal.println();
    terminal.println("TEMP. -");
    terminal.println("NEC(0x807F28D7)");
    terminal.println();
    terminal.println("TEMP. +");
    terminal.println("NEC(0x807F6897)");
    terminal.flush();
  }
  else if (String("vic") == param.asStr())
  {
    // kratki vic
    // ocisti terminal
    terminal.clear();
    terminal.println("Ozbiljno Uzi?");
    terminal.println("?");
    terminal.println("Ozbiljno?!");
    terminal.flush();
  }
  else
  {
    // Send it back
    vremenskiZig();
    terminal.println("Komande:");
    terminal.println("clt - ocisti TERMINAL");
    terminal.println("dlw - obrisi WiFi lozinku");
    terminal.println("dlb - obrisi Blynk token");
    terminal.println("rst - restartaj MCU");
    terminal.println("upd - pokreni web update");
    terminal.println("pin - lista pinova");
    terminal.println("irc - lista IR kodova");
    terminal.println("vic - kratki vic");
    terminal.flush();
  }
}

BLYNK_WRITE(BlynkVirtualPinGumb_ON) // Gumb "GORIONIK ON"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: ON");
    terminal.println("BlynkAPP: ON");

    // posalji IR kod
    irsend.sendNEC(IRKod_GORIONIK);
    Serial.println("MCU: IR Kod GORIONIK poslan");
    terminal.println("MCU: IR Kod GORIONIK poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: GORIONIK ON off");
    //terminal.println("BlynkAPP: GORIONIK ON off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinGumb_OFF) // Gumb "GORIONIK OFF"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: OFF");
    terminal.println("BlynkAPP: OFF");

    // posalji IR kod
    irsend.sendNEC(IRKod_GORIONIK);
    Serial.println("MCU: IR Kod GORIONIK poslan");
    terminal.println("MCU: IR Kod GORIONIK poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: GORIONIK OFF off");
    //terminal.println("BlynkAPP: GORIONIK OFF off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinGumb_Auto) // Gumb "TIMER Auto"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: Auto");
    terminal.println("BlynkAPP: Auto");

    // posalji IR kod
    irsend.sendNEC(IRKod_TIMER);
    Serial.println("MCU: IR Kod TIMER poslan");
    terminal.println("MCU: IR Kod TIMER poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: TIMER Auto off");
    //terminal.println("BlynkAPP: TIMER Auto off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinGumb_Prog) // Gumb "PLAMEN Prog"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: Prog");
    terminal.println("BlynkAPP: Prog");

    // posalji IR kod
    irsend.sendNEC(IRKod_PLAMEN);
    Serial.println("MCU: IR Kod PLAMEN poslan");
    terminal.println("MCU: IR Kod PLAMEN poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: PLAMEN Prog off");
    //terminal.println("BlynkAPP: PLAMEN Prog off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinGumb_Minus) // Gumb "TEMP. -"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: -");
    terminal.println("BlynkAPP: -");

    // posalji IR kod
    irsend.sendNEC(IRKod_TEMPminus);
    Serial.println("MCU: IR Kod TEMP. - poslan");
    terminal.println("MCU: IR Kod TEMP. - poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: TEMP. - off");
    //terminal.println("BlynkAPP: TEMP. - off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinGumb_Plus) // Gumb "TEMP. +"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: +");
    terminal.println("BlynkAPP: +");

    // posalji IR kod
    irsend.sendNEC(IRKod_TEMPplus);
    Serial.println("MCU: IR Kod TEMP. + poslan");
    terminal.println("MCU: IR Kod TEMP. + poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: TEMP. + off");
    //terminal.println("BlynkAPP: TEMP. + off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinEvent_1) // Event 1 "GORIONIK ON"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: Event 1 - GORIONIK ON");
    terminal.println("BlynkAPP: Event 1 - GORIONIK ON");

    // posalji IR kod
    irsend.sendNEC(IRKod_GORIONIK);
    Serial.println("MCU: IR Kod GORIONIK poslan");
    terminal.println("MCU: IR Kod GORIONIK poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: Event 1 - GORIONIK ON off");
    //terminal.println("BlynkAPP: Event 1 - GORIONIK ON off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(BlynkVirtualPinEvent_2) // Event 2 "GORIONIK OFF"
{
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue == true)
  {
    vremenskiZig();
    Serial.println("BlynkAPP: Event 2 - GORIONIK OFF");
    terminal.println("BlynkAPP: Event 2 - GORIONIK OFF");

    // posalji IR kod
    irsend.sendNEC(IRKod_GORIONIK);
    Serial.println("MCU: IR Kod GORIONIK poslan");
    terminal.println("MCU: IR Kod GORIONIK poslan");
  }
  if (pinValue == false)
  {
    //Serial.println("BlynkAPP: Event 2 - GORIONIK OFF off");
    //terminal.println("BlynkAPP: Event 2 - GORIONIK OFF off");
  }
  // Ensure everything is sent
  terminal.flush();
}

BLYNK_CONNECTED()
{
  // Synchronize time on connection
  rtc.begin();
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  Serial.println("Arduino Skech " ArduinoSkechIme);
  Serial.println("Arduino Skech v" ArduinoSkechVerzija);
  Serial.println("Blynk Project " BlynkProject);
  Serial.println("Blynk Project v" BlynkProjectVerzija);
  Serial.println("Blynk Library v" BLYNK_VERSION);

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");

          strcpy(blynk_token, json["blynk_token"]);
        }
        else
        {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(300);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(CREDENTIAL_WIFIMANAGER_AP_SSID, CREDENTIAL_WIFIMANAGER_AP_PASSWORD))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Blynk.begin(blynk_token, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  // Clear the terminal content
  terminal.clear();

  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Arduino Skech " ArduinoSkechIme));
  terminal.println(F("Arduino Skech v" ArduinoSkechVerzija));
  terminal.println(F("Blynk Project " BlynkProject));
  terminal.println(F("Blynk Project v" BlynkProjectVerzija));
  terminal.println(F("Blynk Library v" BLYNK_VERSION));
  terminal.flush();

  irsend.begin();

  setSyncInterval(4 * 60 * 60); // Sync interval in seconds (4 sata)

  // trazi update svakih 12h
  timer.setInterval(43200000L, httpUpdate);
}

void loop()
{
  Blynk.run();
  timer.run();
}