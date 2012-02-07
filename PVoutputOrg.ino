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

char pvoutputwebsite[] PROGMEM = "pvoutput.org";


int webservicePVoutputOrg::getTimerResetValue (){
  //Number of minutes between uploads
  return 5;
}//end function

bool webservicePVoutputOrg::dnsLookup() {
  //Think the pvoutputwebsite should be moved into a function inside the base class then overridden from the child, can we return prog_char* ??
  return ether.dnsLookup( pvoutputwebsite );
}

void webservicePVoutputOrg::preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) 
{ 
  debugMsg("Uploading to ");
  Serial.println(pvoutputwebsite);

  //Format of yyyymmdd
  char d[]={
    '0','0','0','0','0','0','0','0',0              };
  //Format of HHmm
  char t[]={
    '0','0',':','0','0',0              };

  formatTwoDigits(t,hour(dt));
  formatTwoDigits(t+3,minute(dt));

  itoa(year(dt), d, 10);
  formatTwoDigits(d+4,month(dt));
  formatTwoDigits(d+6,day(dt));

  byte sd = stash.create(); 
  stash.print(F("d="));
  stash.print(d);
  stash.print(F("&t="));
  stash.print(t);

  //PV Generation
  stash.print(F("&v1="));
  stash.print(totalkWhGenerated);

  //Spot power
  stash.print(F("&v2="));
  stash.print(spotTotalPowerAC);
  stash.print(F("&c1=1"));

  stash.save();

  Stash::prepare(PSTR("POST http://$F/service/r2/addstatus.jsp HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    "X-Pvoutput-Apikey: $F" "\r\n"
    "X-Pvoutput-SystemId: $F" "\r\n"
    "Accept: */*\r\n"
    //"User-Agent: NanodeSMAPVMonitor" "\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: $D" "\r\n"
    "\r\n"
    "$H"),
  pvoutputwebsite,pvoutputwebsite, PSTR(PVOUTPUTAPIKEY),PSTR(PVOUTPUTSYSTEMID), stash.size(), sd);

}//end function



