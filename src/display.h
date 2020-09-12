////////////////////////////////////////////////////////////////////////////////
// display.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

extern "C"
{
#include "gd32vf103.h"
}

#include "Longan/lcd.h"
#include "Longan/fonts.h"

class Si4703 ;

////////////////////////////////////////////////////////////////////////////////

class Display
{
public:
  Display(Si4703 &si4703) ;

  void setup() ;

  void clear() ;
  void menuOn() ;
  void menu(const std::string &txt1, const std::string &txt2) ;
  void menuOff() ;
  void update(bool force, bool topOnly) ;
  void error(const std::string &txt) ;
  
private:
  ::RV::Longan::Lcd    &_lcd ;
  ::RV::Longan::LcdArea _laData ;  
  ::RV::Longan::LcdArea _laChan ;
  ::RV::Longan::LcdArea _laRssi ;
  ::RV::Longan::LcdArea _laStereo ;
  ::RV::Longan::LcdArea _laMenu ;
  ::RV::Longan::LcdArea _laRdsStation ;
  ::RV::Longan::LcdArea _laRdsTime ;
  ::RV::Longan::LcdArea _laRdsText ;

  Si4703 &_si4703 ;
  uint16_t _chan ;
  uint16_t _rssi ;
  char     _stereo ;
  std::string _rdsStation ;
  std::string _rdsTime ;
  std::string _rdsText ;
} ;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
