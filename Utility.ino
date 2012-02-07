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
  Serial.print(F("free mem="));
  Serial.println(freeMemory());
}



static void error() {
  Serial.println(F("\r\n*Error*"));
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


static time_t ComputeSun(float longitude,float latitude,time_t when, bool rs) {
//Borrowed from TimeLord library http://swfltek.com/arduino/timelord.html
//rs=true for sunrise, false=sunset

  //digitalClockDisplay(when);  Serial.println("");

  uint8_t a;

  float lon=-longitude/57.295779513082322;
  float lat=latitude/57.295779513082322;

  //approximate hour;
  a=6;  //6am
  if(rs) a=18;  //6pm

  // approximate day of year
  float y= (month(when)-1) * 30.4375 + (day(when)-1)  + a/24.0; // 0... 365

  // compute fractional year
  y *= 1.718771839885e-02; // 0... 1

  // compute equation of time... .43068174
  float eqt=229.18 * (0.000075+0.001868*cos(y)  -0.032077*sin(y) -0.014615*cos(y*2) -0.040849*sin(y* 2) );

  // compute solar declination... -0.398272
  float decl=0.006918-0.399912*cos(y)+0.070257*sin(y)-0.006758*cos(y*2)+0.000907*sin(y*2)-0.002697*cos(y*3)+0.00148*sin(y*3);

  //compute hour angle
  float ha=(  cos(1.585340737228125) / (cos(lat)*cos(decl)) -tan(lat) * tan(decl)  );

  if(fabs(ha)>1.0){// we're in the (ant)arctic and there is no rise(or set) today!
    return when; 
  }

  ha=acos(ha); 
  if(rs==false) ha=-ha;

  // compute minutes from midnight
  unsigned int seconds=60*(720+4*(lon-ha)*57.295779513082322-eqt);

  // convert from UTC back to our timezone
  //minutes+= timezone;
  
  tmElements_t tm;

  tm.Year = year(when)-1970;
  tm.Month = month(when);
  tm.Day = day(when);
  tm.Hour = 0;
  tm.Minute = 0;
  tm.Second = 0;

  time_t midnight=makeTime(tm);

  //Move to tomorrow and add on the minutes till sunrise (as seconds)
  midnight+=86400 +seconds;
 
  return midnight;
}

