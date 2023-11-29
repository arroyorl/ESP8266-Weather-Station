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

const char MAIN_page[] PROGMEM = R"=====(
<head>
 <meta name="viewport" content="initial-scale=1">
 <style>
 body {font-family: helvetica,arial,sans-serif;}
 table {border-collapse: collapse; border: 1px solid black;}
 td {padding: 0.25em;}
 .title { font-size: 2em; font-weight: bold;}
 .name {padding: 0.5em;}
 .heading {font-weight: bold; background: #c0c0c0; padding: 0.5em;} 
 .update {color: #dd3333; font-size: 0.75em;}
 </style>
</head>
<div class=title>ESP8266 Weather Station</div>
<div class=name>Firmware: @@VERSION@@&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp<small> @@TIMENOW@@</small></div>
<form method="post" action="/form">
<table>

<tr><td colspan=2 class=heading>WiFi Setup</td></tr>
<tr><td>SSID:</td><td><input type=text name="ssid" value="@@SSID@@"></td></tr>
<tr><td>PSK:</td><td><input type=password name="psk" value="@@PSK@@"></td></tr>
<tr><td>Name:</td><td><input type=text name="clockname" value="@@CLOCKNAME@@"></td></tr>
<tr><td colspan=2>Update Wifi config:<input type=checkbox name=update_wifi value=1></td></tr>

<tr><td colspan=2 class=heading>Parameters</td></tr>
<tr><td>Temp. adjust:</td><td><input type=text name="tempadjust" value="@@TADJUST@@"> &#186C (-5.0 - 5.0)</td></tr>
<tr><td>Altitude:</td><td><input type=text name="altitude" value="@@ALTITUDE@@"> m (0 - 8000)</td></tr>

<tr><td colspan=2 class=heading>Real time clock</td></tr>
<tr><td>NTP server:</td><td><input type=text name="ntpserver" value="@@NTPSERVER@@"> </td></tr>
<tr><td>Time zone:</td><td><input type=text name="timezone" value="@@TIMEZONE@@"> </td></tr>

<tr><td colspan=2 class=heading>Wunderground access</td></tr>
<tr><td>Station ID:</td><td><input type=text size=35 name="stationid" value="@@STATIONID@@"></td></tr>
<tr><td>Station Key:</td><td><input type=text size=35 name="stationkey" value="@@STATIONKEY@@"></td></tr>
<tr><td>Update Interval:</td><td><input type=text name="sendint" value="@@SENDINT@@"> min (1 - 60)</td></tr>

</table>
<p/>
<input type="submit" value="Update">
</form>
<div class="update">@@UPDATERESPONSE@@</div>
</html>
)=====";
