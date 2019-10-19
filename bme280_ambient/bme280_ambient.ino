
#include <ESP8266WiFi.h>
#include "Ambient.h"

#define USE_SERIAL Serial

#define _DEBUG 0
#if _DEBUG
#define DBG(...) { USE_SERIAL.print(__VA_ARGS__); }
#else
#define DBG(...)
#endif

/* ---------------------------------------- */
/* for RTC Memory */
/* ---------------------------------------- */
extern "C"{
    #include "user_interface.h"
}

typedef struct{
  int data1;
  int data2;
} rtcStore;

rtcStore rtcMem;
#define RTCMEMORYSTART  64
#define RTCMEMORYLEN    128
int rtcBuffer[RTCMEMORYLEN]={0};

/* ---------------------------------------- */
/* for BME280 */
/* ---------------------------------------- */
#define BME280_ADDRESS 0x76
#define SENDCOUNT 6
double temp_act = 0.0, press_act = 0.0,hum_act=0.0;
extern void initBME280();
extern void readBME280(double *temp_act, double *press_act, double *hum_act);

/* ---------------------------------------- */
/* Wifi */
/* ---------------------------------------- */
const char *ssid = "XXXXXXX-X-XXXX";
const char *password = "YYYYYYYYYYYY";
WiFiClient client;

/* ---------------------------------------- */
/* Ambient */
/* ---------------------------------------- */
unsigned int channelId = 99999;
const char* writeKey = "12345678901234567890";
Ambient ambient;

/* ---------------------------------------- */
/* Setup */
/* ---------------------------------------- */
void setup()
{
    USE_SERIAL.begin(115200);
 
    /* ---------------------------------------- */
    /* RTC Read */
    readRTCdata();
    int count = rtcBuffer[0];
    int sendfrag=0;
    double temp_sum =(double)rtcBuffer[1]/100.;
    double press_sum=(double)rtcBuffer[2]/100.;
    double hum_sum  =(double)rtcBuffer[3]/100.;

    /* ---------------------------------------- */
    /* Read Data from BME280 */
    readBME280(&temp_act, &press_act, &hum_act);
    temp_sum  = temp_sum  + temp_act;
    press_sum = press_sum + press_act;
    hum_sum   = hum_sum   + hum_act;

    /* ---------------------------------------- */
    /* send Data to Ambient */
    count=count-1;
    if(count<=0){
       count = SENDCOUNT;
       sendfrag = 1;
    }

    if(sendfrag){
      openWifi();
      openAmbient();
      printBME280();
      temp_act  =temp_sum/SENDCOUNT;
      press_act =press_sum/SENDCOUNT;
      hum_act   =hum_sum/SENDCOUNT;
      sendBME280();
      temp_sum  = 0.;
      press_sum = 0.;
      hum_sum   = 0.;
    }

    /* ---------------------------------------- */
    /* RTC Write */
    rtcBuffer[0] = count;
    rtcBuffer[1] = (int)(temp_sum*100.);
    rtcBuffer[2] = (int)(press_sum*100.);
    rtcBuffer[3] = (int)(hum_sum*100.);

    readRTCdata();

    /* ---------------------------------------- */
    /* RTC Sleep */
    ESP.deepSleep(10*60e6);   //10minitus
    delay(100);
}

void loop()
{
}

/* Debug Print */
void printBME280()
{
  DBG("Temperature : ");
  DBG(temp_act);
  DBG("\n");
  DBG("humid: ");
  DBG(hum_act);
  DBG("\n");
  DBG("pressure : ");
  DBG(press_act);
  DBG("\n");
}

void openWifi()
{
  /* ---------------------------------------- */
  /* Wifi */
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
     DBG(".");
    if(i>100){ 
    ESP.deepSleep(10*60e6);   //10minitus
    delay(100);
    }
  }

  DBG("WiFi connected\r\n");
  DBG("IP address: ");
  DBG(WiFi.localIP());
  DBG("\r\n");  
}

void openAmbient()
{
  /* ---------------------------------------- */
  /* Ambient */
  int irtn =false;
  int i=0;

  while(irtn!=true){
    irtn = ambient.begin(channelId, writeKey, &client);
    USE_SERIAL.println("ambient begin: " + String(irtn) + "\r\n");
    delay(100);
    i++;
    if(i>100){ 
      ESP.deepSleep(10*60e6);   //10minitus
      delay(100);
    }
  }  
}

void sendBME280()
{
  ambient.set(1, temp_act);
  ambient.set(2, hum_act);
  ambient.set(3, press_act);
  ambient.send();
}  

void writeRTCdata()
{
  int buckets = sizeof(rtcStore)/sizeof(int);
  int i, rtcPos;
  for(i=0; i<RTCMEMORYLEN/buckets; i++){
    rtcPos = RTCMEMORYSTART+i*buckets;
    rtcMem.data1 = rtcBuffer[i*buckets];
    rtcMem.data2 = rtcBuffer[i*buckets+1];
    system_rtc_mem_write(rtcPos,&rtcMem,buckets*4);
  }
}

void readRTCdata()
{
  int buckets = sizeof(rtcStore)/sizeof(int);
  int i, rtcPos;
  for(i=0; i<RTCMEMORYLEN/buckets; i++){
    rtcPos = RTCMEMORYSTART+i*buckets;
    system_rtc_mem_write(rtcPos,&rtcMem,sizeof(rtcStore));
    rtcBuffer[i*buckets] = rtcMem.data1;
    rtcBuffer[i*buckets+1] = rtcMem.data2;
  }    
}
