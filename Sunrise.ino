static unsigned int  ComputeSun(float longitude,float latitude,time_t when, bool rs) {
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
  return 60*(720+4*(lon-ha)*57.295779513082322-eqt);

  // convert from UTC back to our timezone
  //minutes+= timezone;
 
  //return seconds;
}

