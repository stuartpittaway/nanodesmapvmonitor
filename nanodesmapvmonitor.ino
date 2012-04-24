/*
 NANODE SMA PV MONITOR
 
 See README for release notes/comments! 
 Latest version found at https://github.com/stuartpittaway/nanodesmapvmonitor
 
 
 Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0) 
 http://creativecommons.org/licenses/by-nc-sa/3.0/
 
 You are free:
 to Share — to copy, distribute and transmit the work
 to Remix — to adapt the work
 Under the following conditions:
 Attribution — You must attribute the work in the manner specified by the author or licensor (but not in any way that suggests that they endorse you or your use of the work).
 Noncommercial — You may not use this work for commercial purposes.
 Share Alike — If you alter, transform, or build upon this work, you may distribute the resulting work only under the same or similar license to this one. 
 
 All code is copyright Stuart Pittaway, (c)2012.
 */


//Need to change SoftwareSerial/NewSoftSerial.h file to set buffer to 128 bytes or you will get buffer overruns!
//Find the line below and change
//#define _NewSS_MAX_RX_BUFF 128 // RX buffer size

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Time.h> 
#include <EtherCard.h>
#include <NanodeMAC.h>  // If using a Nanode (www.nanode.eu) instead of Arduino and ENC28J60 EtherShield then use this:

#include "WebSiteService.h"
#include "Bluetooth.h"
#include "SMANetArduino.h"
#include "APIKey.h"


#undef debugMsgln 
#define debugMsgln(s) (__extension__(  {Serial.println(F(s));}  ))
//#define debugMsgln(s) (__extension__(  {__asm__("nop\n\t"); }  ))

#undef debugMsg
#define debugMsg(s) (__extension__(  {Serial.print(F(s));}  ))
//#define debugMsg(s) (__extension__(  { __asm__("nop\n\t");  }  ))

//Do we switch off upload to sites when its dark?
//#define allowsleep

byte Ethernet::buffer[650]; // tcp/ip send and receive buffer

static unsigned long currentvalue=0;
static unsigned int valuetype=0;
static unsigned long value = 0;
static unsigned long spotpowerac=0;
static unsigned long spotpowerdc=0;
static time_t datetime=0;

Stash stash;  //Shared by WebService classes

prog_uchar PROGMEM smanet2packetx80x00x02x00[] ={0x80, 0x00, 0x02, 0x00};

