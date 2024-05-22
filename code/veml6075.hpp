/*************************************************************************
          VEML6075 UV sensor
          based on Adafruit library example

          v1.0 initial version
          
          v2.0 added uvdajust coefficient in UVI
               integration 50 ms
               normal dynamic mode
               empirically calculated coefficiets for a plastic cover
*************************************************************************/

#include <Wire.h>
#include "Adafruit_VEML6075.h"

Adafruit_VEML6075 uv = Adafruit_VEML6075();

float uva;
float uvb;
float uvi;
bool  veml6075Active = false;


bool veml6075setup(){

  if (! uv.begin()) {
    Serial.println("No VEML6075 detected, check wiring");
    veml6075Active = false;
    return false;
  }

  // Set the integration constant
  uv.setIntegrationTime(VEML6075_50MS);
  // Get the integration constant and print it!
  Serial.print("Integration time set to ");
  switch (uv.getIntegrationTime()) {
    case VEML6075_50MS: Debug("50"); break;
    case VEML6075_100MS: Debug("100"); break;
    case VEML6075_200MS: Debug("200"); break;
    case VEML6075_400MS: Debug("400"); break;
    case VEML6075_800MS: Debug("800"); break;
  }
  DebugLn(" ms");

  // Set the high dynamic mode
  uv.setHighDynamic(false);
  // Get the mode
  if (uv.getHighDynamic()) {
    DebugLn("High dynamic reading mode");
  } else {
    DebugLn("Normal dynamic reading mode");
  }

  // Set the mode
  uv.setForcedMode(false);
  // Get the mode
  if (uv.getForcedMode()) {
    DebugLn("Forced reading mode");
  } else {
    DebugLn("Continuous reading mode");
  }

/*********************************************************************
UV COEFFICIENTS AND RESPONSIVITY
                               a    b    c    d   UVAresp  UVBresp
No teflon (open air)          2.22 1.33 2.95 1.74 0.001461 0.002591
0.1 mm teflon 4.5 mm window   2.22 1.33 2.95 1.74 0.002303 0.004686
0.1 mm teflon 5.5 mm window   2.22 1.33 2.95 1.74 0.002216 0.005188
0.1 mm teflon 10 mm window    2.22 1.33 2.95 1.74 0.002681 0.004875
0.25 mm teflon 10 mm window   2.22 1.33 2.95 1.74 0.002919 0.009389
0.4 mm teflon 10 mm window    2.22 1.17 2.95 1.58 0.004770 0.006135
0.7 mm teflon 10 mm window    2.22 1.17 2.95 1.58 0.007923 0.008334

see: https://cdn.sparkfun.com/assets/3/9/d/4/1/designingveml6075.pdf
********************************************************************/

// empirically calculated coefficients for a plastic cover window 
// 1st check values with and without cover and calculate UVAcover/UVAuncover and same for UVB
//     multiply b by UVAcover/uncover factor
//     and d by UVBcover/uncover
// last check UVI of a good quality near weather station
//     calculate UVIgolden/UVIthis factor and
//     multiply UVAresp and UVBresp by this factor
//
// all of above is not very scientifical, but gives a reanonable good UVI value

  uv.setCoefficients(2.22, 1.14,  // UVA_A and UVA_B coefficients (0.8586)
                     2.95, 1.43,  // UVB_C and UVB_D coefficients (0.8226)
                     0.000730, 0.001296); // UVA and UVB responses (0.5)

    veml6075Active = true;
    return true;

}

void get_veml6075data(){

  if (veml6075Active) {
    uva = uv.readUVA();
    uvb = uv.readUVB();
    uvi = uv.readUVI() * settings.data.uvadjust;

    DebugLn("Reading VEML6075 data: raw UVA: " + String(uva) + ", raw UVB: " + String(uvb) + ", UV index: " + String(uvi));
  }

}

String veml6075Json()
{

  String jsonMsg =  String("\"veml6075\": ") + 
                      "{ \"uva\":" + String(uva) + 
                      ", \"uvb\":" + String(uvb) + 
                      ", \"UVindex\":" + String(uvi,0) + 
                      "}";
  
  return jsonMsg;

}

void sendVEML6075data()
{

  mqttSendSensorData(veml6075Json());
}

