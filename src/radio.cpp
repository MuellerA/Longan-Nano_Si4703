///////////////////////////////////////////////////////////////////////////////
// radio.cpp
////////////////////////////////////////////////////////////////////////////////

#include "si4703.h"
#include "display.h"
#include "menu.h"

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
    disp.error("Si4703 init failed") ;
    while (true) ;
  }

  Menu menu
    {
      disp,
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
          "Seek (+)", [&si4703, &disp]()
                      {
                        si4703.seek(true) ;
                        return false ;
                      }
        },
        {
          "Seek (-)", [&si4703, &disp]()
                      {
                        si4703.seek(false) ;
                        return false ;
                      } // todo
        }
      },
      [&disp](){ disp.update(false, true) ; }
    } ;

  disp.clear() ;
  menu.setup() ;
  
  while (true)
  {
    menu.select() ;
    disp.update(false, false) ;
  }
  
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