prog_uchar PROGMEM smanet2packet2[]  ={ 
  0x80, 0x0E, 0x01, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
prog_uchar PROGMEM SMANET2header[] = {  
  0xFF,0x03,0x60,0x65  };
prog_uchar PROGMEM InverterCodeArray[] = {   
  0x5c, 0xaf, 0xf0, 0x1d, 0x50, 0x00 };  //SCAFFOLDS000  meaningless really!  This is our fake address on the SMA NETWORK
prog_uchar PROGMEM fourzeros[]= {
  0,0,0,0};
prog_uchar PROGMEM smanet2packet6[]={      
  0x54, 0x00, 0x22, 0x26, 0x00, 0xFF, 0x22, 0x26, 0x00};


//Password needs to be 12 bytes long, with zeros as trailing bytes (Assume SMA INVERTER PIN code is 0000)
unsigned char SMAInverterPasscode[]={
  '0','0','0','0',0,0,0,0,0,0,0,0};

void setup() 
{ 
  Serial.begin(115200);          //Serial port for debugging output

  pinMode(RED_LED, OUTPUT);
  digitalWrite( RED_LED, LOW);

  //HowMuchMemory();

  //Make sure you have the latest version of NanodeMAC which works on Arduino 1.0
  byte mymac[] = { 
    0,0,0,0,0,0                             };
  NanodeMAC mac( mymac );
  //printMacAddress(mymac);

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
  { 
    //debugMsgln("Fail chip"); 
    error(); 
  }

  debugMsg("DHCP ");
  if (ether.dhcpSetup()) {
    ether.printIp("IP:", ether.myip);
    //ether.printIp("GW:", ether.gwip);
    //ether.printIp("DNS:", ether.dnsip);
  } 
  else { 
    //debugMsgln("DHCP fail"); 
    error(); 
  }
  //debugMsgln("Done");

  HowMuchMemory();


  //  es.ES_enc28j60PowerDown();
  //  es.ES_enc28j60PowerUp();
  
  
} 


void loop() 
{ 
  debugMsgln("loop");

  //DNS lookup is done here otherwise the Serial buffer gets overflowed by the inverter trying to send broadcast messages out
  webservicePachube pachube;
  pachube.begin();

  webserviceSolarStats solarstats;
  solarstats.begin();

  webservicePVoutputOrg pvoutputorg; 
  pvoutputorg.begin();

  digitalWrite( RED_LED, HIGH);

  BTStart();

  initialiseSMAConnection();

  //Dont really need this...
  //InquireBlueToothSignalStrength();

  logonSMAInverter();

  setSyncInterval(3600);  //1 hour
  setSyncProvider(setTimePeriodically);

  //  webserviceemonCMS emoncms;
  //  emoncms.begin();

  //We dont actually use the value this returns, but we can set the clock from its reply
  getDailyYield();

  //getInverterName();
  //HistoricData();

  //Default loop
  //debugMsgln("*LOOP*");

  time_t checktime=nextMinute(now());  //Store current time to compare against

  //bool sleeping=false;

  while (1) {
    //debugMsgln("Main loop");

    //HowMuchMemory();

    // DHCP expiration is a bit brutal, because all other ethernet activity and
    // incoming packets will be ignored until a new lease has been acquired
    if (ether.dhcpExpired() && !ether.dhcpSetup())
    {
      //debugMsgln("DHCP fail"); 
      error(); 
    }


    //Populate datetime and spotpowerac variables with latest values
    //getInstantDCPower();
    getInstantACPower();

    //At this point the instant AC power reading could be sent over to an XBEE/RF module
    //to the Open Energy Monitor GLCD display

    if (timeStatus()!=timeSet) {
      //Every hour call getDailyYield to update our local clock to avoid drift...
      getDailyYield();   
    }

    digitalClockDisplay( now() );
    debugMsgln("");

    if (now()>=checktime) 
    {
      //debugMsgln("Upload time");

//      if (sleeping) {
//        //Force reboot of chip to prevent chip from hanging every 3 or 4 days.
//        sleeping=false;
//        softReset();  
//      }

      //This routine is called as soon as possible after the clock ticks past the minute
      //its important to get regular syncronised readings from the solar, or you'll end
      //up with jagged graphs as PVOutput and SolarStats have a minute granularity and truncate seconds.

      getTotalPowerGeneration();

      //Upload value to various websites/services, depending upon frequency
      solarstats.CountDownAndUpload(currentvalue,spotpowerac,spotpowerdc,datetime);     
      pvoutputorg.CountDownAndUpload(currentvalue,spotpowerac,spotpowerdc,datetime);
      pachube.CountDownAndUpload(currentvalue,spotpowerac,spotpowerdc,datetime);
      //emoncms.CountDownAndUpload(currentvalue,spotpowerac,datetime);

      //finally, set waiting time...
      checktime=nextMinute(now());


#ifdef allowsleep
      if ( (now() > (datetime+3600)) && (spotpowerac==0)) {
        //Inverter output hasnt changed for an hour, so put Nanode to sleep till the morning
        //debugMsgln("Bed time");

        //sleeping=true;

        //Get midnight on the day of last solar generation
        tmElements_t tm;     
        tm.Year = year(datetime)-1970;
        tm.Month = month(datetime);
        tm.Day = day(datetime);
        tm.Hour = 23;
        tm.Minute = 59;
        tm.Second = 59;
        time_t midnight=makeTime(tm);

        //Move to midnight
        //debugMsg("Midnight ");
        //digitalClockDisplay( midnight );
        //debugMsgln("");

        if (now() < midnight) {
          //Time to calculate SLEEP time, we only do this if its BEFORE midnight
          //on the day of solar generation, otherwise we might wake up and go back to sleep immediately!

          //Workout what time sunrise is and sleep till then...
          //longitude, latitude (london uk=-0.126236,51.500152)
          unsigned int minutespastmidnight=ComputeSun(mylongitude,mylatitude,datetime,true);

          //We want to wake up at least 15 minutes before sunrise, just in case...
          checktime=midnight + minutespastmidnight - 15*60;
        }
      }
#endif      

      //debugMsg("Wait for ");
      //digitalClockDisplay( checktime );
      //debugMsgln("");

    }

    //Delay for approx. 4 seconds between instant AC power readings
    //debugMsgln("Ether loop");
    for(int i=1; i<=60; i++) {
      ether.packetLoop(ether.packetReceive());     
      delay(50);
    }

  }//end while
} 

time_t nextMinute(time_t t) {
  t+=(60-second(t));  //Make sure that the check time is on the next minute (zero seconds)
  return t;
}


prog_uchar PROGMEM smanet2totalyieldWh[]=  {  
  0x54, 0x00, 0x01, 0x26, 0x00, 0xFF, 0x01, 0x26, 0x00};

void getTotalPowerGeneration() {

  //Gets the total kWh the SMA inverterhas generated in its lifetime...
  do {
    writePacketHeader(level1packet);
    //writePacketHeader(level1packet,0x01,0x00,smaBTInverterAddressArray);
    writeSMANET2PlusPacket(level1packet,0x09, 0xa0, packet_send_counter, 0, 0, 0);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packetx80x00x02x00,sizeof(smanet2packetx80x00x02x00));
    writeSMANET2ArrayFromProgmem(level1packet,smanet2totalyieldWh,sizeof(smanet2totalyieldWh));
    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);

    sendPacket(level1packet);

    waitForMultiPacket(0x0001);
  }
  while (!validateChecksum());
  packet_send_counter++;

  //displaySpotValues(16);

  memcpy(&datetime,&level1packet[40+1+4],4);
  memcpy(&value,&level1packet[40+1+8],3);

  //digitalClockDisplay(datetime);
  //debugMsg('=');Serial.println(value);
  currentvalue=value;
}

