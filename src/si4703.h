////////////////////////////////////////////////////////////////////////////////
// si4703.h
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <cstdint>

#pragma once

class Si4703
{
public:
  
  Si4703() ;
  Si4703(const Si4703&) = delete ;
  Si4703& operator=(const Si4703&) = delete ;
  
  bool setup() ;

  bool read(uint8_t cnt = 0x10) const ;
  bool write(uint8_t cnt = 0x10) ;

  bool enable() ;
  bool disable() ;

  bool seek(bool up) ;
  bool volume(bool up) ;
  uint8_t volume() const ;
  bool stereo(bool on) ;
  bool stereo() const ;
  bool stereoInd() const ;
  uint16_t channel() const ; // in 100kHz
  bool channel(uint16_t chan) ; // in 100kHz
  uint8_t rssi() const ;
  
  void rds() ;
  bool rdsStationValid() const ;
  std::string rdsStationText() const ;
  bool rdsTimeValid() const ;
  std::string rdsTimeText() const ;

private:
  void rdsStation(uint8_t offset, char ch1, char ch2) ;
  void rdsTime(uint16_t hi, uint16_t lo) ;
  
#pragma pack(push, 1)
  union
  {
    mutable uint16_t _data[0x10] ;
    struct
    {
      uint16_t _deviceId ;    //  0
      uint16_t _chipId ;      //  1
      uint16_t _powerCfg ;    //  2
      uint16_t _channel ;     //  3
      uint16_t _sysConfig1 ;  //  4
      uint16_t _sysConfig2 ;  //  5
      uint16_t _sysConfig3 ;  //  6
      uint16_t _test1 ;       //  7
      uint16_t _test2 ;       //  8
      uint16_t _bootConfig ;  //  9
      uint16_t _statusRssi ;  // 10
      uint16_t _readChan ;    // 11
      uint16_t _rdsA ;        // 12
      uint16_t _rdsB ;        // 13
      uint16_t _rdsC ;        // 14
      uint16_t _rdsD ;        // 15
    } ;      
  } ;
#pragma pack(pop)
  volatile bool _irqRdsInd{false} ;

  char    _rdsStationText[8] ;
  uint8_t _rdsStationValid{0x00} ;
  char    _rdsTimeText[5] ;
  uint8_t _rdsTimeValid{0x00} ;
} ;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
