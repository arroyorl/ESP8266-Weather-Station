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
///////////////////////////////////////////////////////////////
 

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <time.h> 

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define FVERSION  "v2.6"

///////////////////////////////////////////////////
// Comment for final release
//#define RDEBUG
//#define WINDIRDEBUG
///////////////////////////////////////////////////

#define SENSORTYPE BME280
#define SENSORNAME "BME280"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "Rdebug.h"
#include "settings.h"
#include "mainPage.h"
#include "initpage.h"

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

#define WINDIR  A0
#define RAIN_PIN GPIO13 // D7 
#define ANNEMOMETER_PIN GPIO14 // D5

#define   NUM_SAMPLES 5

// Define Red and green LEDs
#ifdef ARDUINO_ESP8266_NODEMCU
# define RED_LED GPIO12 // D6
# define GREEN_LED GPIO10 // D12 - SD3
#endif
#ifdef ARDUINO_ESP8266_GENERIC
# define RED_LED GPIO12 //    antes GPIO9
# define GREEN_LED GPIO10
#endif

ESP8266WebServer setupserver(80);
ESP8266WebServer server(80);

//Struct to store setting in EEPROM
Settings    settings;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
String      data;
String      httpUpdateResponse;
const char* ap_ssid = "ESP-TEMP";
const char* ap_password = ""; //"12345678";
int         ap_setup_done = 0;
float       temperature=0;
float       humidity=0;
float       dewpoint=0;
float       pressure=0;
float       sealevelpressure=0;
float       altitudecalc=0;
float       rainmm=0;
float       rain24mm=0;
long        time_offset=0;
bool        firstloop=true;

rst_info *myResetInfo;

// global variables for time
time_t      now;                    // this are the seconds since Epoch (1970) - UTC
tm          tm;                     // the structure tm holds time information in a more convenient way
char        hhmm[6];                // variable to store time in format HH:MM

float       windspeed=0;
float       windgust=0;

long lastSecond; //The millis counter to see when a second rolls by
unsigned int minutesSinceLastReset; //Used to reset variables after 24 hours. Imp should tell us when it's midnight, this is backup.
uint8_t seconds; //When it hits 60, increase the current minute
uint8_t seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
uint8_t minutes; //Keeps track of where we are in various arrays of data
uint8_t minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data
int  minutes_send = 0; // Keeps track of when send data to Wundergound
bool midnight = false;  // for reset variable at midnight only one time
uint8_t currentday;  // to store current day and reset data at midnight when day changes
long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile uint8_t windClicks = 0;

//We need to keep track of the following variables:
//Wind speed/dir each update (no storage)
//Wind gust/dir over the day (no storage)
//Wind speed/dir, avg over 2 minutes (store 1 per second)
//Wind gust/dir over last 10 minutes (store 1 per minute)
//Rain over the past hour (store 1 per minute)
//Total rain over date (store one per day)

#define WIND_DIR_AVG_SIZE 120
uint8_t windspdavg[WIND_DIR_AVG_SIZE]; //120 bytes to keep track of 2 minute average
int winddiravg[WIND_DIR_AVG_SIZE]; //120 ints to keep track of 2 minute average
float windgust_10m[10]; //10 floats to keep track of largest gust in the last 10 minutes
int windgustdirection_10m[10]; //10 ints to keep track of 10 minute max
volatile float rainHour[60]; //60 floating numbers to keep track of 60 minutes of rain

//These are all the weather values that wunderground expects:
int winddir; // [0-360 instantaneous wind direction]
float windspeedmph; // [mph instantaneous wind speed]
float windgust1min; // [mph wind gust during current minute]
float windgustmph; // [mph current wind gust, using software specific time period]
int windgustdir; // [0-360 using software specific time period]
float windspdmph_avg2m; // [mph 2 minute average wind speed mph]
int winddir_avg2m; // [0-360 2 minute average wind direction]
float windgustmph_10m; // [mph past 10 minutes wind gust mph ]
int windgustdir_10m; // [0-360 past 10 minutes wind gust direction]
float tempf; // [temperature F]
float dewpointf; // devpoint in F
float rainin; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
volatile float dailyrainin; // [rain inches so far today in local time]
float baromin;  // barometric pressure in inches