static time_t setTimePeriodically() {
  debugMsgln("Fake time sync");
  //Just fake the syncTime functions of time.h
  return 0;
}

prog_uchar PROGMEM smanet2packet99[]= {0x00,0x04,0x70,0x00};
prog_uchar PROGMEM smanet2packet0[]=  {0x01,0x00,0x00,0x00};

void initialiseSMAConnection() {

  //Wait for announcement/broadcast message from PV inverter
  waitForPacket(0x0002);

  //Extract data from the 0002 packet
  unsigned char netid=level1packet[4];

  //debugMsg("sma netid=");Serial.println(netid,HEX);

  writePacketHeader(level1packet,0x02,0x00,smaBTInverterAddressArray);
  writeSMANET2ArrayFromProgmem(level1packet,smanet2packet99,sizeof(smanet2packet99));
  writeSMANET2SingleByte(level1packet,netid);
  writeSMANET2ArrayFromProgmem(level1packet,fourzeros,sizeof(fourzeros));
  writeSMANET2ArrayFromProgmem(level1packet,smanet2packet0,sizeof(smanet2packet0));
  writePacketLength(level1packet);
  sendPacket(level1packet);
  
  waitForPacket(0x000a);
  while ((cmdcode!=0x000c) && (cmdcode!=0x0005)) {
    readLevel1PacketFromBluetoothStream(0);
  }

  if (cmdcode!=0x0005) {
    waitForPacket(0x0005);
  }

  //debugMsgln("If we get this far, things are going nicely...."));

  do {
    //First SMANET2 packet
    writePacketHeader(level1packet,sixff);
    //writePacketHeader(level1packet,0x01,0x00,sixff);
    writeSMANET2PlusPacket(level1packet,0x09, 0xa0, packet_send_counter, 0, 0, 0);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packetx80x00x02x00,sizeof(smanet2packetx80x00x02x00));
    writeSMANET2SingleByte(level1packet,0x00);
    writeSMANET2ArrayFromProgmem(level1packet,fourzeros,sizeof(fourzeros));
    writeSMANET2ArrayFromProgmem(level1packet,fourzeros,sizeof(fourzeros));
    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);
    sendPacket(level1packet);

    waitForPacket(0x0001);
  } 
  while (!validateChecksum());
  packet_send_counter++;

  //Second SMANET2 packet
  writePacketHeader(level1packet,sixff);
  //writePacketHeader(level1packet,0x01,0x00,sixff);
  writeSMANET2PlusPacket(level1packet,0x08, 0xa0, packet_send_counter, 0x00, 0x03, 0x03);
  writeSMANET2ArrayFromProgmem(level1packet,smanet2packet2,sizeof(smanet2packet2));
  
  writeSMANET2PlusPacketTrailer(level1packet);
  writePacketLength(level1packet);
  sendPacket(level1packet);
  packet_send_counter++;
  //No reply for this message...
}


prog_uchar PROGMEM smanet2packet_logon[]={ 
  0x80, 0x0C, 0x04, 0xFD, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00,0xaa,0xaa,0xbb,0xbb};

void logonSMAInverter() {
  //Third SMANET2 packet
  debugMsg("*Logon ");
  do {
    writePacketHeader(level1packet,sixff);
    //writePacketHeader(level1packet,0x01,0x00,sixff);
    writeSMANET2PlusPacket(level1packet,0x0e, 0xa0, packet_send_counter, 0x00, 0x01, 0x01);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packet_logon,sizeof(smanet2packet_logon));
    writeSMANET2ArrayFromProgmem(level1packet,fourzeros,sizeof(fourzeros));

    //INVERTER PASSWORD - only 4 digits, although it appears packet can hold longer
    for(int passcodeloop=0;passcodeloop<sizeof(SMAInverterPasscode);passcodeloop++) {
      writeSMANET2SingleByte(level1packet,(SMAInverterPasscode[passcodeloop] + 0x88) % 0xff);
    }

    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);
    sendPacket(level1packet);

    waitForPacket(0x0001);

    //dumpPacket('R');
  } 
  while (!validateChecksum());
  packet_send_counter++;
  debugMsgln("Done");

}

