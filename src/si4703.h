////////////////////////////////////////////////////////////////////////////////
// si4703.h
////////////////////////////////////////////////////////////////////////////////

class Si4703
{
public:
  
  Si4703() ;

  bool setup() ;

  bool read(uint8_t cnt = 0x10) ;
  bool write(uint8_t cnt = 0x10) ;

  bool enable() ;
  bool disable() ;

  bool seek(bool up) ;
  bool volume(bool up) ;
  uint8_t volume() ;
  bool stereo(bool on) ;
  bool stereo() ;
  bool stereoInd() ;
  uint16_t channel() ; // in 100kHz
  bool channel(uint16_t chan) ; // in 100kHz
  uint8_t rssi() ;
  const uint16_t* rds() ;

  //uint16_t data(uint8_t idx) ;
  
private:
#pragma pack(push, 1)
  union
  {
    uint16_t _data[0x10] ;
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
      uint16_t _rdsa ;        // 12
      uint16_t _rdsb ;        // 13
      uint16_t _rdsc ;        // 14
      uint16_t _rdsd ;        // 15
    } ;      
  } ;
#pragma pack(pop)
  
} ;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