// volatiles are subject to modification by IRQs
volatile unsigned long raintime, rainlast, raininterval, rain;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Adafruit_BME280 bme; 

void displayBusy(){
  digitalWrite(RED_LED, LEDON);
  delay(500);
  digitalWrite(RED_LED, LEDOFF);
  delay(500);
}

// reference: http://wahiduddin.net/calc/density_algorithms.htm
float computeDewPoint(float celsius, float humidity)
{
        double RATIO = 373.15 / (273.15 + celsius);  // RATIO was originally named A0, possibly confusing in Arduino context
        double SUM = -7.90298 * (RATIO - 1);
        SUM += 5.02808 * log10(RATIO);
        SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
        SUM += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
        SUM += log10(1013.246);
        double VP = pow(10, SUM - 3) * humidity;
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558 - T);
}

void getBME280data() {

float runtemp=0;
float runhumid=0;
float runpress=0;

  DebugLn("get BME280 data");

  //////////////////////////////// BME280 ////////////////////////////////////////////////
  for(int i = 0 ; i < NUM_SAMPLES; i++){
    runtemp += bme.readTemperature();
    runhumid += bme.readHumidity();
    runpress += bme.readPressure();
  }

  temperature = (runtemp / NUM_SAMPLES) + settings.data.tempadjust;
  tempf = (temperature * 9.0) / 5.0 + 32.0;
  humidity = runhumid / NUM_SAMPLES;
  dewpoint = computeDewPoint(temperature, humidity);
  dewpointf = (dewpoint * 9.0) / 5.0 + 32.0;
  pressure = (runpress / NUM_SAMPLES) / 100.0F;
  sealevelpressure = bme.seaLevelForAltitude(settings.data.altitude, pressure);
  baromin = sealevelpressure * 29.92 / 1013.25;
  DebugLn("BME280 data read !");
  ////////////////////////////////////////////////////////////////////////////////

   if (isnan(humidity) || isnan(temperature) || isnan(pressure)) {
      DebugLn("Failed to read from sensor!");
   }

  
  DebugLn("Temp: "+String(temperature));
  DebugLn("Hum: "+String(humidity));
  DebugLn("DewPoint: "+String(dewpoint));
  DebugLn("Pres: " + String(pressure));
  DebugLn("SeaLevelPres: "+String(sealevelpressure));
}

//Interrupt routines (these are called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void IRAM_ATTR rainIRQ()
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge
{
    raintime = millis(); // grab current time
    raininterval = raintime - rainlast; // calculate interval between this and last event

    if (raininterval > 10) // ignore switch-bounce glitches less than 10mS after initial edge
    {
        dailyrainin += 0.011; //Each dump is 0.011" of water
        rainHour[minutes] += 0.011; //Increase this minute's amount of rain

        rainlast = raintime; // set up for next event
    }
}

void IRAM_ATTR wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation)
{
    if (millis() - lastWindIRQ > 15) // Ignore switch-bounce glitches less than 15ms (99.5 MPH max reading) after the reed switch closes
    {
        lastWindIRQ = millis(); //Grab the current time
        windClicks++; //There is 1.492MPH for each click per second.
    }
}


//Returns the instataneous wind speed
float get_wind_speed()
{
    float deltaTime = millis() - lastWindCheck; // elapsed time

    deltaTime /= 1000.0; //Convert to seconds

    float WSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

    windClicks = 0; //Reset and start watching for new wind
    lastWindCheck = millis();

    WSpeed *= 1.492; //4 * 1.492 = 5.968MPH

    return(WSpeed);
}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
    unsigned int numberOfReadings = 8;
    unsigned int runningValue = 0;

    for(int x = 0 ; x < numberOfReadings ; x++)
        runningValue += analogRead(pinToRead);
    runningValue /= numberOfReadings;

    return(runningValue);
}

