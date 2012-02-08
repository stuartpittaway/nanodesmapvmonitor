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



void WebSiteService::CountDown () {
  if (this->minutecountdownvalue > 0) {
    this->minutecountdownvalue-=1;
  }
}//end function

void WebSiteService::begin() {
  //Service always fires on first call...
  minutecountdownvalue=0;
  lookupDNSHostIP();
}

void WebSiteService::lookupDNSHostIP() {
  debugMsgln("DNS lookup"); 
  if (this->dnsLookup())
  {
    ether.copyIp(this->hostip,ether.hisip);
    //ether.printIp("SRV:", hostip);    
  } 
  else { 
    debugMsgln("DNS fail"); 
    error();
  }
}//end function

void WebSiteService::resetCountDownTimer() {
  this->minutecountdownvalue=getTimerResetValue();  
}

void WebSiteService::setIPAddress() {
  ether.copyIp(ether.hisip, this->hostip);
}


void WebSiteService::stashPrintTwoDigits(byte num)
{
  stash.write('0' + (num / 10));
  stash.write('0' + (num % 10));
}

/*
void WebSiteService::formatTwoDigits(char* strOut, int num)
 {
 strOut[0] = '0' + (num / 10);
 strOut[1] = '0' + (num % 10);
 }
 */

void WebSiteService::CountDownAndUpload(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) 
{ 
  CountDown();

  if (this->minutecountdownvalue<=0) {
    this->resetCountDownTimer();

    this->setIPAddress();

    this->preparePacket(totalkWhGenerated,spotTotalPowerAC, dt) ;

    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();

    //Wait for a bit to process any possible replies, ideally we
    //would like to wait here for a successful HTTP 200 message
    //or timeout/error condition
    for(int i=0; i<250; i++)
    {
      delay(5);
      ether.packetLoop(ether.packetReceive());
    }
  }
}//end function


