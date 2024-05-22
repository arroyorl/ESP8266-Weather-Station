  /*
 *  This sketch reports to wunderground a weather station using several sensors
 *  The server will show the current temperature, humidity and pressure with a BME280
 *  reports wind speed and rain from interrupts from the misol weather station 
 *  wind direction using the analog 0 and the wind direction sensor from the misol weather station
 *  During the setup the server will act as AP mode so that you can set the
 *  Wifi connection, then it will try to connect to the WiFi.
 *  While serving the webserver and showing the temperature, it will also post
 *    the data to Wunderground, so that the data can be
 *    retrieved without having to open port to your router.
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 *  
 *  Weather sensors code is based on  Weather Station using the Electric Imp
 *  By: Nathan Seidle
 */


///////////////////////////////////////////////////////////////
//           History                                         //
///////////////////////////////////////////////////////////////
//  1.0 First installable version,
//  1.1 Chnaged name of windSpeed in get_wind_speed() to avoid 
//      confusion with global variable
//      changed place wher windgust is calculate
//  2.0 added radiance sensor
//  2.1 read UV as avegare of NUM_SAMPLES readings
//      calculate weather data every minute instead on demand
//  2.2 read temp, humid and pressure as average of NUM_SAMPLES
//      corrected error when no wifi is accessible
//  2.3 corrected windgustmph calculation
//  2.4 correct error in windgust calculation
//      updated BME 280 library
//  2.5 increased time to ignore switch-bounce glitches 
//
//  2.6 updated sendHTTPsGet removing the need of fingerprint
//      removed fingerprint update web page
//      replaced NtpClientLib by standard Time.h library
//
//  3.0 split main .ino file into smaller files (.hpp)
//      (beta version under development)
//
//  3.1 rebuild calculations for winspeed and wingust. Now calculate on 5 secs intervals and average of last minute
//      rain now stored in a circular buffer of 24 h * 60 mins, calculate totals for last hour and last 24 hours, instead of natural days 
//      (beta version under development)
//
//  3.2 Calculating rain rate based on Davis weather stations procedure
//      (beta version under development)
//
//  3.2.1 Corrected error in rain_rate calculation
//
//  3.3 sending weather data over MQTT
//
//  3.4 Corrected rain rate calculation (rain.hpp v2.0)
//
//  3.5 Added TSL-2591 (SQM/luminosity), WEML6075 (UV) and GY906 (cloud) sensors
//      removed ML1145 (UV) module
//      BME280: no read data if sensor is not active (fail in initialization)
//      INA219: no read data if sensor is not active (fail in initialization)
//      increased mqtt buffer size to 1024
//      json: only send adta of active/connected sensors
//
///////////////////////////////////////////////////////////////
 

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>  

#include <time.h> 

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>   // v 1.9.12

#include <CircularBuffer.h> // v 1.4.0

#define FVERSION  "v3.5"

///////////////////////////////////////////////////
// Comment for final release
//#define RDEBUG
//#define WINDIRDEBUG
///////////////////////////////////////////////////

#include "Rdebug.h"
#include "settings.h"

#define GPIO0 0
#define GPIO1 1
#define GPIO2 2
#define GPIO3 3
#define GPIO4 4
#define GPIO5 5
#define GPIO6 6
#define GPIO7 7
#define GPIO8 8
#define GPIO9 9
#define GPIO10 10
#define GPIO11 11
#define GPIO12 12
#define GPIO13 13
#define GPIO14 14
#define GPIO15 15
#define GPIO16 16

#define LEDON LOW
#define LEDOFF HIGH
#define SETUP_PIN GPIO0
#define WAKEUP_PIN GPIO16
#define DHT_PIN GPIO5 // D1
#define WINDIR  A0

#define RAIN_PIN GPIO13 // D7 
#define ANNEMOMETER_PIN GPIO14 // D5

#define   NUM_SAMPLES 5

// Define Red and green LEDs
#ifdef ARDUINO_ESP8266_NODEMCU_ESP12E
# define RED_LED GPIO12 // D6
# define GREEN_LED GPIO10 // D12 - SD3
#endif
#ifdef ARDUINO_ESP8266_GENERIC
# define RED_LED GPIO12 //    antes GPIO9
# define GREEN_LED GPIO10
#endif

