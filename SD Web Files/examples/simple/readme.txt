# Simple Example with 1:1 control

Imagine a room with a door sensor, 2 motion sensors in different parts of the room and pressure switch. This room has a desk lamp, a siren, music and a strobe light. In this example, each 1 input is mapped to 1 output. This is 1:1 control.

When [door 1] is [on] wait [0] seconds, then turn [desk lamp] [on] for [30] seconds
When [motion sensor 1] is [on] wait [0] seconds, then turn [siren] [on] [while input is triggered]
When [motion sensor 2] is [off] wait [30] seconds, then turn [music] [off] [while input is triggered]
When [pressure switch 1] is [on] wait [5] seconds, then turn [strobe light] [on] [for [20] seconds

settings:
1,1,1,1,1,1,1,1,1,1,1,1;door 1,motion sensor 1,motion sensor 2,pressure switch,,;desk lamp,siren,music,strobe light,,;103,103,103,103,103,103;100,100,100,100,100,100;
program:
1,1,1,1;1,2,3,4;1,1,1,1;0,0,0,15000;1,2,3,4;1,1,0,1;2,1,1,2;30000,5000,5000,20000;