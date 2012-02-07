/*
NANODE SMA PV MONITOR
 
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

#include <SoftwareSerial.h>
#include <EEPROM.h>


SoftwareSerial blueToothSerial(RxD,TxD);

void BTStart() {
  if (!readArrayFromEEPROM(myBTAddress,6,ADDRESS_MY_BTADDRESS) || !readArrayFromEEPROM(smaBTInverterAddressArray,6,ADDRESS_SMAINVERTER_BTADDRESS)) 
  {
    debugMsgln("Checksum failed - pairing");
    BTInitStartup(true);    
  } 
  else BTInitStartup(false);

  //Start normal connection to BT chip, it will auto-connect as master to the SMA inverter (slave)
  blueToothSerial.begin(9600);
  
}

void sendPacket(unsigned char *btbuffer) {
  quickblink();
  for(int i=0;i<packetposition;i++) {
    blueToothSerial.write(btbuffer[i]);    
  }
}

void quickblink() {
  digitalWrite( RED_LED, LOW);
  delay(100);
  digitalWrite( RED_LED, HIGH);
}


/*
void testreceiveloop() 
{ 

  //Loop whilst there is data available from serial
  while(blueToothSerial.available()) {
    unsigned char a=blueToothSerial.read();    
    if (a==0x7e) Serial.println("");    
    if (a<=0xf) Serial.print("0");
    Serial.print(a,HEX);
  }

  digitalWrite( RED_LED, HIGH);
  delay(1000);
  digitalWrite( RED_LED, LOW);
  delay(1000);
}
*/


void BTInitStartup(bool ForcePair) {
  BTSwitchOff();

  pinMode(INVERTERSCAN_PIN, INPUT);
  delay(250);

  //Check if pin/button is pressed during power on - if so begin SMA Inverter bluetooth pairing/scanning
  if (digitalRead(INVERTERSCAN_PIN)==1 || ForcePair) {
    BTScanForSMAInverterToPairWith();
  }

  BTSwitchOn();  
}

void BTSwitchOn() {
  analogWrite(BLUETOOTH_POWER_PIN, 255);  //Ensure BT chip is on
  delay(1500);  //Give chip time to warm up and enter command mode
}

void BTSwitchOff() {
  analogWrite(BLUETOOTH_POWER_PIN, 0);  //Ensure Bluetooth chip is powered off

  pinMode(BT_KEY, OUTPUT);
  digitalWrite(BT_KEY, LOW);    // Turn off command mode 
}


