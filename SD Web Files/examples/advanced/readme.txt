## Advanced: Multi-switch single light control

Imagine a maze with a single light source that can be controlled by 3 switches hidden in different sections of the maze. It is like a 3-way switch you might have in your house, but awesomer. The main light can be toggled on/off from any push-button switch. This is 3:1 control.

When [push switch 1] is [on] wait [0] seconds, then turn [main light]  [toggled] [until further notice]
When [push switch 2] is [on] wait [0] seconds, then turn [main light]  [toggled] [until further notice]
When [push switch 3] is [on] wait [0] seconds, then turn [main light]  [toggled] [until further notice]

settings:
1,1,1,1,1,1,1,1,1,1,1,1;door 1,motion sensor 1,motion sensor 2,push switch 1,push switch 2,push switch 3;main light,siren,scary music,strobe light,scary prop,prop spotlight;103,103,103,103,103,103;100,100,100,100,100,100;
program:
1,1,1;4,5,6;1,1,1;0,0,0;1,1,1;2,2,2;0,0,0;0,0,0;