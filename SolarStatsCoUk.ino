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
  //Think the pvoutputwebsite should be moved into a function inside the base class then overridden from the child, can we return prog_
  return ether.dnsLookup( solarstatswebsite );
}

void webserviceSolarStats::preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) 
{ 
  Serial.print(F("Uploading to "));
  Serial.println(solarstatswebsite);

  // generate the header with payload - note that the stash size is used,
  // and that a "stash descriptor" is passed in as argument using "$H" 

  //http://forum.jeelabs.net/node/375

  byte sd = stash.create(); 
  stash.print(F("date="));
  //date is in YYYY-MM-DD format
  stash.print(year(dt));
  stash.print('-');
  stashPrintTwoDigits(month(dt));
  stash.print('-');
  stashPrintTwoDigits(day(dt));
  stash.print(F("&time=")); 
  stashPrintTwoDigits(hour(dt));
  stashPrintTwoDigits(minute(dt));
  stash.print(F("&watthour="));
  stash.print(totalkWhGenerated);
  stash.save();


  Stash::prepare(PSTR("GET http://$F/api/Update.ashx?apikey=$F&apipassword=$F&$H HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    "User-Agent: NanodeSMAPVMonitor" "\r\n"
    "Content-Length: $D" "\r\n"
    "\r\n"),
  solarstatswebsite,PSTR(SOLARSTATSAPIKEY),PSTR(SOLARSTATSAPIPASSWORD),sd,solarstatswebsite, 0);  
}