//Struct to store setting in EEPROM
Settings    settings;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
String      data;
String      httpUpdateResponse;
const char* ap_ssid = "METEO-TEMP";
const char* ap_password = ""; //"12345678";
int         ap_setup_done = 0;
float       rain1hmm=0;     // total rain in last 1 hour
float       rain24hmm=0;    // total rain in last 24 hours
float       rainrate1hmm;     // rain rate in 1 hour
float       maxrainrate=0;  // maximum rain rate, used get maximum rainrate over send interval
long        time_offset=0;
bool        firstloop=true;

rst_info *myResetInfo;

// global variables for time
time_t      now;                    // this are the seconds since Epoch (1970) - UTC
tm          tm;                     // the structure tm holds time information in a more convenient way
char        hhmm[6];                // variable to store time in format HH:MM

long lastSecond; //The millis counter to see when a second rolls by
unsigned int minutesSinceLastReset; //Used to reset variables after 24 hours. Imp should tell us when it's midnight, this is backup.
int  minutes_send = 0; // Keeps track of when send data to Wundergound
bool midnight = false;  // for reset variable at midnight only one time
uint8_t currentday;  // to store current day and reset data at midnight when day changes

//We need to keep track of the following data:
//  Rain over the past hour (store 1 per minute)
//  Total rain over 24 hours (store one per day)
// we use a circular buffer to store clicks from rain gauge over last 24 hours 
CircularBuffer<unsigned int, 24*60> rain_buffer; // 24 hous * 60 mins circular buffer

// function prototypes
int setupSTA();
String prepareJsonData();

// MQTT functions
#include  "mqtt.hpp"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Include functions for sensors
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "bme280.hpp"           // temperature, humidity, pression
// #include "ml1145.hpp"           // UV
#include "ina219.hpp"           // radiance
#include "gy906.hpp"            // sky temperature, clouds
#include "sqm-tsl2591.hpp"      // SQM (TSL2591)
#include "veml6075.hpp"         // UVA, UVB and UV index

#include "rain.hpp"
#include "wind.hpp"

// Web page for initial configuration
ESP8266WebServer setupserver(80);
#include "initpage.h"
// web page for full configuration
ESP8266WebServer server(80);
#include "mainPage.h"
// Web server for respond to rawdata web requestes and send data to wonderground
#include "webserver.hpp"


void displayBusy(){
  digitalWrite(RED_LED, LEDON);
  delay(500);
  digitalWrite(RED_LED, LEDOFF);
  delay(500);
}

//Calculates each of the variables that wunderground is expecting
void calcWeather()
{
    Debug("windspeed: ");
    DebugLn(windspeed);
    Debug("windgust: ");
    DebugLn(windgust);
    Debug("winddir: ");
    DebugLn(winddir);

    // Get Temperature, humidity and pressure
    getBME280data();

    // Get UV index
    // getML1145data();

    // get Radiance
    getINA219data();

    // get sky temperature (clouds) data
    get_gy906data();

    // get SQM and luminosity data
    getSQMdata();

    // get UV data
    get_veml6075data();
    
    //Calculate amount of rainfall for the last 60 minutes
    unsigned int buffsize = rain_buffer.size();
    unsigned long rainsum = 0;
    if ( buffsize > 60 ) 
    {
      // buffer contains at least 1 hour of rain data, add 60 mins clicks and calculate rainfall
      for( decltype(rain_buffer)::index_t i = buffsize - 60; i < buffsize; i++)
      {
        rainsum += rain_buffer[i];
      }
    }
    else if (! rain_buffer.isEmpty())
    {
      // buffer is not empty but smaller than 1 hour
//      Debug("buffsize: ");
//      DebugLn(buffsize);
      for( decltype(rain_buffer)::index_t i = 0; i < buffsize; i++)
      {
        rainsum += rain_buffer[i];
//        Debug("rain_buffer[");
//        Debug(i);
//        Debug("]: ");
//        DebugLn(rain_buffer[i]);
      }
    } 
    rain1hmm = rainsum * 0.2794; // Each dump is 0.011" of water = 0.011 * 25.4 = 0.2794 mm
    Debug("rainsum: ");
    Debug(rainsum);
    Debug(", rain 1h: ");
    Debug(rain1hmm);

    // calculate amount of rainfall over last 24 hours (or time registered)
    rainsum=0;
    if (buffsize > 0) 
    {
      // rain buffer is not empty
      for( decltype(rain_buffer)::index_t i = 0; i < buffsize; i++)
      {
        rainsum += rain_buffer[i];
      }
    }
    rain24hmm = rainsum * 0.2794;
    Debug(", rain 24h: ");
    DebugLn(rain24hmm);
    
}

