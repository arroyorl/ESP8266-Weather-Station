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

const char INIT_page[] PROGMEM = R"=====(
<html>
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
<div class=version>Firmware: @@VERSION@@</div>
<form method="post" action="/initForm">
<table>
<tr><td colspan=2 class=heading>WiFi Setup</td></tr>
<tr><td>SSID:</td><td><input type=text name="ssid" value="@@SSID@@"></td></tr>
<tr><td>PSK:</td><td><input type=password name="psk" value="@@PSK@@"></td></tr>
<tr><td>Name:</td><td><input type=text name="clockname" value="@@CLOCKNAME@@"></td></tr>
</table>
<p/>
<input type="submit" value="Update">
</form>
<div class="update">@@UPDATERESPONSE@@</div>
</html>
)=====";


void handleSetup() {
  DebugLn("handlesetup");
  String s = FPSTR(INIT_page);
  s.replace("@@SSID@@", settings.data.ssid);
  s.replace("@@PSK@@", settings.data.psk);
  s.replace("@@CLOCKNAME@@", settings.data.name);
  s.replace("@@VERSION@@",FVERSION);
  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  httpUpdateResponse = "";
  setupserver.send(200, "text/html", s);
}

void handleInitForm() {
  DebugLn("handleInitForm");
  DebugLn("mode "+String(WiFi.status()));

  String t_ssid = setupserver.arg("ssid");
  String t_psk = setupserver.arg("psk");
  String t_name = setupserver.arg("clockname");
  String(t_name).replace("+", " ");
  t_ssid.toCharArray(settings.data.ssid,SSID_LENGTH);
  t_psk.toCharArray(settings.data.psk,PSK_LENGTH);
  t_name.toCharArray(settings.data.name,NAME_LENGTH);
  httpUpdateResponse = "The configuration was updated.";
  setupserver.sendHeader("Location", "/");
  setupserver.send(302, "text/plain", "Moved");
  settings.Save();
  
  ap_setup_done = 1;
}
