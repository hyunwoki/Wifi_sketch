#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "ap_setting.h"



#define NODE_1

//#define ssid "pi"
//#define password "test1234"

#define mqtt_server "211.180.34.28"
#define mqtt_user "your_username"
#define mqtt_password "your_password"

#define mqttClientId          "DEVICE4"
#define mqttClientNode        "1"
#define mqttDomain            "SKMT"
#define mqtt_Topic             mqttDomain"/"mqttClientId
#define willTopic              mqttDomain"/"mqttClientId"/LWT"
#define humidity_topic        "sensor/humidity"
#define temperature_topic     "sensor/temperature"
//#define RELAY_A              mqttDomain"/"mqttClientId"/RELAY1"
//#define RELAY_B              mqttDomain"/"mqttClientId"/RELAY2"


#ifdef NODE_1
       #define RELAY_A     mqttDomain"/"mqttClientId"/RELAY1"
       #define RELAY_B     mqttDomain"/"mqttClientId"/RELAY2"
#else
       #define RELAY_A     mqttDomain"/"mqttClientId"/RELAY3"
       #define RELAY_B     mqttDomain"/"mqttClientId"/RELAY4"
#endif


int  willLwtQos = 0;
int  willRetain = true;
char *REALAY_A = 0;
char *REALAY_B = 0;
char *HUMIDITY_A = 0;
char *TEMPERATURE_B = 0;
char message_buff[100];
String clientName;
unsigned long fast_update, slow_update;
unsigned long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, mqtt_callback, espClient);



int pinLED = 2;                                 // internal LED of ESP-12F
int btn1State = LOW;
int btn2State = LOW;
int length;


#define LED_RED   4
#define LED_GREEN 15
#define DHT_PIN   14
#define relay1    13
#define relay2    12
#define btn1      2
#define btn2      0

DHT dht = DHT(DHT_PIN, DHT22);


void setup() {
  
    Serial.begin(115200);
    wifi_connect();
    mqtt.setServer(mqtt_server, 1883);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += mqttClientId;
    clientName += ".";  
    clientName += mqttClientNode;
    clientName += "-";
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(mqtt_server);
    Serial.print(" as ");
    Serial.println(clientName);
    lastReconnectAttempt = 0;

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);  
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(btn1, INPUT);
    pinMode(btn2, INPUT); 

}


void wifi_connect() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    //  WiFi.config(IPAddress(192, 168, 10, 112), IPAddress(192, 168, 10, 1), IPAddress(255, 255, 255, 0));
  
    int Attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Attempt++;
      LED_WIFI();
      Serial.print(". ");
      Serial.print(Attempt);
      if (Attempt == 100)
      {
        Serial.println();
        Serial.println("------------------------------> Could not connect to WIFI");
        ESP.restart();
        delay(200);
      }
    }
  
    Serial.println();
    Serial.println("===> WiFi connected");
    Serial.print("---------------------------------> IP address: ");
    Serial.println(WiFi.localIP());


    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
}  



boolean reconnect() {
  mqtt.disconnect ();
  delay(100);
  if (mqtt.connect((char*) clientName.c_str(), willTopic, willLwtQos, willRetain, "0")) {
    Serial.println("-----> mqtt connected");    
    mqtt.publish(willTopic, "1", true);
  } else {
    LED_ERR();
    Serial.print("-----------------> failed, rc=");
    Serial.println(mqtt.state());
  }
  //startMills = millis();
  return mqtt.connected();
}




void mqtt_callback(char* topic, byte* payload, unsigned int length) {

      int i = 0;
      for(i=0; i<length; i++) {
      message_buff[i] = payload[i];
      }      
      message_buff[i] = '\0';
      String msgString = String(message_buff);      
     Serial.println("Inbound: " + String(topic) +":"+ msgString);   
  
     
     if (String(topic) == RELAY_A) {
      if (msgString == "1")  digitalWrite( relay1, 1 );   
      else                   digitalWrite( relay1, 0 );   
      }
      
     if (String(topic) == RELAY_B) {
      if (msgString == "1")  digitalWrite( relay2, 1 );   
      else                   digitalWrite( relay2, 0 );   
     }
}





void loop() {
      
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqtt.connected()) {
           long now = millis();
              if (now - lastReconnectAttempt > 5000) {
                   lastReconnectAttempt = now;
                     if (reconnect()) {
                         lastReconnectAttempt = 0;
                        }
                }
                
                Serial.print("failed, rc=");
                Serial.println(mqtt.state());
        }
      } else {
        wifi_connect();
      }


  if ((millis()-fast_update)>50000)
  {
       fast_update = millis();
       Temperature();
  }

  btnState1();
  btnState2();  
  mqtt.loop();
}









void btnState1(){
  
  if (btn1State != digitalRead(btn1)) {
    btn1State = !btn1State;
    if (btn1State){      
      Serial.println("Switch1 LOW");      
      digitalWrite(relay1, LOW);
      mqtt.publish(RELAY_A, "0");   
    }
    else {      
      Serial.println("Switch1 HIGH");
      digitalWrite(relay1, HIGH);
      mqtt.publish(RELAY_A, "1");         
    }
  }
}



void btnState2(){
  
  if (btn2State != digitalRead(btn2)) {
    btn2State = !btn2State;
    if (btn2State){      
      Serial.println("Switch2 LOW");      
      digitalWrite(relay2, LOW);
      mqtt.publish(RELAY_B, "0");  
    }
    else {      
      Serial.println("Switch2 HIGH");
      digitalWrite(relay2, HIGH);
      mqtt.publish(RELAY_B, "1");  
    }
  }
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}



void LED_ERR(){
  digitalWrite(LED_RED, HIGH);
  delay(500);
  digitalWrite(LED_RED, LOW);
  delay(500);
}


void LED_RUN(){
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_GREEN, LOW);
  delay(500);
}


void LED_WIFI(){
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  delay(200);
}



/*-------- NTP code ----------*/
/*
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("------------------> Transmit NTP Request called");
  sendNTPpacket(timeServer);
  delay(1000);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.print("---------------> Receive NTP Response :  ");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.print("=================> No NTP Response :-( : ");
  Serial.println(millis() - beginWait);
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress & address)
{
  //Serial.println("Transmit NTP Request");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  Serial.println("------------> Transmit NTP Sent");
}

*/
