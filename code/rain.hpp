/************************************************************
  rain gauge control

  v1.0 initial version

  v2.0 corrected rain rate calculation

************************************************************/

#define DUMP_VOLUME  0.2794            // Each dump is 0.011" of water = 0.011 * 25.4 = 0.2794 mm
#define RAIN15INTERVAL (15*60*1000)    // 15 mins

// volatiles are subject to modification by IRQs
volatile unsigned int rainclicks=0;  // number of rain gauge clicks
volatile unsigned int rainclicksrate=0; // number of rain gauge clicks for rain rate calculation
volatile unsigned long raintime, rainlast, rainprev, rainact, raininterval;
volatile bool firstclick=true;

//Interrupt routine for rain gauge (it is called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void IRAM_ATTR rainIRQ()
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge
{
    raintime = millis(); // grab current time

    if (raintime - rainlast > 200) // ignore switch-bounce glitches less than 200mS after initial edge
    {
        rainclicks++;
        rainclicksrate++;
        rainprev = rainact;
        rainact = raintime;
        rainlast = raintime;                // set up for next event
    }
}

//*********** Rain gauge initialization *************
void rain_setup(){
  pinMode(RAIN_PIN, FUNCTION_3);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  rainclicks=0;
  rainclicksrate=0;
  rainprev = rainlast = rainact = millis();
  firstclick=true;
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainIRQ, FALLING);
}

//***************** calculate rain rate / 1 hour *******************
/*
* this function must be call very frequetly (every 5 seconds) as assume only 1 rain click occurs 
* during the intervall between calls
*
* calculation based on Davis weather stations rain rate calculation (see below)
*
* "Under normal conditions, rain rate data is sent with a nominal interval of 10 to 12 seconds. 
*  Every time a rain tip or click occurs, a new rain rate value is computed (from the timer values) 
*  and the rate timers are reset to zero. Rain rate is calculated based on the time between successive 
*  tips of the rain collector. The rain rate value is the highest rate since the last transmitted rain rate data packet. 
*  (Under most conditions, however, a rain tip will not occur every 10 to 12 seconds.)
*
*  If there have been no rain tips since the last rain rate data transmission, then the rain rate based on the time since that last tip is indicated. 
*  This results in slowly decaying rate values as a rain storm ends, instead of showing a rain rate which abruptly drops to zero. 
*  This results in a more realistic representation of the actual rain event.
*
*  If this time exceeds roughly 15 minutes, then the rain rate value is reset to zero. 
*  This period of time was chosen because 15 minutes is defined by the U.S. National Weather Service as intervening time 
*  upon which one rain "event" is considered separate from another rain "event". 
*  This is also the shortest period of time that the Umbrella will be seen on the display console after the onset of rain."
*
*   |             |               |             |               |
*   |             |--->(0.2794mm) |             |               |
*   |             |               |-------------|-->(t-prev)    |
*   |             |               |             |---------------|-->(t-prev)
*   |             |               |             |               |..............>(t-act)
*   |t-1          |t0             |t1           |t2             |t3
*
*                 act=t0          act=t1        act=t2          act=t3  
*                 prev=t0         prev=t0       prev=t1         prev=t2
*/
float rain_rate() {
float rainrate=0;

  raininterval = millis() - rainprev; // calculate interval between this and last event

/*
  Debug("rainact: ");
  Debug(rainact);
  Debug(", rainprev: ");
  Debug(rainprev);
  Debug(", millis(): ");
  Debug(millis());
  Debug(", raininterval: ");
  Debug(raininterval);
  Debug(", rainclicksrate: ");
  Debug(rainclicksrate);
  Debug(", firstclick: ");
  Debug(firstclick);
*/

  if (rainclicksrate == 1) {
      // first click on interval
      rainrate = DUMP_VOLUME;
      if (firstclick) {
        rainprev = rainact = millis();  // start interval
        firstclick = false;
      }
  }
  else {
    // calculate rainrate
    if (raininterval == 0 || rainclicksrate == 0) {
      // if interval is zero or are no clicks, define rainrate = 0
      rainrate = 0.0;
    }
    else {
      // dumps after first one
      rainrate = DUMP_VOLUME / raininterval * (60 * 60* 1000); // calculate rain rate in 1 hour
      // rainact = rainlast;   // start interval
    }

  }

  // check is elapsed since last click is > 15 mins
  if ((millis() - rainact) > RAIN15INTERVAL) {
    rainclicksrate = 0;   // reset clicks
    firstclick = true;    // reset first click
  }

/*
Debug(", rainrate: ");
DebugLn(String(rainrate,2));
*/

  return rainrate;
}

