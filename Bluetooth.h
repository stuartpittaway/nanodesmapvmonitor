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


#define RED_LED 6
#define INVERTERSCAN_PIN 14    //Analogue pin 1 - next to VIN on connectors
#define BT_KEY 15              //Forces BT BOARD/CHIP into AT command mode
#define RxD 16
#define TxD 17
#define BLUETOOTH_POWER_PIN 5  //pin 5

//Location in EEPROM where the 2 arrays are written
#define ADDRESS_MY_BTADDRESS  0
#define ADDRESS_SMAINVERTER_BTADDRESS  10


unsigned char smaBTInverterAddressArray[6]={  0x00,0x00,0x00,0x00,0x00,0x00};  //Hold byte array with BT address
unsigned char myBTAddress[6]={  0x00,0x00,0x00,0x00,0x00,0x00};  //Hold byte array with BT address

