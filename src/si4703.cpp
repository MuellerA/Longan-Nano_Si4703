////////////////////////////////////////////////////////////////////////////////
// si4703.cpp
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include "gd32vf103.h"
}

#include "GD32VF103/i2c.h"
#include "GD32VF103/gpio.h"
#include "GD32VF103/time.h"


#include "si4703.h"

using ::RV::GD32VF103::I2c ;
using ::RV::GD32VF103::Gpio ;
using ::RV::GD32VF103::TickTimer ;

I2c  &i2c(I2c::i2c0()) ;
Gpio &gpioSen(Gpio::gpioB15()) ;
Gpio &gpioRst(Gpio::gpioB14()) ;
Gpio &gpioGpio1(Gpio::gpioB13()) ;
Gpio &gpioGpio2(Gpio::gpioB12()) ;

static const uint8_t Addr{0x10} ;

////////////////////////////////////////////////////////////////////////////////

Si4703::Si4703()
{
}

bool Si4703::setup()
{
  i2c.setup(42, 400) ;
  gpioSen.setup(Gpio::Mode::OUT_PP) ;
  gpioRst.setup(Gpio::Mode::OUT_PP) ;
  gpioGpio1.setup(Gpio::Mode::IN_FL) ;
  gpioGpio2.setup(Gpio::Mode::IN_FL) ;

  gpioRst.low() ;
  gpioSen.high() ;
  TickTimer::delayMs(100) ;
  gpioRst.high() ;
  TickTimer::delayMs(100) ;

  if (!read())
    return false ;

  _test1 |= 0x8000 ; // Crystal Oscillator Enable
  if (!write(6))
    return false ;
  TickTimer::delayMs(500) ;

  if (!read())            // update _data
    return false ;

  _sysConfig1 &= ~0x900c ; // RDS Interrupt enable, RDS Enable, GPIO2 Irq
  _sysConfig1 |=  0x9004 ;
    
  _sysConfig2 &= ~0x00ff ; // 87.5–108 MHz (US / Europe, Default), 100 kHz (Europe / Japan), Volume 7
  _sysConfig2 |=  0x0017 ;

  _sysConfig3 &= ~0x00ff ; // Seek SNR Threshold, Seek FM Impulse Detection Threshold
  _sysConfig3 |=  0x0036 ;
    
  if (!write(5))
    return false ;
    
  return true ;
}

bool Si4703::read(uint8_t cnt) const
{
  uint16_t data[0x10] ;

  if (cnt > 0x10)
    cnt = 0x10 ;
  
  if (!i2c.get(Addr, (uint8_t*) data, cnt*sizeof(uint16_t)))
    return false ;

  for (uint8_t i = 0 ; i < cnt ; ++i)
  {
    uint16_t d = data[i] ;
    _data[(0x0a + i) & 0x0f] = ((d & 0xff00) >> 8) | ((d & 0x00ff) << 8) ;
  }
  
  return true ;
}

bool Si4703::write(uint8_t cnt)
{
  uint16_t data[0x10] ;

  if (cnt > 0x10)
    cnt = 0x10 ;
  
  for (uint8_t i = 0 ; i < cnt ; ++i)
  {
    uint16_t d = _data[(0x02 + i) & 0x0f] ;
    data[i] = ((d & 0xff00) >> 8) | ((d & 0x00ff) << 8) ;
  }
  
  return i2c.put(Addr, (uint8_t*) data, cnt*sizeof(uint16_t)) ;
}

bool Si4703::enable()
{
  _powerCfg |= 0x4001 ; // Mute Disable, Powerup Enable
  if (!write(1))
    return false ;
  TickTimer::delayMs(110) ;
  if (!read())
    return false ;

  return true ;
}

bool Si4703::disable()
{
  _test1 |= 0x4000 ; // Audio High-Z Enable
  _powerCfg &= ~0x4040 ; // Mute Disable, Powerup Disable
  if (!write(6))
    return false ;
  TickTimer::delayMs(2) ;
  if (!read())
    return false ;

  return true ;
}

bool Si4703::seek(bool up)
{
  _powerCfg |= up ? 0x0300 : 0x0100 ; // Seek Direction, Seek
  if (!write(1))
    return false ;

  while (true)
  {
    TickTimer::delayMs(500) ;
      
    if (!read(2))
      return false ;

    if (_statusRssi & 0x4000)
      break ;
  }

  _powerCfg &= ~0x0300 ; // Seek Direction, Seek
  if (!write(1))
    return false ;
  TickTimer::delayMs(2) ;
    
  return true ;
}

bool Si4703::volume(bool up)
{
  uint16_t v = _sysConfig2 & 0x000f ;
  if (up)
  {
    if (v != 0x000f) v += 1 ;
  }
  else
  {
    if (v != 0x0000) v -= 1 ;
  }
  _sysConfig2 &= ~0x000f ;
  _sysConfig2 |= v ;

  if (!write(4))
    return false ;

  return true ;
}

uint8_t Si4703::volume() const
{
  return _sysConfig2 & 0x000f ;
}

bool Si4703::stereo(bool on)
{
  if (on)
    _powerCfg &= ~0x2000 ;
  else
    _powerCfg |= 0x2000 ;

  if (!write(1))
    return false ;

  return true ;
}

bool Si4703::stereo() const
{
  return !(_powerCfg & 0x2000) ;
}

bool Si4703::stereoInd() const
{
  return _statusRssi & 0x0010 ;
}
  
uint16_t Si4703::channel() const // in 100kHz
{
  // Channel = 100kHz * channel + 87.5MHz
  //         = (channel + 875) 100kHz
  return (_readChan & 0x03ff) + 875 ;
}

bool Si4703::channel(uint16_t channel)
{
  channel -= 875 ;
  
  _channel &= ~0x83ff ;
  _channel |= 0x8000 | (channel & 0x03ff) ;

  if (!write(2))
    return false ;

  while (true)
  {
    TickTimer::delayMs(5) ;
    
    if (!read(2))
      return false ;

    if (_statusRssi & 0x4000)
      break ;
  }

  _channel &= ~0x8000 ;
  if (!write(2))
    return false ;

  return true ;
}

uint8_t Si4703::rssi() const
{
  return (uint8_t) _data[0x0a] ;
}

const uint16_t* Si4703::rds() const
{
  read(6) ;
  return &_rdsa ;
}
  
//uint16_t Si4703::data(uint8_t idx)
//{
//  return _data[idx] ;
//}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

