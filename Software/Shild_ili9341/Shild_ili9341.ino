#define MY_DEBUG
#define DEBUG
//#define MY_DISABLED_SERIAL

// Enable passive mode
//#define MY_PASSIVE_NODE
#define MY_TRANSPORT_UPLINK_CHECK_DISABLED 
#define MY_TRANSPORT_WAIT_READY_MS 1500
#define MY_RADIO_NRF24
#define MY_RF24_IRQ_PIN   2

#define PIN_NRF_POWER     8
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC
#define MY_NODE_ID        12
#define MY_RF24_CE_PIN    9
#define MY_RF24_CS_PIN    10

#define MY_RF24_POWER_PIN (PIN_NRF_POWER)
#define MY_RF24_CHANNEL   4
#define MY_RF24_PA_LEVEL (RF24_PA_MAX)
#include <MySensors.h>  
#include <TimeLib.h>  

// Define sensor node childs
#define CHILD_ID_TEMP     0  //Температура в комнате
#define CHILD_ID_HUM      1  //Влажность в комнате 
#define CHILD_ID_LIGHT    2  //Влажность в комнате 
#define CHILD_ID_STAT     10 //Канал выдачи состояния
#define CHILD_ID_OUTDOOR  11 //Погода от датчиков на улице 
#define CHILD_ID_FORECAST 12 //Прогноз погоды
#define TIME_SLEEP        300000

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgStat(CHILD_ID_STAT, V_CUSTOM);
MyMessage msgOutdoor(CHILD_ID_OUTDOOR, V_TEXT);
MyMessage msgForecast(CHILD_ID_FORECAST, V_TEXT);
MyMessage msgLight(CHILD_ID_LIGHT, V_PERCENTAGE);
 

#include <SPI1.h>
#include "Adafruit_SPITFT.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "logo.h"
#include "DHT.h"
#include "SoftPWM.h"
#include <avr/pgmspace.h>

void(* resetFunc) (void) = 0; // Reset MC function 

#define TFT_DC    16   // (A2)
#define TFT_CS    25   // 25 (A6) 19 (A5)
#define TFT_RESET 17   // 17(A3)
#define TFT_MOSI  26   // (A7)
#define TFT_SCK   15   // (A1)
#define TFT_MISO  14   // (A0)
#define TFT_LED   23   

#define DHT_VCC   7
#define DHT_DATA  6
#define DHT_GND   5
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321 

SOFTPWM_DEFINE_CHANNEL(0, DDRE, PORTE, PORTE0);  //PIN23 PE0 
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(1, 100); 
SOFTPWM_DEFINE_EXTERN_OBJECT_WITH_PWM_LEVELS(1, 100); 
DHT dht(DHT_DATA, DHTTYPE); 

SPI1Class *spi1 = &SPI1;

Adafruit_ILI9341 tft = Adafruit_ILI9341(spi1, TFT_CS, TFT_DC, TFT_RESET);

#define TZ    5
//uint32_t t_cur      = 0, tm;    
//long int t_correct;
uint32_t ms, ms0 = 0, ms1 = 0, ms2 = 0;
uint32_t ms_time = 0;
uint32_t ms_indoor  = 0, ms_outdoor = 0;
int hour_save = 0,min_save = 0;
bool is_time_point = true;
uint16_t dht_error = 0;

float t_indoor = 0, h_indoor = 0;
char s_outdoor[26],s_forecast[26];

void before(){
  pinMode(DHT_GND,OUTPUT);
  digitalWrite(DHT_GND,LOW);
  pinMode(DHT_VCC,OUTPUT);
  digitalWrite(DHT_VCC,HIGH);
  pinMode(TFT_LED,OUTPUT);
  digitalWrite(TFT_LED,HIGH);  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.drawBitmap(200,0,bitmap,imageWidth, imageHeight,ILI9341_WHITE);
  tft.drawLine(0,80,320,80,ILI9341_WHITE);
  tft.setCursor(0, 95);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(2);
  tft.println(utf8rus(PSTR("В комнате:")));
  tft.drawLine(0,120,320,120,ILI9341_WHITE);
  tft.setCursor(0, 135);
  tft.println(utf8rus(PSTR("На улице:")));
  tft.drawLine(0,160,320,160,ILI9341_WHITE);
  tft.setCursor(0, 175);
  tft.println(utf8rus(PSTR("Прогноз:")));
  dht.begin(); 
  Palatis::SoftPWM.begin(60);
  Palatis::SoftPWM.set(0, 100); 
//  delay(2000);
  
}