void getDailyYield() {
  //Yield
  //We expect a multi packet reply to this question...
  //We ask the inverter for its DAILY yield (generation)
  //once this is returned we can extract the current date/time from the inverter and set our internal clock
  do {
    writePacketHeader(level1packet);
    //writePacketHeader(level1packet,0x01,0x00,smaBTInverterAddressArray);
    writeSMANET2PlusPacket(level1packet,0x09, 0xa0, packet_send_counter, 0, 0, 0);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packetx80x00x02x00,sizeof(smanet2packetx80x00x02x00));
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packet6,sizeof(smanet2packet6));
    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);

    sendPacket(level1packet);

    waitForPacket(0x0001);
  } 
  while (!validateChecksum());
  packet_send_counter++;

  //displaySpotValues(16);
  valuetype = level1packet[40+1+1]+level1packet[40+2+1]*256;

  //Serial.println(valuetype,HEX);
  //Make sure this is the right message type
  if (valuetype==0x2622) {  
    memcpy(&value,&level1packet[40+8+1],3);
    //0x2622=Day Yield Wh
    memcpy(&datetime,&level1packet[40+4+1],4);  
    setTime(datetime);  
  }
}

prog_uchar PROGMEM smanet2acspotvalues[]=  {  
  0x51, 0x00, 0x3f, 0x26, 0x00, 0xFF, 0x3f, 0x26, 0x00, 0x0e};

void getInstantACPower() 
{
  //Get spot value for instant AC wattage
  do {
    writePacketHeader(level1packet);
    //writePacketHeader(level1packet,0x01,0x00,smaBTInverterAddressArray);
    writeSMANET2PlusPacket(level1packet,0x09, 0xA1, packet_send_counter, 0, 0, 0);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packetx80x00x02x00,sizeof(smanet2packetx80x00x02x00));
    writeSMANET2ArrayFromProgmem(level1packet,smanet2acspotvalues,sizeof(smanet2acspotvalues));
    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);

    sendPacket(level1packet);
    waitForMultiPacket(0x0001);
  }
  while (!validateChecksum());
  packet_send_counter++;

  //value will contain instant/spot AC power generation along with date/time of reading...
  memcpy(&datetime,&level1packet[40+1+4],4);
  memcpy(&value,&level1packet[40+1+8],3);
  debugMsg("AC ");
  digitalClockDisplay(datetime);
  debugMsg(" Pwr=");
  Serial.println(value);
  spotpowerac=value;

  //displaySpotValues(28);
}


//Returns volts + amps
//prog_uchar PROGMEM smanet2packetdcpower[]={  0x83, 0x00, 0x02, 0x80, 0x53, 0x00, 0x00, 0x45, 0x00, 0xFF, 0xFF, 0x45, 0x00 };
// Just DC Power (watts)
prog_uchar PROGMEM smanet2packetdcpower[]={  0x83, 0x00, 0x02, 0x80, 0x53, 0x00, 0x00, 0x25, 0x00, 0xFF, 0xFF, 0x25, 0x00 };
void getInstantDCPower() {

  //DC
  //We expect a multi packet reply to this question...
  do {
    writePacketHeader(level1packet);
    //writePacketHeader(level1packet,0x01,0x00,smaBTInverterAddressArray);
    writeSMANET2PlusPacket(level1packet,0x09, 0xE0, packet_send_counter, 0, 0, 0);
    writeSMANET2ArrayFromProgmem(level1packet,smanet2packetdcpower,sizeof(smanet2packetdcpower));
    writeSMANET2PlusPacketTrailer(level1packet);
    writePacketLength(level1packet);

    sendPacket(level1packet);
    waitForMultiPacket(0x0001);
  }
  while (!validateChecksum());
  packet_send_counter++;

  //displaySpotValues(28);

  //float volts=0;
  //float amps=0;

  for(int i=40+1;i<packetposition-3;i+=28){
    valuetype = level1packet[i+1]+level1packet[i+2]*256;
    memcpy(&value,&level1packet[i+8],3);

    //valuetype 
    //0x451f=DC Voltage  /100
    //0x4521=DC Current  /1000
    //0x251e=DC Power /1
    //if (valuetype==0x451f) volts=(float)value/(float)100;
    //if (valuetype==0x4521) amps=(float)value/(float)1000;
    if (valuetype==0x251e) spotpowerdc=value;

    memcpy(&datetime,&level1packet[i+4],4);
  }

  //spotpowerdc=volts*amps;

  debugMsg("DC ");
  digitalClockDisplay(datetime);
  //debugMsg(" V=");Serial.print(volts);debugMsg("  A=");Serial.print(amps);
  debugMsg(" Pwr=");
  Serial.println(spotpowerdc);  
}

