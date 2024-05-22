/*************************************************
          TSL 2591 luminosity sensor + SQM
          based on standard Adafruit_SQM2591 library and
          added code for SQM calculation from 
          SQM_tsl2591 library by by Gabe Shaughnessy

          V2.0 replaced SQM_TSL2591 library by Adafruit_TSL2591
          and made calculations in this module

          v2.1 solved issue when sensor is saturated

          v2.2 used mpsas calculation formula from 
          http://www.unihedron.com/projects/darksky/magconv.php
          if full == ir, consider as full and decrease sensibility
          
*************************************************/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>

Adafruit_TSL2591 sqm = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)

unsigned int    sqmFull;
unsigned int    sqmIr;
unsigned int    sqmVis;
float           sqmMpsas;
float           sqmDmpsas; 
float           sqmLux;
bool            sqmActive = false;

int             numTries = 0;

#define   SQM_NUM_SAMPLES 16

int  SQMgetGainValue(tsl2591Gain_t gain) {
int res;

  switch(gain)
  {
    case TSL2591_GAIN_LOW:
      res = 1;
      break;
    case TSL2591_GAIN_MED:
      res = 25;
      break;
    case TSL2591_GAIN_HIGH:
      res = 428;
      break;
    case TSL2591_GAIN_MAX:
      res = 9876;
      break;
    default:
      res = 0;
  }
  return res;

}

int SQMgetIntegrationValue(tsl2591IntegrationTime_t intTime){

  return ((intTime + 1) * 100);

}

void SQMdisplaySensorDetails(void)
{
  sensor_t sensor;
  sqm.getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  DebugLn("Sensor:       " + String(sensor.name));
  DebugLn("Driver Ver:   " + String(sensor.version));
  DebugLn("Unique ID:    " + String(sensor.sensor_id));

  tsl2591Gain_t gain = sqm.getGain();
  String gainS;
  switch(gain)
  {
    case TSL2591_GAIN_LOW:
      gainS = "Gain:         1x (Low)";
      break;
    case TSL2591_GAIN_MED:
      gainS = "Gain:         25x (Medium)";
      break;
    case TSL2591_GAIN_HIGH:
      gainS = "Gain:         428x (High)";
      break;
    case TSL2591_GAIN_MAX:
      gainS = "Gain:         9876x (Max)";
      break;
  }
  DebugLn(gainS);

  DebugLn("Timing:       " + String((sqm.getTiming() + 1) * 100, DEC) + "ms"); 
  DebugLn("Max Value:    " + String(sensor.max_value) + " lux");
  DebugLn("Min Value:    " + String(sensor.min_value) + " lux");
  DebugLn("Resolution:   " + String(sensor.resolution,4) + " lux");  

}

void SQMbumpGain(int bumpDirection) {

  tsl2591Gain_t gain = sqm.getGain();
  switch (gain) {
  case TSL2591_GAIN_LOW:
    if (bumpDirection > 0) {
      gain = TSL2591_GAIN_MED;
    } else {
      gain = TSL2591_GAIN_LOW;
    }
    break;
  case TSL2591_GAIN_MED:
    if (bumpDirection > 0) {
      gain = TSL2591_GAIN_HIGH;
    } else {
      gain = TSL2591_GAIN_LOW;
    }
    break;
  case TSL2591_GAIN_HIGH:
    if (bumpDirection > 0) {
      gain = TSL2591_GAIN_MAX;
    } else {
      gain = TSL2591_GAIN_MED;
    }
    break;
  case TSL2591_GAIN_MAX:
    if (bumpDirection > 0) {
      gain = TSL2591_GAIN_MAX;
    } else {
      gain = TSL2591_GAIN_HIGH;
    }
    break;
  default:
    break;
  }
  sqm.setGain(gain);
  DebugLn("SQM: new gain: " + String(SQMgetGainValue(gain)) + "x");
}


void SQMbumpTime(int bumpDirection) {

  tsl2591IntegrationTime_t intTime = sqm.getTiming();

  switch (intTime) {
  case TSL2591_INTEGRATIONTIME_100MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_200MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_100MS;
    }
    break;
  case TSL2591_INTEGRATIONTIME_200MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_300MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_100MS;
    }
    break;
  case TSL2591_INTEGRATIONTIME_300MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_400MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_200MS;
    }
    break;
  case TSL2591_INTEGRATIONTIME_400MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_500MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_300MS;
    }
    break;
  case TSL2591_INTEGRATIONTIME_500MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_600MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_400MS;
    }
    break;
  case TSL2591_INTEGRATIONTIME_600MS:
    if (bumpDirection > 0) {
      intTime = TSL2591_INTEGRATIONTIME_600MS;
    } else {
      intTime = TSL2591_INTEGRATIONTIME_500MS;
    }
    break;
  default:
    break;
  }
  sqm.setTiming(intTime);
  DebugLn("SQM: new integration time: " + String(SQMgetIntegrationValue(intTime)) + " ms.");

}

bool SQMsetup()
{
  if(!sqm.begin())
  {
    // TSL2591 not detected
    DebugLn("No TSL2591 detected ... Check wiring or I2C ADDR !");
    sqmActive = false;
    return false;
  }

  // set initial parameters
    sqm.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
    sqm.setTiming(TSL2591_INTEGRATIONTIME_200MS);
 
  // Display sensor details
  SQMdisplaySensorDetails();
  delay(500);

  /* Update these values depending on what you've set above! */  
  DebugLn ("***** Initialized SQM-TSL2591 *****");

  sqmActive = true;
  return true;
}

