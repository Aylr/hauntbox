## Medium Example: Scary Room Scene

Imagine a room where scary music comes on, lights turn out, and a prop is triggered all from 1 motion sensor. This is 1:4, where 1 input controls 4 outputs, each with different timing.

When [motion sensor 1] is [on] wait [2] seconds, then turn [Scary Music] [toggled] for [1] seconds
When [motion sensor 1] is [on] wait [5] seconds, then turn [main light] [off] for [30] seconds
When [motion sensor 1] is [on] wait [10] seconds, then turn [scary prop] [on] for [20] seconds
When [motion sensor 1] is [on] wait [10] seconds, then turn [prop spotlight] [on] for [20] seconds

settings:
1,1,1,1,1,1,1,1,1,1,1,1;door 1,motion sensor 1,motion sensor 2,pressure switch,,;main light,siren,scary music,strobe light,scary prop,prop spotlight;103,103,103,103,103,103;100,100,100,100,100,100;
program:
1,1,1,1;2,2,2,2;1,1,1,1;2000,5000,10000,10000;3,1,5,6;1,0,1,1;2,2,2,2;60000,30000,20000,20000;