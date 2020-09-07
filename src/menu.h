////////////////////////////////////////////////////////////////////////////////
// menu.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

extern "C"
{
#include "gd32vf103.h"
}

#include <functional>

#include "GD32VF103/time.h"
#include "GD32VF103/gpio.h"

using ::RV::GD32VF103::TickTimer ;
using ::RV::GD32VF103::Gpio ;

class Display ;

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////

class Button
{
 public:
  
  Button(uint32_t msLong) ;
  
  void setup() ;
  bool press(bool &type) ; // type==0: short ; type==1: long
  
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
    Entry(const std::string &text, std::function<bool(void)> action) ;
    
    const std::string& text() const ;
    bool operator()() const ;
    
  private:
    std::string _text ;
    std::function<bool(void)> _action ;
  } ;

  Menu(Display &disp, std::initializer_list<Entry> entries, std::function<void(void)> cb) ;
  ~Menu() ;
  
  void setup() ;
  bool select() ;

private:
  Display &_disp ;
  std::vector<Entry> _entries ;
  std::function<void(void)> _cb ;
  Button _button ;
} ;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