void SQMtakeReading(){

  if (!sqmActive) return;  // do nothing if sensor is not active (not found)

  uint32_t lum = sqm.getFullLuminosity();
  uint16_t ir, vis, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  if ((float)full < (float)ir) {
    DebugLn("SQM: full < ir!  Rechecking...");
    numTries++;
    if(numTries > 50) {
      DebugLn("SQM: too many times full < ir!");
      return; // do nothing
    }
    delay(50);
    SQMtakeReading(); // try again !
    return;
  }  
  vis = full - ir;

  tsl2591IntegrationTime_t intTime = sqm.getTiming();
  tsl2591Gain_t gain = sqm.getGain();
  DebugLn("SQM: gain: 0x" + String(gain, HEX) + ", integration: " + String(intTime) + ", full: " + String(full) + ", vis: " + String(vis));


  if (full < 128) {
    // intensity is low, increase gain and/or integration time
    if (gain != TSL2591_GAIN_MAX) {
      // increase gain
        DebugLn("SQM: Bumping gain up");
        SQMbumpGain(1);
        SQMtakeReading();
        return;
    }
    else {
      // gain is maximum, increase integration time
      if (intTime != TSL2591_INTEGRATIONTIME_600MS) {
        DebugLn("SQM: Bumping integration up");
        SQMbumpTime(1);
        SQMtakeReading();
        return;
      } 
    }
  } else if (full == 0xFFFF || ir == 0xFFFF || full == ir) {
    // saturated, decrease gain and/or intensity
    if (intTime != TSL2591_INTEGRATIONTIME_100MS) {
      // decrease integration time
        DebugLn("SQM: Bumping integration down");
        SQMbumpTime(-1);
        SQMtakeReading();
        return;
    } else {
      // time at minimum, decrease gain
      if (gain != TSL2591_GAIN_LOW ) {
        DebugLn("SQM: Bumping gain down");
        SQMbumpGain(-1);
        SQMtakeReading();
        return;
      }
    }
  }

  numTries = 0; // reset # nested calls to function

  // Do iterative sampling to gain statistical certainty
  unsigned long fullCumulative = 0;
  unsigned long visCumulative = 0;
  unsigned long irCumulative = 0;
  int niter = 0;

  intTime = sqm.getTiming();
  gain = sqm.getGain();

  // DebugLn("SQM: gain: 0x" + String(gain, HEX) + ", integration: " + String(intTime));

  while (fullCumulative < (1 << 20)) { // maximum value 2 ^ 20
    lum = sqm.getFullLuminosity();
    ir = lum >> 16;
    full = lum & 0xFFFF;
    if ((float)full >= (float)ir) {
      // valid reading, cumulate 
      niter++;
      fullCumulative += full;
      irCumulative += ir;
      visCumulative += full - ir;
    }
    if (niter >= SQM_NUM_SAMPLES) {
      break;
    }

  }

  float gainValue = SQMgetGainValue(gain);
  float integrationValue = SQMgetIntegrationValue(intTime);

  float factor = (gainValue * integrationValue / 100.F); 
  DebugLn("SQM: niter: " + String(niter) + ", gain:" + String(gainValue) + ", integration time: " + String(integrationValue) + ", factor: " + String(factor));
  sqmFull = (float)fullCumulative / (float)niter; 
  sqmIr = (float)irCumulative / (float)niter;
  float VIS = (float)visCumulative / (factor * niter);
  sqmVis = (float)visCumulative / (float)niter;
  sqmLux = sqm.calculateLux(sqmFull, sqmIr);
//   sqmMpsas = 12.6 - 1.086 * log(VIS);  // original formula from https://github.com/gshau/SQM_TSL2591/tree/master 
  sqmMpsas = log10(sqmLux/108000.F) / (-0.4); // updated formula from http://www.unihedron.com/projects/darksky/magconv.php
  sqmDmpsas = 1.086 / sqrt(visCumulative);

}

void getSQMdata() {

  if (sqmActive) {
/*    uint32_t lum = sqm.getFullLuminosity();
    sqmIr = (lum >> 16);
    sqmFull = (lum & 0xFFFF);
    sqmVis = (sqmFull -sqmIr);
    sqmLux = sqm.calculateLux(sqmFull, sqmIr);
*/
    SQMtakeReading();
    Debug ("Reading TSL2591 data: ");
    Debug("full: " + String(sqmFull));
    Debug(", ir: " + String(sqmIr));
    Debug(", vis: " + String(sqmVis));
    Debug(", lux: " + String(sqmLux));
    DebugLn(", mpsas: " + String(sqmMpsas) + " +/- " + String(sqmDmpsas));
  }

}

String SQMJson()
{

  String jsonMsg =  String("\"SQM\": ") + 
                      "{ \"full\":" + String(sqmFull) + 
                      ", \"ir\":" + String(sqmIr) + 
                      ", \"visible\":" + String(sqmVis) + 
                      ", \"lux\":" + String(sqmLux) + 
                      ", \"mpsas\":" + String(sqmMpsas) + 
                      ", \"dmpsas\":" + String(sqmDmpsas) +    
                      "}";
  
  return jsonMsg;

}

void sendSQMdata()
{

  mqttSendSensorData(SQMJson());
}

