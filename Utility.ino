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



extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

static int freeMemory() {
  int free_memory;

  if((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}


static void HowMuchMemory() {
  Serial.print(F("m="));
  Serial.println(freeMemory());
}



static void error() {
  Serial.println(F("\r\n*Err*"));
  int loop=150;
  while (loop>0) {
    digitalWrite( RED_LED, HIGH);
    delay(50);
    digitalWrite( RED_LED, LOW);
    delay(150);
    loop--;
  } 

  softReset(); 
}

static void softReset() {
  //Should really do this with a watchdog
  asm volatile ("  jmp 0");
} 

static void digitalClockDisplay(time_t t){
  // digital clock display  
  printDigits(hour(t));
  Serial.print(':');
  printDigits(minute(t));
  Serial.print(':');
  printDigits(second(t));
  Serial.print(' ');
  Serial.print(year(t)); 
  printDigits(month(t));
  printDigits(day(t));
}

static void printDigits(int digits){
  if(digits < 10) Serial.print('0');
  Serial.print(digits);
}

static void printHexDigits(int digits){
  if(digits <= 0xF) Serial.print('0');
  Serial.print(digits,HEX);
}

static void printMacAddress(byte mymac[]) {
  Serial.print(F("MAC="));  
  for(int i=0; i<6; i++) {
    printHexDigits(mymac[i]);
  }
  Serial.println("");  
}



