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


char solarstatswebsite[] PROGMEM = "www.solarstats.co.uk";


int webserviceSolarStats::getTimerResetValue (){
  //Number of minutes between uploads
  return 2;
}//end function


bool webserviceSolarStats::dnsLookup() {
  //Think the pvoutputwebsite should be moved into a function inside the base class then overridden from the child, can we return prog_char* ??
  return ether.dnsLookup( solarstatswebsite );
}

void webserviceSolarStats::preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) 
{ 
  Serial.print(F("Uploading to "));
  Serial.println(solarstatswebsite);

  // generate the header with payload - note that the stash size is used,
  // and that a "stash descriptor" is passed in as argument using "$H" 

//http://forum.jeelabs.net/node/375

  //Format of yyyy-MM-dd
  char d[]={
    '0','0','0','0','-','0','0','-','0','0',0      };
  //Format of HHmm
  char t[]={
    '0','0','0','0',0      };

  formatTwoDigits(t,hour(dt));
  formatTwoDigits(t+2,minute(dt));

  formatTwoDigits(d+5,month(dt));
  formatTwoDigits(d+8,day(dt));

  itoa(year(dt), d, 10);
  d[4]='-';

  char watthour[12];
  ultoa(currentvalue,watthour,10);
  byte sd = stash.create(); 

  //Perhaps use instead...
  //ether.browseUrl(PSTR("/xyz.php"), "?search=Arduino", PSTR(HOSTNAME), &browserresult_callback);

  stash.print(F("GET http://www.solarstats.co.uk/api/Update.ashx?apikey="));
  //Would like to do... stash.print(solarstatswebsite);

  stash.print(F("XXXXXXXXXX"));

  stash.print(F("&apipassword=")); 
  stash.print(F("YYYYYYYYYY"));

  stash.print(F("&date="));
  stash.print(d);
  stash.print(F("&time="));
  stash.print(t);
  stash.print(F("&watthour="));
  stash.print(watthour);

  stash.print(F(" HTTP/1.0\r\nHost: www.solarstats.co.uk\r\n"));
  stash.print(F("User-Agent: NanodeSMAPVMonitor\r\nContent-Length: 0 \r\n"));
  stash.print(F("\r\n"));
  stash.save();

  Stash::prepare(PSTR("$H"),sd);
}