void BTScanForSMAInverterToPairWith()
{
  //Bluetooth configuration and scanning/pairing for HC05 chip
  //These sequence of instructions have taken a VERY long time to work out!

  //This code switches on the BT HC05 chip, forces it into command mode by holding 
  //pin BT_KEY high during power up (serial=38400,8,n,1)

  //More information on AT command set for HC05 from
  //http://elecfreaks.com/store/download/datasheet/Bluetooth/HC-0305%20serail%20module%20AT%20commamd%20set%20201104%20revised.pdf

  char retstring[50];
  char comparebuff[15]; 
  char smabtinverteraddress[14];
  unsigned char tempaddress[6];  //BT address

  debugMsgln("Config Bluetooth");

  digitalWrite(BT_KEY, HIGH);    // Turn BT chip into command mode (HC05)
  delay(200);
  BTSwitchOn();

  blueToothSerial.begin(38400);

  //Before tidy 8416 bytes, 8282, 8390
  //Send AT wake up
  BTSendStringAndWait(retstring,F("AT"));
  //Firmware Version = +VERSION=2.0-20100601
  BTSendStringAndWait(retstring,F("AT+VERSION?"));
  //Factory reset to defaults
  BTSendStringAndWait(retstring,F("AT+ORGL"));
  //Name of chip
  BTSendStringAndWait(retstring,F("AT+NAME=NANODE_SMA_SOLAR"));
  //Set password to zeros
  BTSendStringAndWait(retstring,F("AT+PSWD=0000"));
  //MASTER mode
  BTSendStringAndWait(retstring,F("AT+ROLE=1"));
  //Forget existing pairing
  BTSendStringAndWait(retstring,F("AT+RMAAD"));
  //Sets speed to 9600 bits for normal operation (effective after reset of chip)
  BTSendStringAndWait(retstring,F("AT+UART=9600,0,0"));
  //Allow connection to any slave
  BTSendStringAndWait(retstring,F("AT+CMODE=1"));
  //Initialize the SPP profile lib
  BTSendStringAndWait(retstring,F("AT+INIT"));

  //Make sure were are in INITIALIZED mode
  strcpy_P(comparebuff, (const prog_char *)F("INITIALIZED"));
  do {
    BTSendStringAndWait(retstring,F("AT+STATE?"));
  }  
  while (strncmp(retstring+7,comparebuff,11)!=0);  

  //The following automatically scans for the SMA inverter and then pairs with it.

  //Limit results to SMA inverter bluetooth class/filters
  BTSendStringAndWait(retstring,F("AT+IAC=9e8b33"));

  //1F04 = Class of device (SMA Inverter)
  BTSendStringAndWait(retstring,F("AT+CLASS=1F04"));

  //Just find first device which answers our call within 20 seconds
  BTSendStringAndWait(retstring,F("AT+INQM=1,1,20"));

  //Start the INQUIRE process
  BTSendStringAndWait(retstring,F("AT+INQ"));

  //Process the BT address returned an replace characters...
  char *comma=strchr(retstring,',');
  strncpy(smabtinverteraddress,retstring+5,(comma-retstring)-5);
  smabtinverteraddress[(comma-retstring)-5]='\0';  //Add null string terminator

  //Change : for , characters
  for(int i=0; i<strlen(smabtinverteraddress); i++) {
    if (smabtinverteraddress[i]==':') smabtinverteraddress[i]=','; 
  }

  //Serial.print(F("SMA BT="));Serial.println(smabtinverteraddress);

  //RNAME = get remote bluetooth name, usually looks like "+RNAME:SMA001d SN: 1234567890 SN123456"
  BTSendStringAndWait(retstring,smabtinverteraddress,F("AT+RNAME?"));

  //PAIR device
  blueToothSerial.print(F("AT+PAIR="));
  blueToothSerial.print(smabtinverteraddress);
  blueToothSerial.println(F(",20"));
  waitBlueToothReply(retstring);

  //FSAD = Seek the authenticated device in the Bluetooth pair list
  BTSendStringAndWait(retstring,smabtinverteraddress,F("AT+FSAD="));

  //Ask BT chip for its address
  BTSendStringAndWait(retstring,F("AT+ADDR?"));
  //Convert BT Address into a more useful byte array, the BT address is in a really awful format to parse!
  convertBTADDRStringToArray(retstring+6,tempaddress,':');

  writeArrayIntoEEPROM(tempaddress,6,ADDRESS_MY_BTADDRESS);  //EEPROM address 0 contains our bluetooth chip address

  //Ask BT chip what its set to pair with
  BTSendStringAndWait(retstring,F("AT+MRAD?"));
  //Convert BT Address into a more useful byte array, the BT address is in a really awful format to parse!
  convertBTADDRStringToArray(retstring+6,tempaddress,':');

  writeArrayIntoEEPROM(tempaddress,6,ADDRESS_SMAINVERTER_BTADDRESS);  //EEPROM address 10 contains SMA inverter address

  //BIND = Set/Inquire - bind Bluetooth address
  BTSendStringAndWait(retstring,smabtinverteraddress,F("AT+BIND="));

  //Connect the module to the specified Bluetooth address. (Bluetooth address can be specified by the binding command)
  BTSendStringAndWait(retstring,F("AT+CMODE=0"));

  //Make sure were PAIRED and then connect...
  strcpy_P(comparebuff, (const prog_char *)F("PAIRED"));
  do {
    BTSendStringAndWait(retstring,F("AT+STATE?"));
  }  
  while (strncmp(retstring+7,comparebuff,6)!=0);  

  BTSendStringAndWait(retstring,F("AT+RESET"));

  blueToothSerial.end();

  delay(100);
  digitalWrite(BT_KEY, LOW);    // Turn off command mode
  delay(500);  
  debugMsgln("Bluetooth configure. Finished.");
}



void writeArrayIntoEEPROM(unsigned char readbuffer[],int length,int EEPROMoffset) {
  //Writes an array into EEPROM and calculates a simple XOR checksum
  byte checksum=0;
  for(int i=0;i<length; i++) {
    EEPROM.write(EEPROMoffset+i,readbuffer[i]);
    //Serial.print(EEPROMoffset+i); Serial.print("="); Serial.println(readbuffer[i],HEX);   
    checksum^= readbuffer[i];
  }

  //Serial.print(EEPROMoffset+length); Serial.print("="); Serial.println(checksum,HEX);
  EEPROM.write(EEPROMoffset+length,checksum);
}

bool readArrayFromEEPROM(unsigned char readbuffer[],int length,int EEPROMoffset) {
  //Writes an array into EEPROM and calculates a simple XOR checksum
  byte checksum=0;
  for(int i=0;i<length; i++) {
    readbuffer[i]=EEPROM.read(EEPROMoffset+i);
    //Serial.print(EEPROMoffset+i); Serial.print("="); Serial.println(readbuffer[i],HEX);   
    checksum^= readbuffer[i];
  }
  //Serial.print(EEPROMoffset+length); Serial.print("="); Serial.println(checksum,HEX);
  return (checksum==EEPROM.read(EEPROMoffset+length)); 
}

