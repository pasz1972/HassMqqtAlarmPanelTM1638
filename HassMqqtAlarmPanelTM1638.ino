/* ====================================================================
 *  
 *  Home Assistant MQQT Alarm Panel - TM1638
 *  
 *  Based on : https://github.com/rjbatista/tm1638-library/wiki
 *  and      : https://github.com/knolleary/pubsubclient
 *  hass     : https://home-assistant.io
 *  
 *  Example configuration.yaml entry:
 
alarm_control_panel:
  - platform: manual_mqtt
    state_topic: home/alarm/state
    command_topic: home/alarm/set
    
=====================================================================*/
#include "userconfig.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TM1638.h>

// network settings

const char* ssid = mySSID;
const char* password = myPASSWORD;
const char* mqtt_server = myMQTTBROKER;
const int   mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
// NodeMcu    D0 D1 D2            
TM1638 module(16, 5, 4);

long lastMsg = 0;
long lastPnd = 0;
char msg[50];
char pin[5];
int  value = 0;
byte lastkey = 0;
bool pndToggle = false;


enum AlarmState { Disarmed, Disarming, ArmedHome, Arming, ArmedAway, Pending, Triggered };

AlarmState state;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  module.clearDisplay();
  module.setDisplayToString("CONNECT ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  byte dot = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    module.setDisplayToString("CONNECT  ",dot);
    dot++;
    if (dot > 256)
    {
      dot = 0;
    }
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  module.clearDisplay();
  module.setDisplayToString("DISARNN  ");
  state = Disarmed;
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  char s[20];
  
  sprintf(s, "%s", payload);

  if ( (strcmp(topic,"home/alarm/state")==0))
  {
    module.clearDisplay();
    payload[length] = '\0';
    String sp = String((char*)payload);
    if (sp == "disarmed")
    {
      module.setDisplayToString("DISARNND");
      module.setLEDs(0);
      state = Disarmed;
      Serial.println("Disarmed");
    }
    
    else if (sp == "armed_home")
    {
      module.setDisplayToString("ARM HOME");
      state = ArmedHome;
      module.setLEDs(0);
      Serial.println("Armed home");
    }
    else if (sp == "armed_away")
    {
      module.setDisplayToString("ARM AWAY");
      state = ArmedAway;
      Serial.println("Armed away");
    }
    else if (sp == "pending")
    {
      module.setDisplayToString("PENDING ");
      state = Pending;
      Serial.println("Pending");
    }
    else if (sp == "triggered")
    {
      module.setDisplayToString("TRIGGERD");
      state = Triggered;
      Serial.println("Triggered");
    }
    else
    {
      Serial.print("Unknown state message : ");
      Serial.println(s);
    }
  }

  if ( (strcmp(topic,"home/alarm/text")==0))
  {
    module.clearDisplay();
    module.setDisplayToString(s);
  }
  
  if (strcmp(topic,"home/alarm/value")==0)
  { 
    module.clearDisplay();
    module.setDisplayToString(s,0x02);
  }

  if (strcmp(topic,"home/alarm/led")==0)
  { 
    module.setLEDs(hex8(payload));
  }  
}

/* interpret the ascii digits in[0] and in[1] as hex
* notation and convert to an integer 0..255.
*/
int hex8(byte *in)
{
   char c, h;

   c = (char)in[0];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   h = c;

   c = (char)in[1];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   return ( h<<4 | c);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("home/alarm", "ONLINE");
      // ... and resubscribe
      client.subscribe("home/alarm/state");
      client.subscribe("home/alarm/value");
      client.subscribe("home/alarm/set");
      client.subscribe("home/alarm/text");
      client.subscribe("home/alarm/led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  module.setLEDs(0);
  module.clearDisplay();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  byte keys = module.getButtons();
  if (keys != lastkey)
  {
    lastkey = keys;
    int n = 0;
    int k = 0;
    while (n < 8)
    {
      int i = (lastkey >> n) && 0x01;
      k = k + i;
      n++;
    }

    // send to command_topic
    switch (k) {
    case 1:
      client.publish("home/alarm/set", "DISARM");
      state = Disarming;
      module.setDisplayToString("DISARM");
      break;
    case 2:
      client.publish("home/alarm/set", "ARM_HOME");
      state = Arming;
      module.setDisplayToString("ARM HOME");
      break;
    case 3:
      client.publish("home/alarm/set", "ARM_AWAY");
      state = Arming;
      module.setDisplayToString("ARM AWAY");
      break;
    default:
      if (k != 0)
      {
        snprintf (msg, 75, "%ld", k);
        Serial.println(msg);
        client.publish("home/alarm/keys", msg);
      }
    break;
    }
  }

  // send an alive signal every minute
  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    Serial.print("I'm still alive");
    client.publish("home/alarm", "ALIVE");
  }

  if (
        ( (now - lastPnd > 1000) && (state==Pending)   ) ||
        ( (now - lastPnd >  500) && (state==Triggered) ) 
     )   
    {
    lastPnd = now;
    if (pndToggle)
    {
      module.setLEDs(0xFF);
    }
    else
    {
      module.setLEDs(0);
    }
    pndToggle = !pndToggle;
    Serial.print(".");
  }
  
}

