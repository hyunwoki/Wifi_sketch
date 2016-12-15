#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <IRremoteESP8266.h>
#include "ap_setting.h"
#include "ir_code.h"


#define NODE_1
#define DEBUG_PRINT 1
#define mqtt_server    "211.180.34.28"
#define mqtt_user      "your_username"
#define mqtt_password  "your_password"

#define mqttClientId          "DEVICE2"
#define mqttClientNode        "1"
#define mqttDomain            "SKMT"
#define mqttTopic             mqttDomain"/"mqttClientId
#define mqttLwtTopic          mqttDomain"/"mqttClientId"/LWT"
#define humidity_topic        "sensor/humidity"
#define temperature_topic     "sensor/temperature"



#ifdef NODE_1
       #define RELAY_A     mqttDomain"/"mqttClientId"/RELAY/RELAY1"
       #define RELAY_B     mqttDomain"/"mqttClientId"/RELAY/RELAY2"
#else
       #define RELAY_A     mqttDomain"/"mqttClientId"/RELAY/RELAY3"
       #define RELAY_B     mqttDomain"/"mqttClientId"/RELAY/RELAY4"
#endif


int  mqttLwtQos = 0;
int  mqttLwtRetain = true;
char *REALAY_A = 0;
char *REALAY_B = 0;
char *HUMIDITY_A = 0;
char *TEMPERATURE_B = 0;
char message_buff[100];
String clientName;
unsigned long fast_update, slow_update;
unsigned long lastReconnectAttempt = 0;

 

// SWITCH_STATE
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
#define IR_IN     10
#define IR_OUT    5   // GPIO pin 5 (D1 of NodeMCU, WeMos D1 Mini)




//WIFI ETHENET
WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, mqtt_callback, espClient);


// DHT22
DHT dht = DHT(DHT_PIN, DHT22);


// IR
IRrecv irrecv(IR_IN);
IRsend irsend(IR_OUT);



// IR power status
int PowerStatus;



void setup() {

  // start serial
  if (DEBUG_PRINT) {
    Serial.begin (115200);
    Serial.println();
    Serial.println("IR controller Starting");
  }    

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);  
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(btn1, INPUT);
    pinMode(btn2, INPUT);   
    delay(20);
    
    LED_TEST();
    Serial.print("Client : ");
    Serial.println(mqttClientId);
    Serial.print("NODE : ");
    Serial.println(mqttClientNode);

    
    dht.begin();
    irsend.begin();    
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



}


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
      Serial.print(". ");
      Serial.print(Attempt);      
      if (Attempt == 50)
      {
        Serial.println();
        Serial.println("Could not connect to WIFI");
        ESP.restart();
        delay(200);
      }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_GREEN, HIGH);
}  




void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
     
       Serial.print("Attempting MQTT connection...");
       if (mqtt.connect((char*) clientName.c_str(), mqttLwtTopic, mqttLwtQos, mqttLwtRetain, "0")) {
            byte data[] = { '1' };
            mqtt.subscribe(mqttTopic"/IR/#");    
            mqtt.subscribe(mqttTopic"/RELAY/#");    
            mqtt.publish(mqttLwtTopic, data, 1, mqttLwtRetain);   //LTW        
            mqtt.publish(mqttTopic"/ID", mqttClientId);           // mqttClientId    
            Serial.println("connected");          
            } 
        else {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" try again in 1 seconds");
            // Wait 1 LED_ERR
            LED_ERR();
        }
            btnState1();
            btnState2();        
  }
}



void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  
      int khz=38;       //NB Change this default value as neccessary to the correct carrier frequency
      int i = 0;
      for(i=0; i<length; i++) {
      message_buff[i] = payload[i];
      LED_PAYLOAD();
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

     if (String(topic) == mqttTopic"/IR/POWER") {
      if (msgString == "1") {            
             
              irsend.sendRaw(turn_on, sizeof(turn_on)/sizeof(int), khz);          
            }
       else {                          
              irsend.sendRaw(turn_off, sizeof(turn_off)/sizeof(int), khz);   
              
            }
         }
}





void loop() {
      
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connect();
    }
    
    if (!mqtt.connected()) {
        reconnect();
    }
    
    if ((millis()-fast_update)>60000) {
         fast_update = millis();         
         Temperature_update();
    }
 
    Temperature();
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





void LED_TEST(){
  digitalWrite(LED_RED, HIGH);
  delay(200);
  digitalWrite(LED_RED, LOW);
  delay(200);
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  delay(200);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

void LED_ERR(){
  digitalWrite(LED_RED, HIGH);
  delay(250);
  digitalWrite(LED_RED, LOW);
  delay(250);
}


void LED_RUN(){
  digitalWrite(LED_GREEN, HIGH);
  delay(250);
  digitalWrite(LED_GREEN, LOW);
  delay(250);
}


void LED_WIFI(){
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  delay(200);
}

void LED_DATA(){
  for(int i=0; i < 3; i++){
  digitalWrite(LED_RED, HIGH);
  delay(100);
  digitalWrite(LED_RED, LOW);
  delay(100);    
  }
}

void LED_PAYLOAD(){
  for(int i=0; i < 1; i++){
  digitalWrite(LED_RED, HIGH);
  delay(100);
  digitalWrite(LED_RED, LOW);
  delay(100);    
  }
}

void LED_SW(){
  for(int i=0; i < 1; i++){
  digitalWrite(LED_RED, HIGH);
  delay(100);
  digitalWrite(LED_RED, LOW);
  delay(100);    
  }
}


