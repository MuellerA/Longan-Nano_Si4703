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
#include "Longan/toStr.h"

#include "si4703.h"

using ::RV::GD32VF103::I2c ;
using ::RV::GD32VF103::Gpio ;
using ::RV::GD32VF103::GpioIrq ;
using ::RV::GD32VF103::TickTimer ;

I2c  &i2c(I2c::i2c0()) ;
Gpio &gpioSen(Gpio::gpioB15()) ;
Gpio &gpioRst(Gpio::gpioB14()) ;
//Gpio &gpioGpio1(Gpio::gpioB13()) ;
GpioIrq &gpioGpio2(GpioIrq::gpioA4()) ;

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
  //gpioGpio1.setup(Gpio::Mode::IN_FL) ;
  gpioGpio2.setup(GpioIrq::Mode::IN_FL, [this](bool rising){ if (!rising) _irqRdsInd = true ; }) ;

  // bus mode selection method 1, 2-wire
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
    
  _sysConfig2 &= ~0x00ff ; // 87.5–108 MHz (US / Europe, Default), 100 kHz (Europe / Japan), Volume 5
  _sysConfig2 |=  0x1415 ; // Seek Threshold 20

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

  _rdsStationValid = 0x00   ; _rdsStationStr.clear() ;
  _rdsTextValid    = 0x0000 ; _rdsTextStr.clear() ;
  _irqRdsInd = false ;

  TickTimer t(7500) ;
  while (true)
  {
    if (t())
      return false ;
    
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

void Si4703::rdsStation(uint8_t offset, char ch1, char ch2)
{
  uint8_t offsetData = offset << 1 ;
  uint8_t validMask = 1 << offset ;
  
  if (_rdsStationValid & validMask)
  {
    if ((_rdsStationText[offsetData  ] == ch1) && (_rdsStationText[offsetData+1] == ch2))
      return ; // no chagne
    _rdsStationValid = validMask ;
  }
  else
  {
    _rdsStationValid |= validMask ;
  }

  _rdsStationText[offsetData  ] = ch1 ;
  _rdsStationText[offsetData+1] = ch2 ;

  if (_rdsStationValid == 0x0f)
    _rdsStationStr.assign(_rdsStationText, 8) ;
}

bool               Si4703::rdsStationValid() const { return _rdsStationValid == 0x0f ; }
const std::string& Si4703::rdsStationText()  const { return _rdsStationStr ; }

void Si4703::rdsTime(uint16_t hi, uint16_t lo)
{
  _rdsTimeTimer.restart() ;

  _rdsTimeM = ( lo & 0b0000111111000000) >>  6 ;
  _rdsTimeH = ((hi & 0b0000000000000001) <<  4) |
              ((lo & 0b1111000000000000) >> 12) ;
  _rdsTimeS = 0 ;
  uint8_t offset     = (lo & 0b0000000000011111) ;
  uint8_t offsetSign = (lo & 0b0000000000100000) ;

  if (offsetSign) // negative / west
  {
    if (offset & 1) // half hour
    {
      if (_rdsTimeM < 30)
      {
        _rdsTimeM += 30 ;
        if (_rdsTimeH == 0)
          _rdsTimeH = 23 ;
        else
          _rdsTimeH -= 1 ;
      }
      else
      {
        _rdsTimeM -= 30 ;
      }
    }
    offset >>= 1 ; // hours
    if (_rdsTimeH < (0+offset))
      _rdsTimeH = _rdsTimeH + 24 - offset ;
    else
      _rdsTimeH -= offset ;
  }
  else // positive / east
  {
    if (offset & 1) // half hour
    {
      if (_rdsTimeM >= 30)
      {
        _rdsTimeM -= 30 ;
        if (_rdsTimeH == 23)
          _rdsTimeH = 0 ;
        else
          _rdsTimeH += 1 ;
      }
      else
      {
        _rdsTimeM += 30 ;
      }
    }
    offset >>= 1 ; // hours
    if (_rdsTimeH >= (24-offset))
      _rdsTimeH = _rdsTimeH - 24 + offset ;
    else
      _rdsTimeH += offset ;
  }

  ::RV::toStr(_rdsTimeH, _rdsTimeText + 0, 2, '0') ;
  ::RV::toStr(_rdsTimeM, _rdsTimeText + 3, 2, '0') ;
  ::RV::toStr(_rdsTimeS, _rdsTimeText + 6, 2, '0') ;
  _rdsTimeText[2] = ':' ;
  _rdsTimeText[5] = ':' ;

  _rdsTimeValid = 0x0f ;
}

bool        Si4703::rdsTimeValid() const { return _rdsTimeValid == 0x0f ; }
std::string Si4703::rdsTimeText()
{
  while (_rdsTimeTimer())
  {
    _rdsTimeS += 1 ;
    if (_rdsTimeS >= 60)
    {
      _rdsTimeS -= 60 ;
      _rdsTimeM += 1 ;
      if (_rdsTimeM >= 60)
      {
        _rdsTimeM -= 60 ;
        _rdsTimeH += 1 ;
        if (_rdsTimeH >= 24)
        {
          _rdsTimeH -= 24 ;
        }
      }
    }

    ::RV::toStr(_rdsTimeH, _rdsTimeText + 0, 2, '0') ;
    ::RV::toStr(_rdsTimeM, _rdsTimeText + 3, 2, '0') ;
    ::RV::toStr(_rdsTimeS, _rdsTimeText + 6, 2, '0') ;
    _rdsTimeText[2] = ':' ;
    _rdsTimeText[5] = ':' ;
  }
  
  return std::string(_rdsTimeText, 8) ;
}

void Si4703::rdsText(uint8_t offset, char ch1, char ch2, char ch3, char ch4)
{
  uint8_t offsetData = offset << 2 ;
  uint16_t validMask = 1 << offset ;
  uint8_t len = 0xff ;
    
  if (_rdsTextValid & validMask)
  {
    if ((_rdsTextText[offsetData  ] == ch1) &&
        (_rdsTextText[offsetData+1] == ch2) &&
        (_rdsTextText[offsetData+2] == ch3) &&
        (_rdsTextText[offsetData+3] == ch4))
      return ; // no change

    _rdsTextLen = 64 ;
    _rdsTextValid = validMask ;
  }
  else
  {
    _rdsTextValid |= validMask ;
  }

  _rdsTextText[offsetData+3] = ch4 ; if (ch4 == '\r') len = offsetData + 3 ;
  _rdsTextText[offsetData+2] = ch3 ; if (ch3 == '\r') len = offsetData + 2 ;
  _rdsTextText[offsetData+1] = ch2 ; if (ch2 == '\r') len = offsetData + 1 ;
  _rdsTextText[offsetData  ] = ch1 ; if (ch1 == '\r') len = offsetData + 0 ;
  
  if (len != 0xff)
  {
    _rdsTextLen = len ;
    for (uint16_t i = validMask ; i ; i <<= 1)
      _rdsTextValid |= i ;
  }

  if (_rdsTextValid == 0xffff)
    _rdsTextStr.assign(_rdsTextText, _rdsTextLen) ;
}

bool               Si4703::rdsTextValid() const { return _rdsTextValid == 0xffff ; }
const std::string& Si4703::rdsTextText()  const { return _rdsTextStr ; }


void Si4703::rds()
{
  if (!_irqRdsInd)
    return ;
  _irqRdsInd = false ;
  
  read(6) ;
  switch (_rdsB & 0xf800)
  {
  case 0x0000: // 0 A Basic tuning and switching information
  case 0x0800: // 0 B
    rdsStation(_rdsB & 0x03, _rdsD >> 8, _rdsD & 0xff) ;
    return ;

  case 0x2000: // 2 A RadioText only
    rdsText(_rdsB & 0x0f, _rdsC >> 8, _rdsC, _rdsD >> 8, _rdsD) ;
    return ;

  case 0x2800: // 2 B RadioText only
    {
    }
    return ;

  case 0x4000: // 4 A Clock-time and date only
    rdsTime(_rdsC, _rdsD) ;
    return ;

  }
}
  
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

