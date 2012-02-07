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





class WebSiteService 
{  
private:  
  int minutecountdownvalue;
  uint8_t hostip[4];

public:
  //virtual prog_char* getWebAddress();
  virtual int getTimerResetValue();
  virtual void preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt);
  virtual bool dnsLookup();

  void formatTwoDigits(char* strOut, int num);
  void CountDown ();
  void lookupDNSHostIP() ;
  void CountDownAndUpload(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) ;
  void resetCountDownTimer();
  void setIPAddress();
  void begin();
};




//These class def's should/could go in their own header files....


class webservicePachube : 
public WebSiteService
{
public:
  bool dnsLookup();
  void preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) ;
  int getTimerResetValue();
};



class webserviceSolarStats : 
public WebSiteService
{

  public:
  bool dnsLookup();
  void preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) ;
  int getTimerResetValue();
};



class webservicePVoutputOrg : 
public WebSiteService
{
public:
  bool dnsLookup();
  void preparePacket(unsigned long totalkWhGenerated,unsigned long spotTotalPowerAC, time_t dt) ;
  int getTimerResetValue();
};



