Lasers and Logic - A Sci-Fi LARP
==================================================================================================
The code provided here is for establishing a laser tag system to be used in a science fiction based
LARP (Live Action Role Play) Game. The code is meant to handle I/O from RF and IR devices, calculate
damage, and store items that the players can use. For more information about the game and schematics
for devices, go to our site at lasersandlogic.weebly.com.

NOTES:
  The Lasers and Logic code base uses the arduino libraries IRremote and VirtualWire. The files for
  those can be found at https://github.com/shirriff/Arduino-IRremote and 
  http://www.pjrc.com/teensy/td_libs_VirtualWire.html. Make sure you set IRremote to use Timer 2 on the
  arduino, otherwise VirtualWire and IRremote will have conflicts.
  
FILES:  
PlayerControl.ino
  This file is for handling each player's gun damage, items, AoE (Area of Effect) weapons, etc.
  
STracker.ino
  This file is for controlling a motion tracking turret with an IR LED for shooting players.