/*
//Inverter name
prog_uchar PROGMEM smanet2packetinvertername[]={   0x80, 0x00, 0x02, 0x00, 0x58, 0x00, 0x1e, 0x82, 0x00, 0xFF, 0x1e, 0x82, 0x00};  

 void getInverterName() {
 
 do {
 //INVERTERNAME
 debugMsgln("InvName"));
 writePacketHeader(level1packet,sixff);
 //writePacketHeader(level1packet,0x01,0x00,sixff);
 writeSMANET2PlusPacket(level1packet,0x09, 0xa0, packet_send_counter, 0, 0, 0);
 writeSMANET2ArrayFromProgmem(level1packet,smanet2packetinvertername);
 writeSMANET2PlusPacketTrailer(level1packet);
 writePacketLength(level1packet);
 sendPacket(level1packet);
 
 waitForMultiPacket(0x0001);
 } 
 while (!validateChecksum());
 packet_send_counter++;
 
 valuetype = level1packet[40+1+1]+level1packet[40+2+1]*256;
 
 if (valuetype==0x821e) {
 memcpy(invertername,&level1packet[48+1],14);
 Serial.println(invertername);
 memcpy(&datetime,&level1packet[40+4+1],4);  //Returns date/time unit switched PV off for today (or current time if its still on)
 }
 }
 
 void HistoricData() {
 
 time_t currenttime=now();
 digitalClockDisplay(currenttime);
 
 debugMsgln("Historic data...."));
 tmElements_t tm;
 if( year(currenttime) > 99)
 tm.Year = year(currenttime)- 1970;
 else
 tm.Year = year(currenttime)+ 30;  
 
 tm.Month = month(currenttime);
 tm.Day = day(currenttime);
 tm.Hour = 10;      //Start each day at 5am (might need to change this if you're lucky enough to live somewhere hot and sunny!!
 tm.Minute = 0;
 tm.Second = 0;
 time_t startTime=makeTime(tm);  //Midnight
 
 
 //Read historic data for today (SMA inverter saves 5 minute averaged data)
 //We read 30 minutes at a time to save RAM on Arduino
 
 //for (int hourloop=1;hourloop<24*2;hourloop++)
 while (startTime < now()) 
 {
 //HowMuchMemory();
 
 time_t endTime=startTime+(25*60);  //25 minutes on
 
 //digitalClockDisplay(startTime);
 //digitalClockDisplay(endTime);
 //debugMsgln(" ");
 
 do {
 writePacketHeader(level1packet);
 //writePacketHeader(level1packet,0x01,0x00,smaBTInverterAddressArray);
 writeSMANET2PlusPacket(level1packet,0x09, 0xE0, packet_send_counter, 0, 0, 0);
 
 writeSMANET2SingleByte(level1packet,0x80);
 writeSMANET2SingleByte(level1packet,0x00);
 writeSMANET2SingleByte(level1packet,0x02);
 writeSMANET2SingleByte(level1packet,0x00);
 writeSMANET2SingleByte(level1packet,0x70);
 // convert from an unsigned long int to a 4-byte array
 writeSMANET2Long(level1packet,startTime);
 writeSMANET2Long(level1packet,endTime);
 writeSMANET2PlusPacketTrailer(level1packet);
 writePacketLength(level1packet);
 sendPacket(level1packet);
 
 waitForMultiPacket(0x0001);
 }
 while (!validateChecksum());
 //debugMsg("packetlength=");    Serial.println(packetlength);
 
 packet_send_counter++;
 
 //Loop through values
 for(int x=40+1;x<(packetposition-3);x+=12){
 memcpy(&value,&level1packet[x+4],4);
 
 if (value > currentvalue) {
 memcpy(&datetime,&level1packet[x],4);
 digitalClockDisplay(datetime);
 debugMsg("=");
 Serial.println(value);
 currentvalue=value;         
 
 //uploadValueToSolarStats(currentvalue,datetime);          
 }
 }
 
 startTime=endTime+(5*60);
 delay(750);  //Slow down the requests to the SMA inverter
 }
 }
 */