void presentation(){
   sendSketchInfo("TFT INFORMER", "1.0");
   present(CHILD_ID_TEMP, S_TEMP,"C");
   present(CHILD_ID_HUM,  S_HUM,"%");
   present(CHILD_ID_LIGHT, S_DIMMER);
   present(CHILD_ID_STAT, S_CUSTOM);
   present(CHILD_ID_HUM,  S_INFO);
   present(CHILD_ID_HUM,  S_INFO);
   wait(500);
}


void setup(){
}

void loop(){
    ms = millis();
    if( ms0 == 0 || ms0 > ms || ms - ms0 > 500 ){
       ms0 = ms;
       DisplayTime();
       DisplayTimePoint();     
    }
    
    if( ms1 == 0 || ms1 > ms || ms - ms1 > 5000 ){
       ms1 = ms;
       float h = round(dht.readHumidity());
       float t = round(dht.readTemperature()); 
       if( t < -100.0 || t > 100.0 )dht_error++;
       else DisplayIndoor(t,h);
       Serial.print("T=");
       Serial.print(t,1);
       Serial.print(" H=");
       Serial.println(h,0);
       if( ms_time == 0 || ms_time > ms ||ms - ms_time > 300000 )requestTime();
       if( ms_outdoor == 0 || ms_outdoor > ms || ms - ms_outdoor > 300000 ){
           ms_outdoor = ms;
           send(msgStat.set(random(100)));
       }
//       if( ms_thp  == 0 || ms_thp  > ms ||ms - ms_thp  > 300000 )
//          request(10,0,11);
       if( dht_error > 20 )resetFunc();
    }
}

void DisplayIndoor(float t, float h){
   dht_error = 0;
   if( t == t_indoor && h == h_indoor )return;
   t_indoor = t;
   h_indoor = h;
   tft.fillRect(130,81,190,38,ILI9341_BLACK);
   tft.setCursor(140, 88);
   tft.setTextColor(ILI9341_GREEN);  
   tft.setTextSize(3);
   tft.print(t,1);
   tft.print("C ");
   tft.print(h,0);
   tft.print("%");
   send(msgTemp.set(t_indoor,1));
   send(msgHum.set(h_indoor,0));
   
  
  
}

void DisplayOutdoor(char *s){
   ms_outdoor = millis();
   if( strncmp(s_outdoor,s,25) == 0 )return;
   strncpy(s_outdoor,s,25);
   tft.fillRect(115,121,205,38,ILI9341_BLACK);
   tft.setCursor(115, 128);
   tft.setTextColor(ILI9341_CYAN);  
   tft.setTextSize(3);
   int val[3];
   int n = SplitInt(s,val,3);
   if( n <= 0 )return;
   tft.print(val[0]);   
   tft.print("C");  
   if( n <= 1 )return;       
   tft.print(" ");   
   tft.print(val[1]);   
   tft.print("%");   
   if( n <= 2 )return;       
   tft.print(" ");   
   tft.print(val[2]);   
//  tft.println(utf8rus(PSTR("мм")));   
}