String prepareJsonData() {

  time(&now);  // get current time

  String res =  "{\"time\": \"" + String(now) + "\"" + 
                ",\"sensors\":" +
                    "{ \"windspeed\":" + String(windspeed) +
                    ", \"windgust\":" + String(windgust) +
                    ", \"winddirdeg\": \"" + String(winddir) + "\"" +
                    ", \"winddir\": \"" + WindDirString(winddir) + "\"" +
                    ", \"rain1h\":" + String(rain1hmm) +
                    ", \"rain24h\":" + String(rain24hmm) +
                    ", \"rainrate1h\":" + String(rainrate1hmm);
  if (bme280Active) {
        res +=      ",\"temperature\":" + String(temperature) + 
                    ", \"humidity\":" + String(humidity) + 
                    ", \"dewpoint\":" + String(dewpoint) +
                    ", \"pressure\":" + String(pressure) + 
                    ", \"sealevelpressure\":" + String(sealevelpressure);
  }
  if (ina219Active) {
        res +=      ", \"shunt mV\":" + String(shuntVoltage) + 
                    ", \"cell mA\":" + String(cellCurrent) + 
                    ", \"radiance\":" + String(radiance);
  }
  if (gy906connected) {
        res +=      ", \"skyT\":" + String(skyTemp) + 
                    ", \"rawSkyT\":" + String(objTemp) + 
                    ", \"ambientT\":" + String(ambientTemp) + 
                    ", \"cloudI\":" + String(cloudI) + 
                    ", \"clouds\":" + String(clouds);
  }
  if (sqmActive) {
        res +=      ", \"full\":" + String(sqmFull) + 
                    ", \"ir\":" + String(sqmIr) + 
                    ", \"visible\":" + String(sqmVis) + 
                    ", \"lux\":" + String(sqmLux) + 
                    ", \"mpsas\":" + String(sqmMpsas) + 
                    ", \"dmpsas\":" + String(sqmDmpsas); 
  }
  if (veml6075Active) {
        res +=      ", \"uva\":" + String(uva) + 
                    ", \"uvb\":" + String(uvb) + 
                    ", \"UVindex\":" + String(uvi,0);

  }

        res +=      "}}";

  return res;

}

void firstSetup(){
  DebugLn(String(settings.data.magic));
  DebugLn("Setting up AP");
  WiFi.mode(WIFI_AP);             //Only Access point
  WiFi.softAP(ap_ssid, NULL, 8);  //Start HOTspot removing password will disable security
  delay(50);
  setupserver.on("/", handleSetup);
  setupserver.on("/initForm", handleInitForm);
  DebugLn("Server Begin");
  setupserver.begin(); 
  delay(100);
  do {
    setupserver.handleClient(); 
    delay(500);
    Debug(".");
  }
  while (!ap_setup_done);
    
  settings.data.magic[0] = MAGIC[0];
  settings.data.magic[1] = MAGIC[1];
  settings.data.magic[2] = MAGIC[2];
  settings.data.magic[3] = MAGIC[3];
  setupserver.stop();
  WiFi.disconnect();
  settings.Save();
  DebugLn("First Setup Done");
}


