////////////////////////////////////////////////////////////////////////////////
// display.cpp
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include "gd32vf103.h"
}

#include "display.h"
#include "si4703.h"

using ::RV::Longan::Lcd ;
using ::RV::Longan::LcdArea ;

////////////////////////////////////////////////////////////////////////////////

Display::Display(Si4703 &si4703) :
  _lcd(Lcd::lcd()),
  _laData   (_lcd,   0, 160,  0, 64),
  _laChan   (_lcd,   0,  80,  0, 16),
  _laRssi   (_lcd,  80,  60,  0, 16),
  _laStereo (_lcd, 140,  20,  0, 16),
  _laMenu   (_lcd,   0, 160, 32, 32),
  _laRdsStation(_lcd,   0,  80, 16, 16),
  _laRdsTime   (_lcd,  80, 160, 16, 16),
  _laRdsText   (_lcd,   0, 160, 32, 48),
  _si4703{si4703}, _chan{0xffff}, _rssi{0xffff}, _stereo{0}
{
}

void Display::setup()
{
  _lcd.setup() ;

  LcdArea laTitle (_lcd, 0, 110,  0, 16, &::RV::Longan::Roboto_Bold7pt7b , 0xffffffUL, 0xa00000UL) ;
  laTitle.put("  R A D I O  ") ;
}

void Display::menuOn()
{
  _laMenu.clear() ;
}

void Display::menu(const std::string &txt1, const std::string &txt2)
{
  _laMenu.txtPos(0) ; _laMenu.put(txt1.c_str()) ; _laMenu.clearEOL() ;
  _laMenu.txtPos(1) ; _laMenu.put(txt2.c_str()) ; _laMenu.clearEOL() ;
}
void Display::menuOff()
{
  _laMenu.clear() ;
  update(true, false) ;
}

void Display::error(const std::string &txt)
{
  _laMenu.clear() ;
  _laMenu.txtPos(0) ; _laMenu.put(txt.c_str()) ;
}

void Display::update(bool force, bool topOnly)
{
  _lcd.heartbeat() ;
    
  _si4703.rds() ;
    
  // display channel
  {
    if (_chan != _si4703.channel())
    {
      _chan = _si4703.channel() ;
      _laChan.txtPos(0) ;
      _laChan.put((uint16_t)(_chan / 10)) ;
      _laChan.put('.') ;
      _laChan.put((uint16_t)(_chan % 10)) ;
      _laChan.put("MHz") ;
      _laChan.clearEOL() ;
    }
  }
  
  // display rssi
  {
    if (_rssi != _si4703.rssi())
    {
      _rssi = _si4703.rssi() ;
      _laRssi.txtPos(0) ;
      _laRssi.put(_rssi) ;
      _laRssi.clearEOL() ;
    }
  }
  
  // display stereo
  {
    if (_stereo != (_si4703.stereoInd() ? 'S' : 'M'))
    {
      _stereo = _si4703.stereoInd() ? 'S' : 'M' ;
      _laStereo.txtPos(0) ;
      _laStereo.put(_stereo) ;
      _laStereo.clearEOL() ;
    }
  }

  // rds station
  {
    std::string rdsStation = _si4703.rdsStationText() ;
    if (_rdsStation != rdsStation)
    {
      _rdsStation = rdsStation ;
      _laRdsStation.clear() ;
      _laRdsStation.put(_rdsStation.c_str()) ;
    }
  }

  // rds time
  {
    std::string rdsTime = _si4703.rdsTimeValid() ? _si4703.rdsTimeText() : "" ;
    if (_rdsTime != rdsTime)
    {
      _rdsTime = rdsTime ;
      _laRdsTime.txtPos(0) ;
      _laRdsTime.put(_rdsTime.c_str()) ;
      _laRdsTime.clearEOL() ;
    }
  }

  if (topOnly)
    return ;

  // rds text
  {
    std::string rdsText = _si4703.rdsTextText() ;
    if (_rdsText != rdsText)
    {
      _rdsText = rdsText ;
      _laRdsText.clear() ;
      _laRdsText.put(_rdsText.c_str()) ;
    }

  }
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