void DisplayForecast(char *s){
   ms_outdoor = millis();
//   send(msgStat.set(1));   
   if( strncmp(s_forecast,s,25) == 0 )return;
   strncpy(s_forecast,s,25);
   int val[7];
   int n = SplitInt(s,val,7);
 //  for( int i=0;i<n; i++){
 //    Serial.println(val[i]);
 //  }  
   tft.fillRect(110,161,210,38,ILI9341_BLACK);
   tft.fillRect(0,190,320,50,ILI9341_BLACK);
   tft.setCursor(110, 175);
   tft.setTextColor(ILI9341_ORANGE);  
   tft.setTextSize(2);  
   if( n <= 0 )return;
   tft.print(val[0]);   
   tft.print("C");  
   if( n <= 1 )return;       
   tft.print("  ");   
   tft.print(val[1]);   
   tft.print("%");   
   if( n <= 2 )return;       
   tft.print("  ");   
   tft.print(val[2]);   
   tft.println(utf8rus(PSTR("мм")));   
   if( n <= 3 )return; 
   switch( val[3] ) {
      case 0 : tft.print(utf8rus(PSTR("ясно")));break; 
      case 1 : tft.print(utf8rus(PSTR("малооблачно")));break; 
      case 2 : tft.print(utf8rus(PSTR("облачно")));break; 
      case 3 : tft.print(utf8rus(PSTR("пасмурно")));break; 
   }
   if( n <= 4 )return; 
   switch( val[4] ) {
      case 4  : tft.println(utf8rus(PSTR(", дождь")));break; 
      case 5  : tft.println(utf8rus(PSTR(", ливень")));break; 
      case 6  : tft.println(utf8rus(PSTR(", град")));break; 
      case 7  : tft.println(utf8rus(PSTR(", снег")));break; 
      case 8  : tft.println(utf8rus(PSTR(", гроза")));break; 
      case 9  : tft.println(utf8rus(PSTR(", хз")));break; 
      case 10 : tft.println(utf8rus(PSTR(", без oсадков")));break; 
   }
   if( n <= 5 )return; 
   tft.print(utf8rus(PSTR("ветер ")));
   switch( val[5] ) {
      case 0  : tft.print(utf8rus(PSTR("северный")));break; 
      case 1  : tft.print(utf8rus(PSTR("С-восточный")));break; 
      case 2  : tft.print(utf8rus(PSTR("восточный")));break; 
      case 3  : tft.print(utf8rus(PSTR("Ю-восточный")));break; 
      case 4  : tft.print(utf8rus(PSTR("южный")));break; 
      case 5  : tft.print(utf8rus(PSTR("Ю-западный")));break; 
      case 6  : tft.print(utf8rus(PSTR("западный")));break; 
      case 7  : tft.print(utf8rus(PSTR("С-западный")));break; 
   }
   if( n <= 6 )return; 
   tft.print(", ");   
   tft.print(val[6]);   
   tft.println(utf8rus(PSTR("м/c")));   
   
}


void DisplayTimePoint(){
  tft.fillRect(83,2,12,50,ILI9341_BLACK);
  tft.setCursor(77, 4);
  tft.setTextColor(ILI9341_YELLOW);  
  tft.setTextSize(6);
  if( is_time_point )tft.print(":");
  else tft.print(" ");
//  Serial.print("point=");
//  Serial.println(is_time_point);
  is_time_point = !is_time_point;   
}

