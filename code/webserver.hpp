//--------------------------------------------
//  HTTP POST
//--------------------------------------------
void sendHTTP (String httpPost){
  WiFiClient client;
  HTTPClient http;
  DebugLn(httpPost);
  http.begin(client, httpPost);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("");
  DebugLn(httpCode);    
  http.end();
}

//--------------------------------------------
//  HTTP GET
//--------------------------------------------
void sendHTTPGet (String httpGet){
  WiFiClient client;
  HTTPClient http;
  DebugLn(httpGet);
  http.begin(client, httpGet);
  int httpCode = http.GET();
  DebugLn(httpCode);    
  http.end();
}

//--------------------------------------------
//  HTTPs GET
//--------------------------------------------
void sendHTTPsGet (String httpGet){
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  // fingerprint not needed
  client->setInsecure();
  // send data
  DebugLn("HTTPsGet: " + httpGet);
  https.begin(*client, httpGet);
  // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = https.GET();
  String payload = https.getString();

  DebugLn("HTTPsGet return code: " + String(httpCode));    // this return 200 when success
  DebugLn(payload);     // this will get the response
  https.end();

}

//--------------------------------------------
//  web server page returning raw data (json)
//--------------------------------------------
void handleRowData() {
  DebugLn("handleRowData");

  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time

  httpUpdateResponse = "";
  server.send(200, "text/html", prepareJsonData() + "\r\n");

}

//--------------------------------------------
//  send data to wunderground (https get)
//--------------------------------------------
void sendDataWunderground(){

  String  weatherData =  "https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?";
  weatherData += "ID=" + String(settings.data.stationID);
  weatherData += "&PASSWORD=" + String(settings.data.stationKey);
  weatherData += "&dateutc=now";
  weatherData += "&action=updateraw";
  weatherData += "&tempf=" + String(tempf);
  weatherData += "&humidity=" + String(humidity);
  weatherData += "&dewptf=" + String(dewpointf);
  weatherData += "&baromin=" + String(baromin);
  weatherData += "&winddir=" + String(winddir);
  weatherData += "&windspeedmph=" + String(windspeed / 1.609344);
  weatherData += "&windgustmph=" + String(windgust / 1.609344);
  weatherData += "&windgustdir=" + String(winddir);
  weatherData += "&rainin=" + String(rainrate1hmm / 25.4);
  weatherData += "&dailyrainin=" + String (rain24hmm / 25.4);

  // add UV info if ML1145 is connected
  if (veml6075Active) weatherData += "&UV=" + String(uvi);

  // solar radiance
  weatherData += "&solarradiation=" + String (radiance);
  
  // send to Wunderground
  sendHTTPsGet(weatherData);
              
}
