///////////////////////////////////////////////////////////////////////////////
// radio.cpp
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include "gd32vf103.h"
}

#include "Longan/lcd.h"
#include "Longan/fonts.h"

#include "si4703.h"

using ::RV::Longan::Lcd ;
using ::RV::Longan::LcdArea ;

Lcd &lcd(Lcd::lcd()) ;


int main()
{
  Si4703 si4703 ;

  lcd.setup() ;

  {
    LcdArea laTitle (lcd, 0, 110,  0, 16, &::RV::Longan::Roboto_Bold7pt7b , 0xffffffUL, 0xa00000UL) ;

    laTitle.clear() ;
    laTitle.put("  R A D I O  ") ;
  }
  LcdArea laData(lcd, 0, 160, 16, 64) ;
  
  if (!si4703.setup())
  {
    laData.put("Si4703 init failed") ;
    while (true) ;
  }

  si4703.read() ;

  si4703.enable() ;
  si4703.seek(true) ;
  
  while (true)
  {
    lcd.heartbeat() ;
  }
  
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
