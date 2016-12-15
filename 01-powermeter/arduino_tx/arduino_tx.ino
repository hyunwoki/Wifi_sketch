#include <SPI.h>
#include <RF24Network.h>
#include <RF24.h>
#include "EmonLib.h"
#include <LCD5110_Graph_SPI.h>

#define LED_RED 10
#define LED_GREEN 9

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(7, 8);
RF24Network network(radio);          // Network uses that radio
const uint16_t this_node = 00;       // Address of our node in Octal format ( 04,031, etc)
const uint16_t other_node = 01;      // Address of the other node in Octal format
const uint16_t channel = 8;         // channel

const unsigned long interval = 5000;      //ms  // How often to send 'hello world to the other unit
unsigned long last_sent;                  // When did we last send?
unsigned long packets_sent;               // How many have we sent already

LCD5110 myGLCD(5,6,3);
extern unsigned char SmallFont[];


// Structure of our payload
struct payload_t {                 
  //unsigned long ms;
  unsigned long counter;
  unsigned long power1;
  unsigned long power2;
  unsigned long power3;
};

payload_t emontx;


// Create  instances for each CT channel
  EnergyMonitor ct1,ct2,ct3;                                              
  const int CT1 = 1; 
  const int CT2 = 1;                                                      
  const int CT3 = 1;



void setup() {

  Serial.begin(115200);
  Serial.println("RF24Network/EnergyMonitor_rx/");
   
  Serial.print("Rx_Node: "); 
  Serial.println(this_node);   
  
  Serial.print("Tx_Node: "); 
  Serial.println(other_node); 
  
  //printf_begin();
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  
// Setup emonTX CT channel (channel, calibration)
// Calibration factor = CT ratio / burden resistance  
  if (CT1) ct1.current(0, 30.606);                                     
  if (CT2) ct2.current(1, 30.606);                                     
  if (CT3) ct3.current(2, 30.606); 
  
  radio.begin();
  network.begin(channel , this_node);

}



void loop() {
    
    // Check the network regularly
      network.update();   
                     
    // Check the CT-SNESOR
      ct_sensor();



   unsigned long now = millis();              // If it's time to send a message, send it!
   
    if ( now - last_sent >= interval  )
    {
      last_sent = now;      

      Serial.print("Sending packet #");
      Serial.print(packets_sent);
      Serial.print(" at ");
      Serial.print("Sending...");
      
      Nokia();
      //payload_t payload = { millis(), packets_sent++, emontx.power1, emontx.power2, emontx.power3  };     
      payload_t payload = { packets_sent++, emontx.power1, emontx.power2, emontx.power3  };     
      RF24NetworkHeader header(/*to node*/ other_node);
      bool ok = network.write(header,&payload,sizeof(payload));
      if (ok){
        Serial.println("ok."); 
        digitalWrite(LED_GREEN, HIGH); delay(250); digitalWrite(LED_GREEN, LOW);
      } else{
        Serial.println("failed.");
        digitalWrite(LED_RED, HIGH); delay(250); digitalWrite(LED_RED, LOW);      
    }
  }
}

void ct_sensor(){
//ct.calcIrms(number of wavelengths sample)*AC RMS voltage

    if (CT1) {
      emontx.power1 = ct1.calcIrms(1480) * 240.0;                           
      //Serial.print(emontx.power1);                                         
    }
    
    if (CT2) {
      emontx.power2 = ct2.calcIrms(1480) * 240.0;     
      //Serial.print(" "); Serial.print(emontx.power2);
    } 
  
    if (CT3) {
      emontx.power3 = ct3.calcIrms(1480) * 240.0;     
      //Serial.print(" "); Serial.println(emontx.power3);
    } 
    
}



void Nokia()
{
    
    myGLCD.setFont(SmallFont);
    myGLCD.clrScr();
    char tbuf[6];

    myGLCD.print("WATT POWER", CENTER, 0);

    dtostrf(emontx.power1, 4, 0, tbuf);
    myGLCD.print("POWER1 :", LEFT, 15);
    myGLCD.print(tbuf, 47, 15);
    myGLCD.print("W", RIGHT, 15);

    dtostrf(emontx.power2, 4, 0, tbuf);
    myGLCD.print("POWER2 :", LEFT, 25);
    myGLCD.print(tbuf, 47, 25);
    myGLCD.print("W", RIGHT, 25);

    dtostrf(emontx.power3, 4, 0, tbuf);
    myGLCD.print("POWER3 :", LEFT, 35);
    myGLCD.print(tbuf, 47, 35);
    myGLCD.print("W", RIGHT, 35);
    myGLCD.update();
    
}





