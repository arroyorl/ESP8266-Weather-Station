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
<table style="width:700px">

<tr><td colspan=2 class=heading>WiFi Setup</td></tr>
<tr><td>SSID:</td><td><input type=text name="ssid" value="@@SSID@@"></td></tr>
<tr><td>PSK:</td><td><input type=password name="psk" value="@@PSK@@"></td></tr>
<tr><td>Name:</td><td><input type=text name="clockname" value="@@CLOCKNAME@@"></td></tr>
<tr><td colspan=2>Update Wifi config:<input type=checkbox name=update_wifi value=1></td></tr>

<tr><td colspan=2 class=heading>Parameters</td></tr>
<tr><td>Send interval:</td><td><input type=text name="sendinterval" value="@@SENDINTERVAL@@"> min. (1 - 60)</td></tr>
<tr><td>Altitude:</td><td><input type=text name="altitude" value="@@ALTITUDE@@"> m (0 - 8000)</td></tr>
<tr><td>Radiance adjust:</td><td><input type=text name="radadjust" value="@@RADADJUST@@"> </td></tr>
<tr><td>UV adjust:</td><td><input type=text name="uvadjust" value="@@UVADJUST@@"> </td></tr>

</table>

<table style="width:700px">
<tr><td colspan=4 class=heading>Sky temperature parameters</td></tr>
<tr><td>Clear sky temp.:</td>
  <td colspan=2><input type=text size=6 name="cloudytemp" value="@@CLOUDYTEMP@@">
      <small>&nbsp;&nbsp;<label align="right" style="width:50px; display:inline-block; background:@@COLSKYTEMP@@"> @@SKYTEMP@@ </label>&nbsp;&#186;C </small></td></tr>
<tr><td>K1 <input type=text size="6" name="k1" value="@@K1@@"></td>
    <td>K2 <input type=text size="6" name="k2" value="@@K2@@"></td>
    <td>K3 <input type=text size="6" name="k3" value="@@K3@@"></td>
	<td>K4 <input type=text size="6" name="k4" value="@@K5@@"></td>
</tr>
<tr><td>K5 <input type=text size="6" name="k5" value="@@K4@@"></td>
    <td>K6 <input type=text size="6" name="k6" value="@@K6@@"></td>
 	<td>K7 <input type=text size="6" name="k7" value="@@K7@@"></td>
    <td></td>
</tr>
</table>

<table style="width:700px">
<tr><td colspan=2 class=heading>Real time clock</td></tr>
<tr><td>NTP server:</td><td><input type=text size=35 name="ntpserver" value="@@NTPSERVER@@"> </td></tr>
<tr><td>Time zone:</td><td><input type=text size=35 name="timezone" value="@@TIMEZONE@@"> </td></tr>

<tr><td colspan=2 class=heading>MQTT Setup</td></tr>
<tr><td>MQTT broker:</td><td><input type=text size=35 name="mqttbroker" value="@@MQTTBROKER@@"></td></tr>
<tr><td>MQTT port:</td><td><input type=text size=35 name="mqttport" value="@@MQTTPORT@@"></td></tr>
<tr><td>MQTT user:</td><td><input type=text size=35 name="mqttuser" value="@@MQTTUSER@@"></td></tr>
<tr><td>MQTT passwd:</td><td><input type=text size=35 name="mqttpasswd" value="@@MQTTPASSWD@@"></td></tr>
<tr><td>MQTT topic:</td><td><input type=text size=35 name="mqtttopic" value="@@MQTTTOPIC@@"></td></tr>

<tr><td colspan=2 class=heading>Wunderground access</td></tr>
<tr><td>Station ID:</td><td><input type=text size=35 name="stationid" value="@@STATIONID@@"></td></tr>
<tr><td>Station Key:</td><td><input type=text size=35 name="stationkey" value="@@STATIONKEY@@"></td></tr>

</table>
<p/>
<input type="submit" value="Update">
</form>
<div class="update">@@UPDATERESPONSE@@</div>
</html>
)=====";

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
  s.replace("@@SENDINTERVAL@@",String(settings.data.sendinterval));
  s.replace("@@TADJUST@@",String(settings.data.tempadjust));
  s.replace("@@ALTITUDE@@",String(settings.data.altitude));
  s.replace("@@RADADJUST@@",String(settings.data.radadjust));
  s.replace("@@UVADJUST@@",String(settings.data.uvadjust));

