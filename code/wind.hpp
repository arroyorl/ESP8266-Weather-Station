//We need to keep track of the following variables:
//Wind speed/dir each update (no storage)
//Wind gust/dir over the day (no storage)
//Wind speed/dir, avg over 2 minutes (store 1 per second)
//Wind gust/dir over last 10 minutes (store 1 per minute)

#define WIND_DIR_AVG_SIZE 120
uint8_t windspdavg[WIND_DIR_AVG_SIZE]; //120 bytes to keep track of 2 minute average
int winddiravg[WIND_DIR_AVG_SIZE]; //120 ints to keep track of 2 minute average
float windgust_10m[10]; //10 floats to keep track of largest gust in the last 10 minutes
int windgustdirection_10m[10]; //10 ints to keep track of 10 minute max
long lastWindCheck = 0;

float       windspeednow=0.0; // instantaneous wind speed in Km/h
float       windspeed=0.0;    // last minute average wind speed in Km/h
float       windgust=0.0;     // last minute wind gust in km/h
int         winddirnow;       // [0-360 instantaneous wind direction]
int         winddir;          // [0-360 last minute wind direction]

float       maxwindspeed=0.0; 
float       sumwindspeed=0.0;
int         windiravg[12];

byte        numsampleswind=0;

// volatiles are subject to modification by IRQs
volatile long lastWindIRQ = 0;
volatile uint8_t windClicks = 0;

//Interrupt routine for anemometer (it is called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void IRAM_ATTR wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation)
{
    if (millis() - lastWindIRQ > 15) // Ignore switch-bounce glitches less than 15ms (99.5 MPH max reading) after the reed switch closes
    {
        lastWindIRQ = millis(); //Grab the current time
        windClicks++; //There is 1.492MPH for each click per second.
    }
}

//*********** Annemometer initialization *************
void wind_setup(){
  pinMode(ANNEMOMETER_PIN, FUNCTION_3);
  pinMode(ANNEMOMETER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANNEMOMETER_PIN), wspeedIRQ, FALLING);
}

//Returns the instataneous wind speed
float get_wind_speed()
{
    float deltaTime = millis() - lastWindCheck; // elapsed time

    deltaTime /= 1000.0; //Convert to seconds

    float WSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

    windClicks = 0; //Reset and start watching for new wind
    lastWindCheck = millis();

    // WSpeed *= 1.492 * 1.609344; // 1.492 mph/click * 1.609344 = speed in Km/h
    WSpeed *= 2.4;              // 2.4 Km/h per click/sec

    return(WSpeed);
}

float get_wind_speed_2()
{
    float deltaTime = millis() - lastWindCheck; // elapsed time

    deltaTime /= 1000.0; //Convert to seconds

    float WSpeed = (float)windClicks / deltaTime; // clics/sec

    windClicks = 0; //Reset and start watching for new wind
    lastWindCheck = millis();

    WSpeed *= 0.34 * 3.6; // 0.34 m/s click * 3.6 = speed in Km/h

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

