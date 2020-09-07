////////////////////////////////////////////////////////////////////////////////
// menu.cpp
////////////////////////////////////////////////////////////////////////////////

#include "menu.h"
#include "display.h"

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////

Button::Button(uint32_t msLong)
  : _gpio{Gpio::gpioA8()}, _tType{TickTimer::msToTick(msLong)}      
{
}

void Button::setup()
{
  _gpio.setup(Gpio::Mode::IN_FL) ;
  _lastValue  = _gpio.get() ;
  _tLastChange = TickTimer::now() ;
}

bool Button::press(bool &type) // type==0: short ; type==1: long
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
  
////////////////////////////////////////////////////////////////////////////////
// Menu
////////////////////////////////////////////////////////////////////////////////

Menu::Entry::Entry(const std::string &text, std::function<bool(void)> action) :
  _text(text), _action(action)
{
}

const std::string& Menu::Entry::text() const { return _text ; }
bool Menu::Entry::operator()() const { return _action() ; }

Menu::Menu(Display &disp, std::initializer_list<Entry> entries, std::function<void(void)> cb) : _disp(disp), _entries(entries), _cb(cb), _button(500)
{
}
Menu::~Menu()
{
}

void Menu::setup()
{
  _button.setup() ;
}
  
bool Menu::select()
{
  bool btnLong ;
  if (_entries.empty() || !_button.press(btnLong))
    return false ;
    
  size_t idx{0} ;
  TickTimer t(7500) ;

  _disp.menuOn() ;
  _disp.menu("Long: select " + _entries[idx].text(), "Short: next "  + _entries[(idx+1) % _entries.size()].text()) ;
      
  while (!t())
  {
    _cb() ;
      
    if (_button.press(btnLong))
    {
      t.restart() ;

      if (btnLong)
      {
        _disp.menu("Short: update " + _entries[idx].text(), "Long: select " + _entries[(idx+1) % _entries.size()].text()) ;

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
              {
                return true ;
                _disp.menuOff() ;
              }
            }
          }
        }
      }
      else
      {
        idx = (idx + 1) % _entries.size() ; 
      }

      _disp.menu("Long: select " + _entries[idx].text(), "Short: next "  + _entries[(idx+1) % _entries.size()].text()) ;
    }    
  }

  _disp.menuOff() ;
  return true ;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