#define COLRED    "#FF4500"   // orange red
#define COLGREEN  "#7FFF00"   //Chartreuse (ligth green)

  s.replace("@@CLOUDYTEMP@@",String(settings.data.cloudytemp,1));
  s.replace("@@SKYTEMP@@",String(skyTemp,1));
  if (skyTemp < settings.data.cloudytemp)
    s.replace("@@COLSKYTEMP@@", COLGREEN);
  else
    s.replace("@@COLSKYTEMP@@", COLRED);
  s.replace("@@K1@@",String(settings.data.k1,2));
  s.replace("@@K2@@",String(settings.data.k2,2));
  s.replace("@@K3@@",String(settings.data.k3,2));
  s.replace("@@K4@@",String(settings.data.k4,2));
  s.replace("@@K5@@",String(settings.data.k5,2));
  s.replace("@@K6@@",String(settings.data.k6,2));
  s.replace("@@K7@@",String(settings.data.k7,2));

  s.replace("@@NTPSERVER@@",settings.data.ntpserver);
  s.replace("@@TIMEZONE@@",settings.data.timezone);

  s.replace("@@MQTTBROKER@@",settings.data.mqttbroker);
  s.replace("@@MQTTPORT@@",String(settings.data.mqttport));
  s.replace("@@MQTTUSER@@",settings.data.mqttuser);
  s.replace("@@MQTTPASSWD@@",settings.data.mqttpassword);
  s.replace("@@MQTTTOPIC@@",settings.data.mqtttopic);

  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  strftime(hhmm,6,"%H:%M",&tm);     // format time (HH:MM)
  s.replace("@@TIMENOW@@",String(hhmm));

  httpUpdateResponse = "";
  server.send(200, "text/html", s);
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

  String sint = server.arg("sendinterval");
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
  aux = server.arg("radadjust");
  if (aux.length()) {
    settings.data.radadjust = aux.toFloat();
  }
  aux = server.arg("uvadjust");
  if (aux.length()) {
    settings.data.uvadjust = aux.toFloat();
  }

  String cloudyT = server.arg("cloudytemp");
  if (cloudyT.length()) {
    settings.data.cloudytemp = cloudyT.toFloat();
    settings.data.cloudytemp=max(float(-50),settings.data.cloudytemp);
    settings.data.cloudytemp=min(settings.data.cloudytemp,float(20));
  }
  aux = server.arg("k1");
  if (aux.length()) {
    settings.data.k1 = aux.toFloat();
  }
  aux = server.arg("k2");
  if (aux.length()) {
    settings.data.k2 = aux.toFloat();
  }
  aux = server.arg("k3");
  if (aux.length()) {
    settings.data.k3 = aux.toFloat();
  }
  aux = server.arg("k4");
  if (aux.length()) {
    settings.data.k4 = aux.toFloat();
  }
  aux = server.arg("k5");
  if (aux.length()) {
    settings.data.k5 = aux.toFloat();
  }
  aux = server.arg("k6");
  if (aux.length()) {
    settings.data.k6 = aux.toFloat();
  }
  aux = server.arg("k7");
  if (aux.length()) {
    settings.data.k7 = aux.toFloat();
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

  String brokerprev = String(settings.data.mqttbroker);
  int portprev = settings.data.mqttport;
  String topicprev = settings.data.mqtttopic;
  String userprev = settings.data.mqttuser;
  String passwprev = settings.data.mqttpassword;
  aux = server.arg("mqttbroker");
  aux.toCharArray(settings.data.mqttbroker,128);
  aux = server.arg("mqttuser");
  aux.toCharArray(settings.data.mqttuser,30);
  aux = server.arg("mqttpasswd");
  aux.toCharArray(settings.data.mqttpassword,30);
  aux = server.arg("mqtttopic");
  aux.toCharArray(settings.data.mqtttopic,30);
  aux = server.arg("mqttport");
  if (aux.length()) {
    settings.data.mqttport=aux.toInt();
  }

  if (String(settings.data.mqttbroker) != brokerprev 
      || settings.data.mqttport != portprev 
      || String(settings.data.mqtttopic) != topicprev
      || String(settings.data.mqttuser) != userprev 
      || String(settings.data.mqttpassword) != passwprev) {
    // MQTT broker or port has changed. restart mqtt connection
    mqtt_disconnect();
    mqtt_init();
  }

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