void DisplayTime(){
  int   m = minute();
  int   h = hour();
  if( m == min_save && h == hour_save )return;
  min_save  = m;
  hour_save = h;
  char s[10];
  char point = ':';
//  char point = ' ';
  if( is_time_point )point = ':';
  sprintf(s,"%02d%c%02d",h,point,m);
  tft.fillRect(0,0,200,60,ILI9341_BLACK);
  tft.setCursor(5, 2);
  tft.setTextColor(ILI9341_YELLOW);  
  tft.setTextSize(6);
  tft.println(s);
  tft.fillRect(0,55,320,20,ILI9341_BLACK);
  tft.setCursor(5, 60);
  tft.setTextSize(2);
  tft.print(day());
   switch(month()){
    case 1  : tft.print(utf8rus(PSTR(" января ")));break;
    case 2  : tft.print(utf8rus(PSTR(" февраля ")));break;
    case 3  : tft.print(utf8rus(PSTR(" марта ")));break;
    case 4  : tft.print(utf8rus(PSTR(" апреля ")));break;
    case 5  : tft.print(utf8rus(PSTR(" мая ")));break;
    case 6  : tft.print(utf8rus(PSTR(" июня ")));break;
    case 7  : tft.print(utf8rus(PSTR(" июля ")));break;
    case 8  : tft.print(utf8rus(PSTR(" августа ")));break;
    case 9  : tft.print(utf8rus(PSTR(" сентября ")));break;
    case 10 : tft.print(utf8rus(PSTR(" октября ")));break;
    case 11 : tft.print(utf8rus(PSTR(" ноября ")));break;
    case 12 : tft.print(utf8rus(PSTR(" декабря ")));break;
  }
 tft.print(year());
 switch( weekday() ){
    case 1 : tft.print(utf8rus(PSTR(", воскресенье")));break;
    case 2 : tft.print(utf8rus(PSTR(", пондельник")));break;
    case 3 : tft.print(utf8rus(PSTR(", вторник")));break;
    case 4 : tft.print(utf8rus(PSTR(", среда")));break;
    case 5 : tft.print(utf8rus(PSTR(", четверг")));break;
    case 6 : tft.print(utf8rus(PSTR(", пятница")));break;
    case 7 : tft.print(utf8rus(PSTR(", суббота")));break;
 }
 
//void fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);    
  
}

// This is called when a new time value was received

void receiveTime(uint32_t controllerTime)
{
  uint32_t t = controllerTime+3600*TZ;
#ifdef DEBUG  
  char s[10];
  sprintf(s,"%02d:%02d",hour(),minute());
  Serial.print("Time value received: ");
  Serial.println(s);
#endif  
  setTime( t );
 // t_correct = t - t_cur;
  ms_time = millis();
 
}

void receive(const MyMessage &message) {
    char s[60];
    message.getString(s);
    if( ( message.sender == MY_PARENT_NODE_ID )
         && message.sensor == CHILD_ID_OUTDOOR  ){
          DisplayOutdoor(s);
         }
    else if( ( message.sender == MY_PARENT_NODE_ID )
         && message.sensor == CHILD_ID_FORECAST  ){
          DisplayForecast(s);
         }
    else if( ( message.sender == MY_PARENT_NODE_ID )
         && message.sensor == CHILD_ID_LIGHT  ){
          int level = message.getInt();
          if( level < 0 )level = 0;
          if( level > 100 )level = 100;
          Palatis::SoftPWM.set(0, level); 

         }
   

    
Serial.print("received something: ");
Serial.print("sender= "); // Node ID of the Sender
Serial.print(message.sender);
Serial.print("  type= "); //Message type, the number assigned
Serial.print(message.type); // V_TEMP is 0 zero. etc.
Serial.print(" sensor= "); // Child ID of the Sensor/Device
Serial.print(message.sensor);
Serial.print(" message= "); // This is where the wheels fall off
Serial.println(message.getString(s)); // This works great!
}

char out_buffer[32],in_buffer[32];

char *utf8rus(const char *source)
{
  int i,k,j=0;
  char *target = out_buffer;
  unsigned char n;
//  const char *in_buffer = (const char *)source;
//  char m[2] = { '0', '\0' };
  strncpy_P(in_buffer,source,31);
  k = strlen(in_buffer); i = 0;

  while (i < k) {
    n = in_buffer[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = in_buffer[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
//          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          if (n >= 0x90 && n <= 0xBF) n = n + 0x2F;
          break;
        }
        case 0xD1: {
          n = in_buffer[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
//          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          if (n >= 0x80 && n <= 0x8F) n = n + 0x6F;
          break;
        }
      }
    }
    target[j++] = n;
    if(j>=31)break;
//    m[0] = n; target = target + String(m);
  }
  target[j] = '\0';
  return target;
} 

int SplitInt(char *s, int *val, int num ){
   int len = strlen(s);
   int count = 0;
   char *p = s;
   for( int i=0; i<len; i++ ){
       if( s[i] == '|' ){
          s[i] = '\0';
          val[count++] = atoi(p);
          p = s+i+1;
          if( count >= num )break;
       }     
   }
   val[count++] = atoi(p); 
    return count;
}
