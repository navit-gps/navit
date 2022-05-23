import navit
navit.config_load("navit.xml.local")
pos=navit.pcoord("5023.7493 N 00730.2898 E",1)
dest=navit.pcoord("5023.6604 N 00729.8500 E",1)
inst=navit.config().navit()
inst.set_position(pos)
inst.set_destination(dest,"Test")
inst.set_center(pos)
