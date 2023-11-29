/*  Copyright (C) 2016 Buxtronix and Alexander Pruss

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <EEPROM.h>

#ifndef SETTINGS_H 
#define SETTINGS_H 

#define MAGIC_LENGTH 4
#define SSID_LENGTH 32
#define PSK_LENGTH 64
#define NAME_LENGTH 32
#define ID_LENGTH 16
#define KEY_LENGTH 16
#define TIMEZONE_LENGTH 128

#define MAGIC "WST\0"

/* Configuration of NTP */
#define NTP_SERVER "europe.pool.ntp.org"           
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"  

char my_default_ssid[SSID_LENGTH] = "";
char my_default_psk[PSK_LENGTH] = "";

#define CLOCK_NAME "ESP-WST"

class Settings {
  public:
  struct {
    char    magic[MAGIC_LENGTH];               // magic = "WST"
    char    ssid[SSID_LENGTH];                 // SSID
    char    psk[PSK_LENGTH];                   // PASSWORD
    char    name[NAME_LENGTH];                 // NOMBRE
    char    stationID[ID_LENGTH];              // Wunderground Station ID
    char    stationKey[KEY_LENGTH];            // Wunderground Station Key
    char    timezone[TIMEZONE_LENGTH];         // TIMEZONE definition
    char    ntpserver[TIMEZONE_LENGTH];        // NTP server
    int     sendinterval;                      // Intervalo de envío a Wunderground (min)
    float   tempadjust;                        // Calibracion de temperatura
    float   altitude;                          // altitude over sea level 
    } data;
    
    Settings() {};
 
    void Load() {           // Carga toda la EEPROM en el struct
      EEPROM.begin(sizeof(data));
      EEPROM.get(0,data);  
      EEPROM.end();
     // Verify magic; // para saber si es la primera vez y carga los valores por defecto
     if (String(data.magic)!=MAGIC){
      data.magic[0]=0;
      data.ssid[0]=0;
      data.psk[0]=0;
      data.name[0]=0;
      data.stationID[0]=0;
      data.stationKey[0]=0;
      strcpy(data.timezone,TIMEZONE);
      strcpy(data.ntpserver,NTP_SERVER);
      data.sendinterval=1;
      data.tempadjust=0.0;
      data.altitude=0.0;
      Save();
     }
    };
      
    void Save() {
      EEPROM.begin(sizeof(data));
      EEPROM.put(0,data);
      EEPROM.commit();
      EEPROM.end();
    };
};
  #endif