void BTSendStringAndWait(char retstring[],  __FlashStringHelper* str) {
  Serial.println(str);
  blueToothSerial.println(str);
  waitBlueToothReply(retstring);  
}

void BTSendStringAndWait(char retstring[], char smabtinverteraddress[14], __FlashStringHelper* str) {
  Serial.print(str);
  Serial.println(smabtinverteraddress);

  blueToothSerial.print(str);
  blueToothSerial.println(smabtinverteraddress);
  waitBlueToothReply(retstring);  
}

void waitBlueToothReply(char outbuf[]) {
  //Wait max 10 lines of input before error
  unsigned int errorloop=10;
  char btb[50];

  outbuf[0]=0;

  delay(15);

  do {
    int bp=0;
    do {
      btb[bp]=getByte();
      Serial.print(btb[bp]);
      bp++;
      if  (btb[bp-1]=='\n') break;  //New line
    } 
    while ((btb[bp-1]!='\n') && (bp<50));

    btb[bp]=0;  //Force end of string

    if (btb[0]=='+') {     
      strncpy (outbuf,btb,bp-2);
      outbuf[bp-2]='\0';     
    }

    if ((strncmp(btb,"ERROR",5)==0) ||  (strncmp(btb,"FAIL",4)==0) ) {
      Serial.print(btb);
      error();
    }

    errorloop--;

    if (errorloop==0) error();
  } 
  while (strncmp(btb,"OK\r\n",4)!=0); 
}

unsigned char getByte() {
  //Return a single byte from the bluetooth stream (with error timeout/reset)
  unsigned long time;

  //This will eventually loop back to zero (about 70 days I think) but we 
  //would have to be very unlucky to hit that exact event!
  //Max wait 60 seconds, before throwing an fatal error
  time = 60000+millis(); 

  if (blueToothSerial.overflow()) {
    debugMsgln("Overflow"); 
  }

  while (blueToothSerial.available()==false) {
    delay(5);  //Wait for BT byte to arrive
    if (millis() > time) { 
      debugMsgln("Timeout");
      error();
    }
  }

  return (unsigned char)blueToothSerial.read();
}




void convertBTADDRStringToArray(char *tempbuf,unsigned char *outarray, char match) {
  //Convert BT Address into a more useful byte array, the BT address is in a really awful format to parse!
  //Must be a better way of doing this function!

  //Unit test cases...
  //Test BT address strcpy(tempbuf,"1234:56:0\x0");
  //Test BT address strcpy(tempbuf,"1010:7:310068\x0");
  //Test BT address strcpy(tempbuf,"80:25:1dac53\x0");
  //Test BT address strcpy(tempbuf,"2:72:D2224\x0");
  //Test BT address strcpy(tempbuf,"1234:56:0\x0");
  //Test BT address strcpy(tempbuf,"1:72:D2224\x0");
  //Test BT address strcpy(tempbuf,"1234:56:0\x0");
  //Test BT address strcpy(tempbuf,"1234,56,abcdef\x0");
  //Test BT address strcpy(tempbuf,"0002,72,0d2224\x0");
  int l=strlen(tempbuf);
  char *firstcolon=strchr(tempbuf,match)+1;
  char *lastcolon=strrchr(tempbuf,match)+1;

  //Could use a shared buffer here to save RAM
  char output[13]={
    '0','0','0','0','0','0','0','0','0','0','0','0',0      };

  //Deliberately avoided sprintf as it adds over 1600bytes to the program size at compile time.
  int partlen=(firstcolon-tempbuf)-1;
  strncpy(output+(4-partlen),tempbuf,partlen);

  partlen=(lastcolon-firstcolon)-1;
  strncpy(output+4+(2-partlen),firstcolon,partlen);

  partlen=l-(lastcolon-tempbuf);
  strncpy(output+6+(6-partlen),lastcolon,partlen);

  //Finally convert the string (11AABB44FF66) into a real byte array
  //written backwards in the same format that the SMA packets expect it in  
  int i2=5;
  for(int i=0; i<12; i+=2){
    outarray[i2--]=hex2bin(&output[i]);
  }

  /*
Serial.print("BT StringToArray=");  
   for(int i=0; i<6; i+=1)    debugPrintHexByte(outarray[i]);   
   Serial.println("");
   */
}

int hex2bin( const char *s )
{
  int ret=0;
  int i;
  for( i=0; i<2; i++ )
  {
    char c = *s++;
    int n=0;
    if( '0'<=c && c<='9' )
      n = c-'0';
    else if( 'a'<=c && c<='f' )
      n = 10 + c-'a';
    else if( 'A'<=c && c<='F' )
      n = 10 + c-'A';
    ret = n + ret*16;
  }
  return ret;
}

