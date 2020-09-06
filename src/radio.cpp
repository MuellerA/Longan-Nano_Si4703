///////////////////////////////////////////////////////////////////////////////
// radio.cpp
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include "gd32vf103.h"
}

#include <functional>

#include "Longan/lcd.h"
#include "Longan/fonts.h"
#include "GD32VF103/time.h"
#include "GD32VF103/gpio.h"

#include "si4703.h"

using ::RV::Longan::Lcd ;
using ::RV::Longan::LcdArea ;
using ::RV::GD32VF103::TickTimer ;
using ::RV::GD32VF103::Gpio ;

////////////////////////////////////////////////////////////////////////////////
// Display
////////////////////////////////////////////////////////////////////////////////

class Display
{
public:
  Display(const Si4703 &si4703) :
    _lcd(Lcd::lcd()),
    _laData  (_lcd,   0, 160, 16, 64),
    _laChan  (_lcd,   0,  80, 16, 16),
    _laRssi  (_lcd,  80,  60, 16, 16),
    _laStereo(_lcd, 140,  20, 16, 16),
    _laMenu  (_lcd,   0, 160, 32, 48),
    _si4703{si4703}, _chan{0xffff}, _rssi{0xffff}, _stereo{0}
  {
  }

  void setup()
  {
    _lcd.setup() ;

    LcdArea laTitle (_lcd, 0, 110,  0, 16, &::RV::Longan::Roboto_Bold7pt7b , 0xffffffUL, 0xa00000UL) ;
    laTitle.put("  R A D I O  ") ;
  }

  LcdArea& laData() { return _laData ; }
  LcdArea& laMenu() { return _laMenu ; }
  
  void update()
  {
    _lcd.heartbeat() ;
    
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

  }
  
private:
  Lcd    &_lcd ;
  LcdArea _laData ;  
  LcdArea _laChan ;
  LcdArea _laRssi ;
  LcdArea _laStereo ;
  LcdArea _laMenu ;

  const Si4703 &_si4703 ;
  uint16_t _chan ;
  uint16_t _rssi ;
  char     _stereo ;
} ;

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////

class Button
{
 public:
  
  Button(uint32_t msLong)
    : _gpio{Gpio::gpioA8()}, _tType{TickTimer::msToTick(msLong)}      
  {
  }

  void setup()
  {
    _gpio.setup(Gpio::Mode::IN_FL) ;
    _lastValue  = _gpio.get() ;
    _tLastChange = TickTimer::now() ;
  }

  bool press(bool &type) // type==0: short ; type==1: long
  {
    uint64_t now = TickTimer::now() ;
    uint64_t delta = now - _tLastChange ;
    
    if (delta < _tIgnoreChange) // ignore change faster than 20ms
      return false ;
    
    if (_gpio.get() == _lastValue)
      return false ;

    _lastValue = !_lastValue ;
    _tLastChange = now ;
    
    if (_lastValue)
      return false ;

    type = delta > _tType ;
    return true ;
  }
  
 private:
  Gpio    &_gpio ;
  bool     _lastValue ;
  uint64_t _tLastChange ;
  const uint64_t _tIgnoreChange{TickTimer::msToTick(20)} ;
  const uint64_t _tType ;
} ;

////////////////////////////////////////////////////////////////////////////////
// Menu
////////////////////////////////////////////////////////////////////////////////

class Menu
{
public:
  class Entry
  {
  public:
    Entry(const std::string &text, std::function<bool(void)> action) :
      _text(text), _action(action)
    {
    }

    const std::string& text() const { return _text ; }
    bool operator()() const { return _action() ; }
    
  private:
    std::string _text ;
    std::function<bool(void)> _action ;
  } ;

  Menu(LcdArea &la, std::initializer_list<Entry> entries, std::function<void(void)> cb) : _la(la), _entries(entries), _cb(cb), _button(500)
  {
  }
  ~Menu()
  {
  }

  void setup()
  {
    _button.setup() ;
  }
  
  bool select()
  {
    bool btnLong ;
    if (_entries.empty() || !_button.press(btnLong))
      return false ;
    
    size_t idx{0} ;
    TickTimer t(7500) ;
    
    _la.clear() ;
    _la.txtPos(0) ; _la.put("Long: select ") ; _la.put(_entries[idx].text().c_str()) ;
    _la.txtPos(1) ; _la.put("Short: next ") ; _la.put(_entries[(idx+1) % _entries.size()].text().c_str());
      
    while (!t())
    {
      _cb() ;
      
      if (_button.press(btnLong))
      {
        t.restart() ;

        if (btnLong)
        {
          _la.clear() ;
          _la.txtPos(0) ; _la.put("Short: update ") ; _la.put(_entries[idx].text().c_str()) ;
          _la.txtPos(1) ; _la.put("Long: select ") ; _la.put(_entries[(idx+1) % _entries.size()].text().c_str()) ;

          while (!t())
          {
            _cb() ;
            
            if (_button.press(btnLong))
            {
              t.restart() ;

              if (btnLong)
              {
                idx = (idx + 1) % _entries.size() ;
                break ;
              }
              else
              {
                if (_entries[idx]())
                  return true ;
              }
            }
          }
        }
        else
        {
          idx = (idx + 1) % _entries.size() ; 
        }

        _la.clear() ;
        _la.txtPos(0) ; _la.put("Long: select ") ; _la.put(_entries[idx].text().c_str()) ;
        _la.txtPos(1) ; _la.put("Short: next ") ; _la.put(_entries[(idx+1) % _entries.size()].text().c_str()) ;
      }    
    }

    return true ;
  }

private:
  LcdArea &_la ;
  std::vector<Entry> _entries ;
  std::function<void(void)> _cb ;
  Button _button ;
} ;

////////////////////////////////////////////////////////////////////////////////
// main()
////////////////////////////////////////////////////////////////////////////////

int main()
{
  Si4703 si4703 ;
  Display disp(si4703) ;

  disp.setup() ;

  
  if (!si4703.setup()  ||
      !si4703.read()   ||
      !si4703.enable() ||
      !si4703.seek(true))
  {
    disp.laData().put("Si4703 init failed") ;
    while (true) ;
  }

  Menu menu
    {
      disp.laMenu(),
     {
      { "Volume (-)", [&si4703]()
                      {
                        si4703.volume(false) ;
                        return false ;
                      }
      },
      { "Volume (+)", [&si4703]()
                      {
                        si4703.volume(true) ;
                        return false ;
                      }
      },
      {
        "Seek (+)", [&si4703]()
                   {
                     si4703.seek(true) ;
                     return false ;
                   } // todo
      },
      {
        "Seek (-)", [&si4703]()
                   {
                     si4703.seek(false) ;
                     return false ;
                   } // todo
      }
     },
     [&disp](){ disp.update() ; }
    } ;

  menu.setup() ;
  
  
  while (true)
  {
    if (!menu.select())
      disp.update() ;
  }
  
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
