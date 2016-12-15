#include <SPI.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <RF24Network.h>
#include <RF24.h>
#include "ap_setting.h"

/***** Configure the chosen CE,CS pins *****/
RF24 radio(4,15);                    // nRF24L01(+) radio attached using Getting Started board 
RF24Network network(radio);          // Network uses that radio

const uint16_t this_node = 01;        // Address of our node in Octal format
const uint16_t other_node = 00;       // Address of the other node in Octal format
const uint16_t channel = 8;

#define DEBUG_PRINT 0
#define EVENT_PRINT 0
#define PAYLOAD_PRINT 1

#define mqttClientNode        "PowerMeter"
#define mqttClientId          "DEVICE8"
#define mqtt_server           "211.180.34.28"
//#define mqttUsername        "samknag"
//#define mqttPassword        "test1234"
#define mqttDomain            "SKMT"
#define mqttTopic             mqttDomain"/"mqttClientId
#define mqttLwtTopic          mqttDomain"/"mqttClientId"/LWT"
#define LED_RED   2 
#define LED_GREEN 0




float value;
int  mqttLwtQos = 0;
int  mqttLwtRetain = true;
char message_buff[100];
String clientName;
unsigned long fast_update, slow_update;
unsigned long publish_cnt = 0;


WiFiClient wifiClient;
PubSubClient mqtt(mqtt_server, 1883, callback, wifiClient);


// Structure of our payload
struct payload_t {                  
  //unsigned long ms;
  unsigned long counter;
  unsigned long power1;
  unsigned long power2;
  unsigned long power3;  
};





void wifi_connect() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    //WiFi.config(IPAddress(192, 168, 10, 112), IPAddress(192, 168, 10, 1), IPAddress(255, 255, 255, 0));
  
    int Attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Attempt++;
      LED_WIFI();
      if (EVENT_PRINT) {
        Serial.print(". ");
        Serial.print(Attempt); 
      }
           
      if (Attempt == 50)
      {
          if (EVENT_PRINT) {
              Serial.println();
              Serial.println("Could not connect to WIFI");
              ESP.restart();
          }
      }
    }
   if (EVENT_PRINT) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
   }
     digitalWrite(LED_GREEN, HIGH);
}  


void reconnect() {
  while (!mqtt.connected()) {
     if (EVENT_PRINT) { 
         Serial.print("Attempting MQTT connection...");
     }
       if (mqtt.connect((char*) clientName.c_str(), mqttLwtTopic, mqttLwtQos, mqttLwtRetain, "0")) {
            byte data[] = { '1' };
            mqtt.subscribe(mqttTopic"/#");    
            mqtt.publish(mqttLwtTopic, data, 1, mqttLwtRetain);     
            //mqtt.publish(mqttLwtTopic, "1", true);     
            //mqtt.publish(mqttTopic"/ID", mqttClientId);  
               if (EVENT_PRINT) {           
                  Serial.println("connected");       
               }
        } else {
             if (EVENT_PRINT) {
                Serial.print("failed, rc=");
                Serial.print(mqtt.state());
                Serial.println(" try again in 1 seconds");
                
             }
            LED_ERR();
      }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
   if (PAYLOAD_PRINT) {
      int i = 0;
      for(i=0; i<length; i++) {
      message_buff[i] = payload[i];     
      }      
      message_buff[i] = '\0';
      String msgString = String(message_buff);      
      Serial.println("Inbound: " + String(topic) +":"+ msgString);  
      LED_PAYLOAD();
   } 
}



void setup() {
  
  if (DEBUG_PRINT) {
      Serial.begin (115200);
      Serial.println();
      Serial.println("Energry controller Starting");

      Serial.print("ESP.getChipId() : ");
      Serial.println(ESP.getChipId());
  
      Serial.print("ESP.getFlashChipId() : ");
      Serial.println(ESP.getFlashChipId());
  
      Serial.print("ESP.getFlashChipSize() : ");
      Serial.println(ESP.getFlashChipSize());
  }    

      delay(20);
      pinMode(LED_RED, OUTPUT);  
      pinMode(LED_GREEN, OUTPUT);  
  
   
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


   if (DEBUG_PRINT) {
      Serial.print("Client : ");
      Serial.println(mqttClientId);
      
      Serial.print("NODE : ");
      Serial.println(mqttClientNode);
      
      Serial.print("Connecting to ");
      Serial.print(mqtt_server);
      
      Serial.print(" as ");
      Serial.println(clientName);
    }
      SPI.begin();
      radio.begin();
      network.begin(channel,  this_node);//node address
}


void loop() {    

    if (WiFi.status() != WL_CONNECTED) {
        wifi_connect();
    }

    
    if (!mqtt.connected()) {
        reconnect();
    }


// Check the network regularly    
   network.update(); 
  
// Check for incoming data from the sensors
   incoming_data();
    
   mqtt.loop();

}



void incoming_data(){

     
    while (network.available()) {
      RF24NetworkHeader header;
      payload_t payload;
      network.read(header, &payload, sizeof(payload));

      if(DEBUG_PRINT){      
          Serial.print("Received packet #");
          Serial.print(payload.counter);        
          Serial.print(" at ");
          //Serial.print(payload.ms);        
          //Serial.print(" watt ");      
          Serial.print(payload.power1);
          Serial.print("  ");
          Serial.print(payload.power2);
          Serial.print("  ");
          Serial.println(payload.power3);
      }
      value = payload.power1;
      dtostrf (value, 3, 1, message_buff);
      mqtt.publish(mqttTopic"/POWER1", message_buff);
      delay(100);
      
      value = payload.power2;
      dtostrf (value, 3, 1, message_buff);
      mqtt.publish(mqttTopic"/POWER2", message_buff);
      delay(100);
          
      value = payload.power3;
      dtostrf (value, 3, 1, message_buff);
      mqtt.publish(mqttTopic"/POWER3", message_buff);      
      delay(100);

      publish_cnt++;
      dtostrf (publish_cnt, 2, 1, message_buff);
      mqtt.publish(mqttTopic"/STATUS", message_buff);

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





void LED_RUN(){
  digitalWrite(LED_GREEN, HIGH);  delay(250);
  digitalWrite(LED_GREEN, LOW);   delay(250);
}


void LED_WIFI(){
  digitalWrite(LED_GREEN, HIGH);  delay(250);
  digitalWrite(LED_GREEN, LOW);   delay(250);
}


void LED_PAYLOAD(){
  digitalWrite(LED_RED, HIGH);  delay(100);
  digitalWrite(LED_RED, LOW);   delay(100);
}

void LED_ERR(){
  digitalWrite(LED_RED, HIGH);  delay(250);
  digitalWrite(LED_RED, LOW);   delay(250);
}


