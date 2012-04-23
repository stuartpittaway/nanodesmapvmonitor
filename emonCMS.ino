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

char emoncmswebsite[] PROGMEM = "vis.openenergymonitor.org";


int webserviceemonCMS::getTimerResetValue (){
  //Number of minutes between uploads
  return 1;
}//end function


bool webserviceemonCMS::dnsLookup() {
  //Think the pvoutputwebsite should be moved into a function inside the base class then overridden from the child, can we return prog_char* ??
  return ether.dnsLookup( emoncmswebsite );
}

void webserviceemonCMS::preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC,unsigned long spotTotalPowerDC, time_t dt) 
{ 
  byte sd = stash.create(); 
  stash.print(F("{\"solarkw\":\""));
  stash.print(spotTotalPowerAC);
  stash.print(F("\",\"solarkwh\":\""));
  stash.print((double)totalkWhGenerated/(double)1000);
  stash.print(F("\"}"));
  stash.save();



  Stash::prepare(PSTR("PUT http://$F/emoncms3/api/post.json?apikey=$F&json=$H HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    //"User-Agent: NanodeSMAPVMonitor" "\r\n"
    "Content-Length: $D" "\r\n"
    "\r\n"),
  emoncmswebsite,PSTR(EMONCMSAPIKEY),sd,emoncmswebsite, 0);

}





