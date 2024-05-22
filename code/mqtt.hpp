///////////////////////////////////////////////////////////
// MQTT functions
///////////////////////////////////////////////////////////

#include <PubSubClient.h>  // v 2.8
WiFiClient espClient;
PubSubClient client(espClient);

String      base_topic;

#define MQTT_BUFFER_SIZE  1024

/**************************************************
 * Callback function for MQTT
 * ***********************************************/
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
char* buff;
const char* nomsg="";

  Debug("MQTT message received [");
  Debug(String(topic));
  Debug("] :");

  buff = (char*)malloc(length+1);
  memcpy(buff, payload, length);
  buff[length] = '\0';
  DebugLn(buff);

  // is payload no NULL?
  if (length > 0) { 
    if (String(topic) == (base_topic + "/setup/altitude") ) {
      float altitudeprev = settings.data.altitude;
      settings.data.altitude = atof(buff);
      // save settings only if parameter has changed
      if (altitudeprev != settings.data.altitude) settings.Save();  
      // remove if retained message
      client.publish((base_topic + "/setup/altitude").c_str(), nomsg, true);
    }
  }
  
  free(buff);
}

/*************************************************
 * MQTT reconnect function
 * **********************************************/
bool mqtt_reconnect() {
int res;

  if (!client.connected() ) {
    DebugLn("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = String(settings.data.name) + String(random(0xffff), HEX);

    // Attempt to connect
    if(strlen(settings.data.mqttuser)) {
      res = client.connect(clientId.c_str(), settings.data.mqttuser, settings.data.mqttpassword);
    }
    else {
      res = client.connect(clientId.c_str());
    }
    if (res) {
      DebugLn("MQTT connected");
      // once connected ....resubscribe
      base_topic = String(settings.data.mqtttopic);
      base_topic.replace("%modname%",String(settings.data.name));
      DebugLn("base topic: " + base_topic);
      client.subscribe((base_topic + "/setup/#").c_str());
      DebugLn("subscribe: " + base_topic + "/setup/#");
    } else {
      Debug ("MQTT reconnection failed, rc=");
      DebugLn (client.state());
    }
  }
  else // already connected
    res = true;

  if (res) {
  }

  return res;
}

/*************************************************
 * MQTT init function
 * **********************************************/
void mqtt_init() {

  client.setServer(settings.data.mqttbroker, settings.data.mqttport);
  client.setBufferSize(MQTT_BUFFER_SIZE);
  DebugLn("setServer: " + String(settings.data.mqttbroker) + ", port: " +String(settings.data.mqttport));
  client.setCallback(mqtt_callback);
  mqtt_reconnect();

}

/*************************************************
 * MQTT disconnect function
 * **********************************************/
void mqtt_disconnect() {
  client.disconnect();
}

/*************************************************
 * MQTT send function
 * **********************************************/
void mqtt_send(String subtopic, String message, bool retain){

String topic = base_topic + "/" + subtopic;

  if (strlen(settings.data.mqttbroker) > 0 ) {
    if(mqtt_reconnect() ) {
      // send data to topic
      client.publish(topic.c_str(), message.c_str(), retain);
      Debug("mqtt send [" );
      Debug(topic);
      Debug("]: ");
      DebugLn(message);
    }
  }

}

/*************************************************
 * MQTT send data of one sensor
 * **********************************************/
void mqttSendSensorData(String jsonData) {

  mqtt_send ("data", jsonData, false);

}

/*************************************************
 * handle MQTT
 * **********************************************/
void handleMQTT(){

    // ********* initialize MQTT ******************
  if (strlen(settings.data.mqttbroker) > 0 ) {
    client.loop();
  }
  
}

