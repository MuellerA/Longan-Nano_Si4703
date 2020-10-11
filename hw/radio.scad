////////////////////////////////////////////////////////////////////////////////
// radio.scad
////////////////////////////////////////////////////////////////////////////////

// https://github.com/MuellerA/Thingiverse.git#ac669d2e31545f4aca594795901329ed9a7b87eb
use <../../../3D Druck/Thingiverse/PCB/pcb.scad> ;

pcb = "longan" ; // [longan,si4703,button]


/* [Hidden] */

length = 46.5 ;
width  = 26 ;
wallWidth  =  0.8 ;

$fn=40 ;

module base(height)
{
  translate([0, 0, height/2])
    difference()
  {
    translate([0, 0, -wallWidth/2])
      cube([length + 2*wallWidth, width + 2*wallWidth, height+wallWidth], center=true) ;
    translate([0, 0, +wallWidth/2])
      cube([length + 0*wallWidth-0.01, width + 0*wallWidth-0.01, height+wallWidth+0.01], center=true) ;
  }
}

module hooks(height, top)
{
  w = wallWidth ;
  d = w/2 ;
  points =
    [
      [ [0, 0], [w, -w], [0, -2*w], [-w, -2*w], [-w, 0] ],
      [ [0, 0], [0, -2*w], [w+d, -2*w], [w+d, 0.5*w], [d, 1.5*w], [w+d, 2.5*w], [2*w+d, 2.2*w], [2*w+d, -3*w], [0, -6*w], [-w, -6*w], [-w, 0] ]
    ] ;

  module hook(points)
  {
    rotate([0, 0, 90])
      rotate([90, 0, 0])
      linear_extrude(height=3, center=true)
      polygon(points) ;
  }

  p1 = (top) ? points[0] : points[1] ;
  p2 = (top) ? points[1] : points[0] ;
  
  translate([+length/2-5, -width/2, height]) rotate([0, 0,   0]) hook(p1) ;
  translate([0          , -width/2, height]) rotate([0, 0,   0]) hook(p2) ;
  translate([-length/2+5, -width/2, height]) rotate([0, 0,   0]) hook(p1) ;
  translate([+length/2-5, +width/2, height]) rotate([0, 0, 180]) hook(p1) ;
  translate([0          , +width/2, height]) rotate([0, 0, 180]) hook(p2) ;
  translate([-length/2+5, +width/2, height]) rotate([0, 0, 180]) hook(p1) ;

  translate([+length/2  , 0       , height]) rotate([0, 0,  90]) hook(p1) ;
  translate([-length/2  , 0       , height]) rotate([0, 0, 270]) hook(p1) ;
}

module top()
{
  pcbLength  = 46.5 ;
  pcbWidth   = 20.5 ;
  pcbHeight  =  1.9 ;
  railHeight =  6.0 ;
  railOffset =  2.3 ;

  height = railHeight + pcbHeight + wallWidth + 1 ;
  
  dispLength = 23.9 ;
  dispWidth  = 13.5 ;
  dispOffset =  3.5 ;

  buttonD       = 3.0 ;
  buttonLoffset = pcbLength / 2 - 8.9 ;
  buttonWoffset = 3.5 ;

  usbWidth  = 9.9 ;
  usbHeight = 3.9 ;
  usbOffset = 2.0 ;
  
  difference()
  {
    union()
    {
      // base
      base(height) ;
      
      // pcb frame
      rotate([0, 0, 90])
        PcbHolder(pcbLength, pcbWidth, pcbHeight, wallWidth, railHeight, railOffset,
                  clip = [10, 5]) ;

      translate([-dispLength/2+dispOffset-1.5*wallWidth, 0, 2/2]) cube([wallWidth, 20 , 2], center=true) ;
      
      // button
      bd = buttonD + 2 * 0.4 ;
      translate([-buttonLoffset, buttonWoffset, 1/2]) cylinder(d=bd+2*wallWidth, h=1, center=true) ;      
    }

    union()
    {
      // display
      translate([dispOffset, 0, 0]) cube([dispLength, dispWidth, 10], center=true) ;

      // button
      bd = buttonD + 2 * 0.4 ;
      translate([-buttonLoffset, buttonWoffset, 0]) cylinder(d=bd, h=10, center=true) ;
      
      // usb
      translate([-length/2, 0, usbHeight/2+usbOffset])
        cube([10, usbWidth, usbHeight], center=true) ;
    }
  }

  hooks(height, true) ;
}

module button()
{
  cylinder(h=1, d=4.4) ;
  cylinder(h=6, d=2.5) ;
}

module bottom()
{
  pcbLength  = 22.8 ;
  pcbWidth   = 29.6 ;
  pcbHeight  =  1.9 ;
  railHeight =  3.0 ;
  railOffset =  5.0 ;

  height = railHeight + pcbHeight + 6.4 ;
  
  difference()
  {
    union()
    {
      base(height) ;

      PcbSi4703() ;
    }

    union()
    {
      translate([7, width/2, railHeight + pcbHeight + 3.5])
      cube([10, 5*wallWidth, 7], center=true) ;
    }
  }

  hooks(height, false) ;
}

if (pcb == "longan")
  top() ;
else if (pcb == "si4703")
  bottom() ;
else if (pcb == "button")
  button() ;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