int get_wind_direction()
// read the wind direction sensor, return heading in degrees
{
    unsigned int adc;

    adc = averageAnalogRead(WINDIR); // get the current reading from the sensor

#ifdef WINDIRDEBUG
    Debug("adc: ");
    DebugLn(adc);
#endif  

    // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
    // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
    // Note that these are not in compass degree order! See Weather Meters datasheet for more information.

    if (adc < 89) return (113);   // ESE
    if (adc < 106) return (68);    // ENE
    if (adc < 133) return (90);   // E
    if (adc < 180) return (158);  // SSE
    if (adc < 241) return (135);  // SE
    if (adc < 294) return (203);  // SSW
    if (adc < 377) return (180);  // S
    if (adc < 468) return (23);   // NNE
    if (adc < 561) return (45);   // NE
    if (adc < 648) return (248);  // WSW
    if (adc < 702) return (225);  // SW
    if (adc < 781) return (338);  // NNW
    if (adc < 845) return (0);    // N
    if (adc < 892) return (293);  // WNW
    if (adc < 947) return (315);  // NW
    if (adc < 1023) return (270); // W
    return (-1); // error, disconnected?
}

String WindDirString(int wdir) {
  if (wdir < 11) return("N");
  if (wdir < 34) return("NNE");
  if (wdir < 56) return("NE");
  if (wdir < 78) return("ENE");
  if (wdir < 101) return("E");
  if (wdir < 124) return("ESE");
  if (wdir < 146) return("SE");
  if (wdir < 168) return("SSE");
  if (wdir < 191) return("S");
  if (wdir < 214) return("SSW");
  if (wdir < 236) return("SW");
  if (wdir < 259) return("WSW");
  if (wdir < 281) return("W");
  if (wdir < 304) return("WNW");
  if (wdir < 326) return("NW");
  if (wdir < 349) return("NNW");
  return ("N");

}

