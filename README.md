# ESP8266-Weather-Station
Weather station using an ESP8266, a SparkFun weather station kit and several I2C sensors

It uses a BME280 for Temperature and humitity measurement, a MLX90614 to measure sky temperature ad calculate cloud coverage, a TSL2591 to get luminosity and calculate sky quality (SQM measured in mpsas), a VEML6075 to get UV index and an anemomether, a wing gauge and a rain sensor

All data is transmitted over MQTT.

The MLX90614 sensor reads sky temperature (object temperature) and applies a correction factor factor based on a formula from indi-duino Meteo: [https://github.com/LabAixBidouille/Indi/blob/master/3rdparty/indi-duino/devices/Firmwares/indiduinoMETEO/indiduinoMETEO.ino](https://github.com/LabAixBidouille/Indi/blob/master/3rdparty/indi-duino/devices/Firmwares/indiduinoMETEO/indiduinoMETEO.ino) , also calculates a cloud coverage percentage based on a formula from same program.

Sky quality measure is based on SQM_example by Gabe Shaughnessy on library [https://github.com/gshau/SQM_TSL2591](https://github.com/gshau/SQM_TSL2591)

> [!NOTE]
> On first run, the program creates an access point with name "METEO-TEMP" creating a web server accessible on http://192.168.4.1 which allow to configure SSID/password, node name, and MQTT configuration. Once configured these parameters, the program connects to the configured WiFi and starts data capturing and submission. This AP can also been activated after a reset if the SETUP_PIN (GPIO 0) is down during a lapse of 5 seconds after reset.

on normal operation, the program presents 4 web pages on the device IP address:
	**http://ip_address/setup** - the setup page mentioned above
	**http://ip_address/parameters** - which allows to configure several parameters of the program
	**http://ip_address/data** - which returns all sensors data in JSON format

## Configurable parameters are:
**Send interval:** num of seconds elapsed between sensors data reading and submission
**Altitude:** altitude in meters. it is calculated to calculate sea level pressure
**Radiance adjust:** multiplier for radiance adjustment
**UV adjust:** multiplier for UV adjustment

### Sky temperature parameters
- **Clear sky temp.**, defines the temperature of the limit of the sky temperature below what is considered clear sky (default -5ºC), above this temperature is considered cloudy, and above 0 ºC is considered overcast. Cloud index is calculated as a linear percentage of actual sky temp between 0ºC and "Clear sky temp".
- **K1 to K7** parametes, see [apendixes 6 and 7 of CloudWatcher](https://lunaticoastro.com/aagcw/enhelp/)

### MQTT parameters
- **broker:** ip address of broker
- **port:** broker access port, 1883 by default
- **user:** MQTT broker access user
- **passwd:** MQTT broker access password
- **topic:** topic to be used as base for MQTT messages (default:jeedom/%modname%, where %modname% is replaced by the name defined in WiFi section

### Weather Underground access parameters:
Station ID and Station Key, see [https://www.wunderground.com/pws/installation-guide](https://www.wunderground.com/pws/installation-guide)

> [!IMPORTANT]
> check all defines regarding GPIOs for leds, wing gauge and I2C bus.

## Leds behavior:
On start RED and GREEN leds are on for 5 secs. If SETUP_PIN (GPIO 0) is down at end of this period, both leds are off and the program enters in AP mode for configuration, otherwise goes to normal operation mode (STA mode).

Initial setup (AP mode), RED led blinks quickly (200 ms).

During WiFi conecction (setup STA), RED led bilnks slowly (500 ms), after the program is connected to WiFi, RED led is off and GREEN led is on during 3 secs.

On normal operation, both leds are off.
