//-------------------DHT22. Parameters------------------------------

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;
int flag = 1;

bool checkBound(float newValue, float prevValue, float maxDiff) 
{return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;}


void Temperature() {

  
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();


    if (isnan(newTemp) || isnan(newHum)) {
       Serial.println("Failed to read from DHT");    
       return;
    }

    
    if (checkBound(newTemp, temp, diff)) {    
      temp = newTemp;             
      mqtt.publish(HUMIDITY_A, FtoS(temp).c_str(), true);      
     }   
         
    if (checkBound(newHum, hum, diff)) {
      hum = newHum;         
      mqtt.publish(TEMPERATURE_B, FtoS(hum).c_str(), true);       
     }
 
   }
}



// float형 숫자, 소수부분이 0인경우 정수로, 소수부분이 0이 아닌
// 숫자라면 소수점 이하 첫째자리까지만 표시

static String FtoS(float num) {
String str = (String) num;

  if ((int) num == num) {
    return str.substring(0, str.indexOf(".") + 2);
  } else {
    return str.substring(0, str.indexOf(".") + 2);
  }
}