//Calculates each of the variables that wunderground is expecting
void calcWeather()
{
    //current winddir, current windspeed, windgustmph, and windgustdir are calculated every 100ms throughout the day

    //Calc windspdmph_avg2m
    float temp = 0;
    for(int i = 0 ; i < 120 ; i++)
        temp += windspdavg[i];
    temp /= 120.0;
    windspdmph_avg2m = temp;
//    windspeed = windspdmph_avg2m * 1.609344;  //Wind speed in Km/h
    windspeed = windspeedmph * 1.609344;  //Wind speed in Km/h
    windgustmph = windgust1min; // wind gust in last minute
    windgust = windgustmph * 1.609344; // wind gust in Km/h
    Debug("windspeed: ");
    DebugLn(windspeed);
    Debug("winddir: ");
    DebugLn(winddir);

    //Calc winddir_avg2m, Wind Direction
    //You can't just take the average. Google "mean of circular quantities" for more info
    //We will use the Mitsuta method because it doesn't require trig functions
    //And because it sounds cool.
    //Based on: http://abelian.org/vlf/bearings.html
    //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
    long sum = winddiravg[0];
    int D = winddiravg[0];
    for(int i = 1 ; i < WIND_DIR_AVG_SIZE ; i++)
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
    winddir_avg2m = sum / WIND_DIR_AVG_SIZE;
    if(winddir_avg2m >= 360) winddir_avg2m -= 360;
    if(winddir_avg2m < 0) winddir_avg2m += 360;
    Debug("winddir_avg2m: ");
    DebugLn(winddir_avg2m);

    //Calc windgustmph_10m
    //Calc windgustdir_10m
    //Find the largest windgust in the last 10 minutes
    windgustmph_10m = 0;
    windgustdir_10m = 0;
    //Step through the 10 minutes
    for(int i = 0; i < 10 ; i++)
    {
        if(windgust_10m[i] > windgustmph_10m)
        {
            windgustmph_10m = windgust_10m[i];
            windgustdir_10m = windgustdirection_10m[i];
        }
    }
    Debug("windgust: ");
    DebugLn(windgust);

    // Get Temperature, humidity and pressure
    getBME280data();

    //Total rainfall for the day is calculated within the interrupt
    //Calculate amount of rainfall for the last 60 minutes
    rainin = 0;
    for(int i = 0 ; i < 60 ; i++)
        rainin += rainHour[i];
    //data in mm of rain
    rainmm = rainin * 25.4;
    rain24mm = dailyrainin * 25.4;
    Debug("rain 1h: ");
    Debug(rainmm);
    Debug(", rain 24h: ");
    DebugLn(rain24mm);
    
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


void handleSetup() {
  DebugLn("handlesetup");
  String s = FPSTR(INIT_page);
  s.replace("@@SSID@@", settings.data.ssid);
  s.replace("@@PSK@@", settings.data.psk);
  s.replace("@@CLOCKNAME@@", settings.data.name);
  s.replace("@@VERSION@@",FVERSION);
  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  httpUpdateResponse = "";
  setupserver.send(200, "text/html", s);
}

void handleInitForm() {
  DebugLn("handleInitForm");
  DebugLn("mode "+String(WiFi.status()));

  String t_ssid = setupserver.arg("ssid");
  String t_psk = setupserver.arg("psk");
  String t_name = setupserver.arg("clockname");
  String(t_name).replace("+", " ");
  t_ssid.toCharArray(settings.data.ssid,SSID_LENGTH);
  t_psk.toCharArray(settings.data.psk,PSK_LENGTH);
  t_name.toCharArray(settings.data.name,NAME_LENGTH);
  httpUpdateResponse = "The configuration was updated.";
  setupserver.sendHeader("Location", "/");
  setupserver.send(302, "text/plain", "Moved");
  settings.Save();
  
  ap_setup_done = 1;
}

void handleRoot() {
  DebugLn("handleRoot");

  String s = FPSTR(MAIN_page);
  s.replace("@@SSID@@", settings.data.ssid);
  s.replace("@@PSK@@", settings.data.psk);
  s.replace("@@CLOCKNAME@@", settings.data.name);
  s.replace("@@VERSION@@",FVERSION);
  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  s.replace("@@STATIONID@@",settings.data.stationID);
  s.replace("@@STATIONKEY@@",settings.data.stationKey);
  s.replace("@@SENDINT@@",String(settings.data.sendinterval));
  s.replace("@@TADJUST@@",String(settings.data.tempadjust));
  s.replace("@@ALTITUDE@@",String(settings.data.altitude));
  s.replace("@@NTPSERVER@@",settings.data.ntpserver);
  s.replace("@@TIMEZONE@@",settings.data.timezone);
  
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  strftime(hhmm,6,"%H:%M",&tm);     // format time (HH:MM)
  s.replace("@@TIMENOW@@",String(hhmm));

  httpUpdateResponse = "";
  server.send(200, "text/html", s);
}

void handleRowData() {
  DebugLn("handleRowData");

//  calcWeather();
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time

  httpUpdateResponse = "";
  server.send(200, "text/html", 
                   "{\"time\":" + String(now) + 
                   ",\"sensors\":" +
                        "{\"temperature\":" + String(temperature) + 
                        ", \"humidity\":" + String(humidity) + 
                        ", \"dewpoint\":" + String(dewpoint) +
                        ", \"pressure\":" + String(pressure) + 
                        ", \"sealevelpressure\":" + String(sealevelpressure) + 
                        ", \"windspeed\":" + String(windspeed) +
                        ", \"windgust\":" + String(windgust) +
                        ", \"winddirdeg\": \"" + String(winddir) + "\""
                        ", \"winddir\": \"" + WindDirString(winddir) + "\""
                        ", \"rain1h\":" + String(rainmm) +
                        ", \"rain24h\":" + String(rain24mm) +
                        "}}\r\n"
                   );

}

void sendHTTP (String httpPost){
  WiFiClient client;
  HTTPClient http;
  DebugLn(httpPost);
  http.begin(client, httpPost);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("");
  DebugLn(httpCode);    
  http.end();
}

void sendHTTPGet (String httpGet){
  WiFiClient client;
  HTTPClient http;
  DebugLn(httpGet);
  http.begin(client, httpGet);
  int httpCode = http.GET();
  DebugLn(httpCode);    
  http.end();
}

void sendHTTPsGet (String httpGet){
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  // fingerprint not needed
  client->setInsecure();
  // send data
  DebugLn("HTTPsGet: " + httpGet);
  https.begin(*client, httpGet);
  // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = https.GET();
  String payload = https.getString();

  DebugLn("HTTPsGet return code: " + String(httpCode));    // this return 200 when success
  DebugLn(payload);     // this will get the response
  https.end();

}

void handleForm() {
  DebugLn("handleForm");

  DebugLn("mode "+String(WiFi.status()));
  String update_wifi = server.arg("update_wifi");
  DebugLn("update_wifi: " + update_wifi );
  String t_ssid = server.arg("ssid");
  String t_psk = server.arg("psk");
  String t_name = server.arg("clockname");
  String(t_name).replace("+", " ");
  if (update_wifi == "1") {
    t_ssid.toCharArray(settings.data.ssid,SSID_LENGTH);
    t_psk.toCharArray(settings.data.psk,PSK_LENGTH);
    t_name.toCharArray(settings.data.name,NAME_LENGTH);
  }

  String aux;
  aux = server.arg("stationid");
  aux.toCharArray(settings.data.stationID,128);
  aux = server.arg("stationkey");
  aux.toCharArray(settings.data.stationKey,128);

  String sint = server.arg("sendint");
  if (sint.length()) {
    settings.data.sendinterval = sint.toInt();
    settings.data.sendinterval=max(1,settings.data.sendinterval);
    settings.data.sendinterval=min(settings.data.sendinterval,60);
  }
  
  String tadj = server.arg("tempadjust");
  if (tadj.length()) {
    settings.data.tempadjust = tadj.toFloat();
    settings.data.tempadjust=max(float(-5),settings.data.tempadjust);
    settings.data.tempadjust=min(settings.data.tempadjust,float(5));
  }
  String altid = server.arg("altitude");
  if (altid.length()) {
    settings.data.altitude = altid.toFloat();
    settings.data.altitude=max(float(0),settings.data.altitude);
    settings.data.altitude=min(settings.data.altitude,float(8000));
  }

  aux = server.arg("ntpserver");
  aux.toCharArray(settings.data.ntpserver,128);
  aux = server.arg("timezone");
  aux.toCharArray(settings.data.timezone,128);

  configTime(settings.data.timezone, settings.data.ntpserver);
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  strftime(hhmm,6,"%H:%M",&tm);     // format time (HH:MM)

  Debug("-> Time sync to ");
  DebugLn(ctime(&now));

  httpUpdateResponse = "The configuration was updated.";
  ap_setup_done = 1;
  server.sendHeader("Location", "/setup");
  server.send(302, "text/plain", "Moved");
  settings.Save();
  
  if (update_wifi != "") {
    delay(500);
    setupSTA();             // connect to Wifi
  }
}

int setupSTA()
{
  int timeOut=0;

  for (int retry=0; retry<=3; retry++) {
    WiFi.disconnect();
    WiFi.hostname("ESP_" + String(settings.data.name)) ;
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
//        break;        
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

void sendDataWunderground(){

  String  weatherData =  "https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?";
  weatherData += "ID=" + String(settings.data.stationID);
  weatherData += "&PASSWORD=" + String(settings.data.stationKey);
  weatherData += "&dateutc=now";
  weatherData += "&action=updateraw";
  weatherData += "&tempf=" + String(tempf);
  weatherData += "&humidity=" + String(humidity);
  weatherData += "&dewptf=" + String(dewpointf);
  weatherData += "&baromin=" + String(baromin);
  weatherData += "&winddir=" + String(winddir);
  weatherData += "&windspeedmph=" + String(windspeedmph);
  weatherData += "&windgustmph=" + String(windgustmph);
  weatherData += "&windgustdir=" + String(windgustdir);
  weatherData += "&windspdmph_avg2m=" + String(windspdmph_avg2m);
  weatherData += "&winddir_avg2m=" + String(winddir_avg2m);
  weatherData += "&windgustmph_10m=" + String(windgustmph_10m);
  weatherData += "&windgustdir_10m=" + String(windgustdir_10m);
  weatherData += "&rainin=" + String(rainin);
  weatherData += "&dailyrainin=" + String (dailyrainin);

  // send to Wunderground
  sendHTTPsGet(weatherData);
              
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
  // BME280, SDA=4 (D2), SCL=5 (D1)
  bme.begin(0x76);          
  DebugLn("-> BME begin");
  
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
  seconds = 0;
  lastSecond = millis();
    
  //*********** Annemometer initialization *************
  pinMode(ANNEMOMETER_PIN, FUNCTION_3);
  pinMode(ANNEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANNEMOMETER_PIN), wspeedIRQ, FALLING);

  //*********** Rain gauge initialization *************
  pinMode(RAIN_PIN, FUNCTION_3);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainIRQ, FALLING);

  // read current time
  time(&now);                       // read the current time
  Debug("-> Time sync to ");
  DebugLn(ctime(&now));



  DebugLn("-> End Setup");
}

//When it's midnight, reset the total amount of rain and gusts
void midnightReset()
{
    dailyrainin = 0; //Reset daily amount of rain

/* No needed
    windgustmph = 0; //Zero out the windgust for the day
    windgustdir = 0; //Zero out the gust direction for the day
*/

    minutes = 0; //Reset minute tracker
    seconds = 0;
    lastSecond = millis(); //Reset variable used to track minutes

    minutesSinceLastReset = 0; //Zero out the backup midnight reset variable
}


void loop() {

  // handle http client while waiting
  server.handleClient();    
  
  // handle OTA
  ArduinoOTA.handle();
  
  //Keep track of which minute it is
  if(millis() - lastSecond >= 1000)
  {
      lastSecond += 1000;

      //Take a speed and direction reading every second for 2 minute average
      if(++seconds_2m > 119) seconds_2m = 0;

      //Calc the wind speed and direction every second for 120 second to get 2 minute average
      windspeedmph = get_wind_speed();
      winddir = get_wind_direction();


      //Check to see if this is a gust for the minute
      if(windspeedmph > windgust1min ) {
        windgust1min = windspeedmph;
        windgustdir = winddir;
      }

      windspdavg[seconds_2m] = (int)windspeedmph;
      winddiravg[seconds_2m] = winddir;

      if(windspeedmph > windgust_10m[minutes_10m])
      {
        windgust_10m[minutes_10m] = windspeedmph;
        windgustdirection_10m[minutes_10m] = winddir;
      }
   
      //If we roll over 60 seconds then update the arrays for rain and windgust
      if(++seconds > 59)
      {
          seconds = 0;
          midnight = false; // reset midnight switch

          calcWeather();  // Calculate weather data each minute

          if(++minutes > 59) minutes = 0;
          if(++minutes_10m > 9) minutes_10m = 0;
          minutes_send++;
          rainHour[minutes] = 0; //Zero out this minute's rainfall amount
          windgust1min = 0; // Zero out this minute's windgust
          windgust_10m[minutes_10m] = 0; //Zero out this minute's gust

          minutesSinceLastReset++; //It's been another minute since last night's midnight reset
      }

  }


  //force a midnigt reset on day change
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
//      calcWeather();
      digitalWrite(GREEN_LED, LEDON);
      sendDataWunderground();
      digitalWrite(GREEN_LED, LEDOFF);
    }
       
    minutes_send=0;
  }

 // calculate weather variables on first loop
 if (firstloop) {
  calcWeather();  // Calculate weather data each minute
  firstloop=false;
 }
  
 delay(100);

}
