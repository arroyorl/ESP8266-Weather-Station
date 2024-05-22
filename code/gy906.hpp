/***************************************************
  Functions for the GY-906 / MLX90614 Temp Sensor

  based on example Written by Limor Fried/Ladyada for Adafruit Industries.

  Bus I2C
  SCL GPIO05 / (D1)
  SDA GPIO04 / (D2)
 ****************************************************/

#include <Adafruit_MLX90614.h>      // Adafruit MLX90614 v 2.1.5

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

double  emissivity;
float   ambientTemp;
float   objTemp;
float   skyTemp;
float   cloudI;

bool    gy906connected=false;

bool    clouds = true;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void gy906setup(){

  Wire.begin(); //Joing I2C bus

  if (mlx.begin()) {
    DebugLn("GY-906 initialized");
    gy906connected=true;
  }
  else {  
    DebugLn("Error connecting to GY-906 sensor. Check wiring.");
    gy906connected=false;
  }

  emissivity = mlx.readEmissivity();
  DebugLn("Emissivity: " + String(emissivity,3));

}

// function copied from indi-duino Meteo: 
//    https://github.com/LabAixBidouille/Indi/blob/master/3rdparty/indi-duino/devices/Firmwares/indiduinoMETEO/indiduinoMETEO.ino  
//Cloudy sky is warmer that clear sky. Thus sky temperature meassure by IR sensor
//is a good indicator to estimate cloud cover. However IR really meassure the
//temperature of all the air column above increassing with ambient temperature.
//So it is important include some correction factor:
//From AAG Cloudwatcher formula. 
//  https://lunaticoastro.com/aagcw/enhelp/
//  https://github.com/LabAixBidouille/Indi/blob/master/3rdparty/indi-aagcloudwatcher/indi_aagcloudwatcher.cpp
//Sky temp correction factor. Tsky=Tsky_meassure - Tcorrection
//Formula Tcorrection = (K1 / 100) * (Tambient - K2 / 10) + (K3 / 100) * pow((exp (K4 / 1000* Tambient)) , (K5 / 100));

float skyTempAdj(float skyT, float ambT) {
float t67;

  float k1 = settings.data.k1;
  float k2 = settings.data.k2;
  float k3 = settings.data.k3;
  float k4 = settings.data.k4;
  float k5 = settings.data.k5;
  float k6 = settings.data.k6;
  float k7 = settings.data.k7;

  if (abs(k2/10.F - ambT) < 1) {
    t67 = sgn(k6) * sgn(ambT - k2/10.F) * abs(k2/10.F - ambT);
  }
  else {
    t67 = k6 / 10.F * sgn(ambT - k2/10.F) * (log(abs((k2 / 10.F - ambT))) / log(10.F) + k7 / 100);  
  }
  DebugLn("t67: " + String(t67));
  float Td = (k1 / 100.F) * (ambT - k2 / 10) + (k3 / 100.F) * pow((exp (k4 / 1000.F * ambT )) , (k5 / 100.F)) + t67;
  float res = skyT - Td;
  return res;
}

/*
* calculate cloud coverage index
* assumes sky temp above 0 is overcast, betwwen 0 and settings.data.cloudytemp is cloudy and below it is clear
* this calculates cloudy percentage as a linear value between 0 (100% ) and settings.data.cloudytemp (0%)
*  copied from indi-duino Meteo: 
*    https://github.com/LabAixBidouille/Indi/blob/master/3rdparty/indi-duino/devices/Firmwares/indiduinoMETEO/indiduinoMETEO.ino  
*/
#define T_CLOUDY   0.F

float cloudIndex(float skyT) {

  float tClear = settings.data.cloudytemp;

  if (skyT < tClear) skyT = tClear;
  if (skyT > T_CLOUDY) skyT = T_CLOUDY;
  float Index = (skyT - tClear) * 100 / (T_CLOUDY - tClear);
  return Index;
}

void get_gy906data(){

  DebugLn("Get GY-906 data");

  if (!gy906connected) return; // GY906 is not connected
  
  emissivity = mlx.readEmissivity();
  ambientTemp = mlx.readAmbientTempC();
  objTemp = mlx.readObjectTempC();

  // adjust sky temperature 
  skyTemp = skyTempAdj(objTemp, ambientTemp);  

  // calculate cloud coverage
  cloudI = cloudIndex(skyTemp);

  clouds = skyTemp > settings.data.cloudytemp;

  DebugLn("skyTemp: " + String(skyTemp,1) + ", skyTraw: " + String(objTemp,1) +", AmbientTemp: " + String(ambientTemp,1) + ", cloudIndex: " + String(cloudI,1) + ", clouds: " + String(clouds));
}

String gy906Json()
{

  String jsonMsg =  String("\"mlx90614\": ") + 
                      "{ \"skyT\":" + String(skyTemp) + 
                      ", \"rawSkyT\":" + String(objTemp) + 
                      ", \"ambientT\":" + String(ambientTemp) + 
                      ", \"cloudI\":" + String(cloudI) + 
                      ", \"clouds\":" + String(clouds) +    
                      "}";
  
  return jsonMsg;

}

void sendGY906data()
{

#ifdef  W_MQTT
  mqttSendSensorData(gy906Json());
#endif
}