int setupSTA()
{
  int timeOut=0;

  for (int retry=0; retry<=3; retry++) {
    WiFi.disconnect();
    WiFi.hostname("ESP-" + String(settings.data.name)) ;
    WiFi.mode(WIFI_STA);
    DebugLn("Connecting to "+String(settings.data.ssid));
    DebugLn("Connecting to "+String(settings.data.psk));
    
    if (String(settings.data.psk).length()) {
      WiFi.begin(String(settings.data.ssid), String(settings.data.psk));
    } else {
      WiFi.begin(String(settings.data.ssid));
    }
    timeOut=0;
    retry=0;    
     while (WiFi.status() != WL_CONNECTED) {
      if (timeOut < 10){          // if not timeout, keep trying
        delay(100);
        Debug(String(WiFi.status()));
        displayBusy();
        timeOut ++;
      } else{
        timeOut = 0;
        DebugLn("-Wifi connection timeout");
        displayBusy();
        retry++;
        if (retry == 3) { 
          return 0;
        }
      }
    }
    DebugLn(" Connected");

  // Print the IP address
    DebugLn(WiFi.localIP()); 
    DebugLn(WiFi.hostname().c_str());
    break; 
    }
    return 1;
}

void setup() {
  DebugStart();
  DebugLn("Start ->");

  myResetInfo = ESP.getResetInfoPtr();
  DebugLn("myResetInfo->reason "+String(myResetInfo->reason)); // reason is uint32
                                                                 // 0 = power down
                                                                 // 6 = reset button
                                                                 // 5 = restart from deepsleep


  //************** load settings **********************************
  settings.Load();

  //************** setup LED and DHT pins, and switch OFF LEDs **************
  DebugLn("Setup ->");
  pinMode(RED_LED, FUNCTION_3);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, FUNCTION_3);
  pinMode(GREEN_LED, OUTPUT);

  // ******** initiallize LEDs and wait for setup pin *****************
  DebugLn("-> Check if SETUP_PIN is low");
  digitalWrite(RED_LED, LEDON);
  digitalWrite(GREEN_LED, LEDON);
  // Wait up to 5s for SETUP_PIN to go low to enter AP/setup mode.
  pinMode(SETUP_PIN, INPUT);      //Configure setup pin as input
  digitalWrite(SETUP_PIN, HIGH);  //Enable internal pooling
  delay(5000);  
  DebugLn("Magic ->"+String(settings.data.magic));

  // if NO MAGIC Or SETUP PIN enter hard config...
  if (String(settings.data.magic) != MAGIC||!digitalRead(SETUP_PIN)){
    digitalWrite(GREEN_LED, LEDOFF);
    digitalWrite(RED_LED, LEDON);
    DebugLn("First Setup ->");
    firstSetup();
  }

  // NO SETUP, switch off both LEDs
  digitalWrite(GREEN_LED, LEDOFF);
  digitalWrite(RED_LED, LEDOFF);

  //********* initialize sensors *****************
  BME280setup();
  INA219setup();
  gy906setup();
  SQMsetup();
  veml6075setup();
  
  // ********** set timezone (do it before WiFi setup) *******************
  configTime(settings.data.timezone, settings.data.ntpserver);

  // *********** setup STA mode and connect to WiFi ************
  if (setupSTA() == 0) { // SetupSTA mode
    DebugLn("Wifi no connected, wait 60 sec. and restart");
    delay(60*1000);
    ESP.restart();
  }

  // ********** initialize OTA *******************
  ArduinoOTA.begin();

  // ********* initialize MQTT ******************
  if (strlen(settings.data.mqttbroker) > 0 ) {
    // MQTT broker defined, initialize MQTT
    mqtt_init();
    delay(100);
  }

  //********** switch ON GREEN LED, initialize web servers and switch OFF LEDs
  digitalWrite(GREEN_LED, LEDON);
  digitalWrite(RED_LED, LEDOFF);
  DebugLn("-> Initiate WebServer");
  server.on("/",handleRowData);
  server.on("/data",handleRowData);
  server.on("/setup", handleRoot);
  server.on("/form", handleForm);
  delay(100);
  server.begin(); 
  delay(3000);
  digitalWrite(GREEN_LED, LEDOFF);
  digitalWrite(RED_LED, LEDOFF);  


  // ********* initialize counters for Rain and WindSpeed
  lastSecond = millis();
  
  // setup wind gauge and anemometer interrupts
  wind_setup();
  rain_setup();

  // read current time
  time(&now);                       // read the current time
  Debug("-> Time sync to ");
  DebugLn(ctime(&now));



  DebugLn("-> End Setup");
}

