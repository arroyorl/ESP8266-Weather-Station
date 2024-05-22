#include <Wire.h>
#include <Adafruit_Sensor.h>  // 1.1.14
#include <Adafruit_INA219.h>  // 1.2.3

Adafruit_INA219 ina219;

float       shuntVoltage=0;
float       cellCurrent=0;
float       radiance=0;

bool        ina219Active=false;


/**************************************************************************/
/*     INA219 setup function 
/**************************************************************************/
void INA219setup(void) 
{

  /* Initialise the sensor */
  // INA219 (current), SDA=4 (D2), SCL=5(D1)
  if (ina219.begin()) {
    ina219Active = true;
    ina219.setCalibration_16V_400mA();
    DebugLn("-> INA219 begin");
  }
  else {
    ina219Active = false;
    DebugLn("-> INA219 not found");
  }

}

///////////////////////////// INA219 /////////////////////////////////////////////
//
void getINA219data(void) {

  if (!ina219Active) return;

  /* Display the results (light is measured in lux) */
  shuntVoltage=ina219.getShuntVoltage_mV();
  cellCurrent=ina219.getCurrent_mA(); 
  if (shuntVoltage < 0 ) shuntVoltage = 0;
  if (cellCurrent < 0 ) cellCurrent = 0;
  radiance = cellCurrent * settings.data.radadjust;
  DebugLn("V: " + String(shuntVoltage));
  DebugLn("I: " + String(cellCurrent));
  DebugLn("INA219 data read !");
  
}

