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
  _laMenu   (_lcd,   0, 160, 32, 48),
  _laRdsStation(_lcd,   0,  80, 16, 16),
  _laRdsTime   (_lcd,  80, 160, 16, 16),
  _laRdsText   (_lcd,   0, 160, 32, 48),
  _si4703{si4703}, _chan{0xffff}, _rssi{0xffff}, _stereo{0}
{
}

void Display::setup()
{
  _lcd.setup() ;

  LcdArea laTitle (_lcd, 0, 160,  0, 80, &::RV::Longan::Roboto_Bold7pt7b , 0xffffffUL, 0xa00000UL) ;
  laTitle.clear() ;
  laTitle.put("R A D I O", 80, 40, 0, 0) ;
}

void Display::clear()
{
  _lcd.clear() ;
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

std::string rdsToLcd(const std::string &rds)
{
  std::string lcd ;
  uint8_t table{0} ;

  for (uint8_t idx = 0 ; idx < (uint8_t)rds.size() ; ++idx)
  {
    char c = rds[idx] ;

    if ((0x20 <= c) && (c <= 0x7d))
    {
      lcd += c ;
    }
    else if ((c == 0x0f) && (rds[idx+1] == 0x0f))
    {
      table = 0 ;
      idx += 1 ;
    }
    else if ((c == 0x0e) && (rds[idx+1] == 0x0e))
    {
      table = 1 ;
      idx += 1 ;
    }
    else if ((c == 0x1b) && (rds[idx+1] == 0x6e))
    {
      table = 1 ;
      idx += 1 ;
    }
    else
    {
      if ((table == 0) || (table == 1))
      {
        switch ((uint8_t)c)
        {
        case 0x8d: lcd += "ss" ; continue ;
        case 0x91: lcd += "ae" ; continue ;
        case 0x97: lcd += "oe" ; continue ;
        case 0x99: lcd += "ue" ; continue ;
        }
      }
      if (table == 0)
      {
        switch ((uint8_t)c)
        {
        case 0xd1: lcd += "Ae" ; continue ;
        case 0xd7: lcd += "Oe" ; continue ;
        case 0xd9: lcd += "Ue" ; continue ;
        }
      }
      lcd += "?" ;
    }
  }

  return lcd ;
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
      std::string txt = rdsToLcd(_rdsStation) ;
      _laRdsStation.put(txt.c_str()) ;
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
      std::string txt = rdsToLcd(_rdsText) ;
      _laRdsText.put(txt.c_str()) ;
    }

  }
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