//When it's midnight, reset the total amount of rain and gusts
void midnightReset()
{

    minutesSinceLastReset = 0; //Zero out the backup midnight reset variable

}


void loop() {

  // handle http client while waiting
  server.handleClient();    
  
  // handle OTA
  ArduinoOTA.handle();

  // handle MQTT
  handleMQTT();

  // weather regular calculations
  // calculate wind speed and wind-dir every 5 seconds
  // calculate averages every minute
  if(millis() - lastSecond >= 5000)
  {
      lastSecond += 5000;

      // get rain rate in 1 hour
      rainrate1hmm = max(rain_rate(), maxrainrate);
      maxrainrate = rainrate1hmm;

      //Get wind speed and direction
      windspeednow = get_wind_speed(); //Wind speed in Km/h
      sumwindspeed += windspeednow;
      maxwindspeed = max(windspeednow, maxwindspeed);
      winddirnow = get_wind_direction();
      winddiravg[numsampleswind] = winddirnow;
      numsampleswind++;

      // 12 samples of 5 secs = 1 minute elapsed
      // calculate wind speed, windgust and windir_avg for last minute
      // also store number of rain gauge clicks along this minute in a whole day (24 h x 60 mins) circular buffer
      if (numsampleswind >= 12)   
      {
        windspeed = (sumwindspeed / (float)numsampleswind);  
        windgust = maxwindspeed;
        sumwindspeed = 0.0;
        maxwindspeed = 0.0;

        //Calc winddir_avg, Wind Direction
        //You can't just take the average. Google "mean of circular quantities" for more info
        //We will use the Mitsuta method because it doesn't require trig functions
        //And because it sounds cool.
        //Based on: http://abelian.org/vlf/bearings.html
        //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
        long sum = winddiravg[0];
        int D = winddiravg[0];
        for(int i = 1 ; i < numsampleswind ; i++)
        {
            int delta = winddiravg[i] - D;

            if(delta < -180)
                D += delta + 360;
            else if(delta > 180)
                D += delta - 360;
            else
                D += delta;

            sum += D;
        }
        winddir = sum / numsampleswind;
        if(winddir >= 360) winddir -= 360;
        if(winddir < 0) winddir += 360;
        Debug("winddir_avg1m: ");
        DebugLn(winddir);

        // store rain clicks on circular buffer
        rain_buffer.push(rainclicks);
        rainclicks=0;

        calcWeather();  // Calculate weather data each minute

        numsampleswind = 0;

        minutes_send++;
        minutesSinceLastReset++; //It's been another minute since last night's midnight reset

      }
  }

  //force a midnigt variables reset on day change
  time(&now);                      // read the current time
  localtime_r(&now, &tm);          // update the structure tm with the current time
  if ( currentday != tm.tm_mday)   // is a day change? (midnight)
  {
      midnight = true; // set this to avoid repeat this group more than once at midnight
      currentday = tm.tm_mday;
      midnightReset(); //Reset a bunch of variables like rain and daily total rain
      DebugLn("Midnight reset");
  }

    if (minutes_send >= settings.data.sendinterval) {
    // Send data to Wunderground
    if(strlen(settings.data.stationID) > 0 && strlen(settings.data.stationKey) > 0 ){
      DebugLn("--- Sending data to Wunderground ---");
      digitalWrite(GREEN_LED, LEDON);
      sendDataWunderground();
      digitalWrite(GREEN_LED, LEDOFF);
    }
    // ********* send data over MQTT ******************
    if (strlen(settings.data.mqttbroker) > 0 ) {
      mqttSendSensorData(prepareJsonData());
    }
       
    maxrainrate = 0;  // reset max rainrate
    minutes_send=0;
  }

 // calculate weather variables on first loop
 if (firstloop) {
  calcWeather();  // Calculate weather data each minute
  firstloop=false;
 }
  
 delay(100);

}
