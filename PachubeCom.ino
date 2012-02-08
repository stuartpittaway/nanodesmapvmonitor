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

char pachubewebsite[] PROGMEM = "api.pachube.com";

int webservicePachube::getTimerResetValue (){
  //Number of minutes between uploads
  return 1;
}//end function


bool webservicePachube::dnsLookup() {
  return ether.dnsLookup( pachubewebsite );
}

void webservicePachube::preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) 
{ 
  byte sd = stash.create(); 
  stash.print(F("{\"version\":\"1.0.0\",\"datastreams\":["));
  //PV Generation
  stash.print(F("{\"id\":\"1\",\"current_value\":\""));
  stash.print((double)totalkWhGenerated/(double)1000);
  stash.print(F("\"}"));
  //Spot power
  stash.print(F(",{\"id\":\"4\",\"current_value\":\""));
  stash.print(spotTotalPowerAC);
  stash.print(F("\"}"));
  //Uptime in whole seconds
  stash.print(F(",{\"id\":\"2\",\"current_value\":\""));
  stash.print(millis()/1000);
  stash.print(F("\"}"));

  //Nanode RAM Memory Free
  stash.print(F(",{\"id\":\"3\",\"current_value\":\""));
  stash.print(freeMemory());
  stash.print(F("\"}"));
  stash.print(F("]}"));
  stash.save();

  Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.json HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    "X-PachubeApiKey: $F" "\r\n"
    "User-Agent: NanodeSMAPVMonitor" "\r\n"
    "Content-Length: $D" "\r\n"
    "\r\n"
    "$H"),
  pachubewebsite,PSTR(PACHUBEFEED), pachubewebsite, PSTR(PACHUBEAPIKEY), stash.size(), sd);

}





