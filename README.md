# Using your Hauntbox.

## Things you'll need.

1. A 12V or 24V DC power supply.
2. A formatted microSD card.
3. A wired network connection.
4. Some sensors and interesting things to hook up to the outputs.
5. A sense of adventure.

## Initial overview and set up

<a href="http://www.flickr.com/photos/hauntbox/8748714580/" title="IO Diagrams by hauntbox, on Flickr"><img src="http://farm8.staticflickr.com/7289/8748714580_894bc39b7f_c.jpg" width="618" height="800" alt="IO Diagrams"></a>

**Note: It is very important to protect the Hauntbox from shorts that could damage the sensitive electronics. Never change wiring to inputs or outputs when it is plugged into a power supply.**

1. Wire your sensors to the inputs on the Hauntbox. They are along the top of the board labeled A through F (more below)
2. Wire up the things you want to control to the outputs. They are along the bottom of the Hauntbox labeled 1 through 6. Usually these will be wired from the 5V or +V to the OC (the switched ground).
3. Plug in your network cable to the CAT5 network jack on the left side of the Hauntbox.
4. Plug in your power supply and plug it into the power jack on the left side of the Hauntbox.
5. Fire up your favorite browser and browse to http://hauntbox.local (Note that some browsers do require the "http://").
	- An aside about bonjour. If you have more than one Hauntbox on the same network you **must** change the name to be unique on the network. This is easily done by popping the microSD card out of your Hauntbox and making a file called "bonjour.txt" on the card. Edit this with your favorite text editor (Notepad on Windows, TextEdit on Mac, nano on Linux) and make up a name less than 16 characters that only has letters and numbers. No special characters. Put the card back into your Hauntbox and reset. Now browse to "http://yourname.local"
6. First take a look at the **settings** tab. Rename your sensors and outputs so you can easily see what you're controlling. Be sure to click "Save" when you're done.
7. Next go the the **program** tab. This is where the fun is. Each row is a sentence that you fill out. Press save and see the Hauntbox do your bidding.

## Input & Output details

### Each input has four terminals:

<a href="http://www.flickr.com/photos/hauntbox/9818718746/" title="input_details_v01 by hauntbox, on Flickr"><img src="http://farm8.staticflickr.com/7400/9818718746_3e6be9f287.jpg" width="437" height="408" alt="input_details_v01"></a>

1. **+V** This is 12V if you're using a 12V power supply or 24V if you use a 24V supply. Think of it as (+) on a battery.
2. **5V** This is 5V from the onboard power supply. Think of it as (+) on a battery.
3. **GND** Ground. Sometimes referred to as RTN (power return). Think of it as (-) on a battery.
4. **[A-F] in** This is the actual input. Hook up the output of your sensor here. It is an analog pin that can accept up to 24V DC signals.
	- It is important in the **settings** page to select the appropriate voltage to make sure the trigger thresholds are set properly.

### Each output has four terminals:

<a href="http://www.flickr.com/photos/hauntbox/9818844054/" title="output_details_v01 by hauntbox, on Flickr"><img src="http://farm3.staticflickr.com/2874/9818844054_97f0f887a9.jpg" width="436" height="407" alt="output_details_v01"></a>

1. **+V** This is 12V if you're using a 12V power supply or 24V if you use a 24V supply. Think of it as (+) on a battery.
2. **5V** This is 5V from the onboard power supply. Think of it as (+) on a battery.
3. **GND** Ground. Sometimes referred to as RTN (power return). Think of it as (-) on a battery.
4. **OC [1-6]** This is the actual switched output. It is called an "open collector" which simply means it is a switched ground (GND) so you can control devices of varying voltages. This is the secret sauce how you can use either a 5V, 12V or 24V device. It can handle up to 300mA, which is enough to power some solenoids, big relays, and many other things. It has solid state auto-resetting fuse to help protect your Hauntbox from mistakes.

### Logic Level Output terminals:

**L1, L2, L3, L4, L5, L6** These are TTL logic (0V - 5V) outputs. Don't know what that means? -then you probably won't use (or need) these outputs!  They are there for the more advanced hacker or haunter who wants to trigger other micro-controllers or other electronic devices that are compatible with 5V logic signals.  These outputs are triggered along with the OC triggers: **L1** is triggered along with **OC1**, **L2** is triggered along with **OC2**, etc.

## Example wiring

### Input: Switch / Reed Switch (door sensor) hook up

<a href="http://www.flickr.com/photos/hauntbox/9818718786/" title="door_switch_v01 by hauntbox, on Flickr"><img src="http://farm3.staticflickr.com/2840/9818718786_8da2962432.jpg" width="500" height="239" alt="door_switch_v01"></a>

1. Hook up **Terminal 1** to your Hauntbox input **+5V** (or **+V**)
2. Hook up **Terminal 2** to your Hauntbox input **[A-F] in**.
3. Select the appropriate voltage in the **settings** page to make sure the trigger thresholds are set properly.

### Input: Motion Sensor hook up

<a href="http://www.flickr.com/photos/hauntbox/9818781793/" title="motion_sensor_v01 by hauntbox, on Flickr"><img src="http://farm6.staticflickr.com/5461/9818781793_6b826773c5.jpg" width="427" height="500" alt="motion_sensor_v01"></a>

1. Hook up motion sensor **+5V** to your Hauntbox input **+5V**.
2. Hook up motion sensor **GND** to your Hauntbox input **GND**.
3. Hook up motion sensor **OUT** to your Hauntbox input **[A-F] in**.
4. Select the appropriate voltage in the **settings** page to make sure the trigger thresholds are set properly.

### Output: Hauntbox Sound Module hook up

[Insert picture here]>

1. Hook up sound module **V in** to your Hauntbox output **+V** (sound module runs on 12V (or 24V) power).
2. Hook up sound module **GND** to your Hauntbox output **GND**
3. Hook up sound module **OC TRG** (open collector trigger) to your Hauntbox output **OC [1-6]**. Note that you can hook up sound module **LOG TRG** (logic trigger) to your Hauntbox output **L [1-6]** instead.

### Output: Powertail hook up

<a href="http://www.flickr.com/photos/hauntbox/8761210491/" title="powertail hook up by hauntbox, on Flickr"><img src="http://farm3.staticflickr.com/2836/8761210491_dd47c09bb7_c.jpg" width="800" height="650" alt="powertail hook up"></a>

The Powertail has 3 terminals, only two of which you need. (You can ignore "3: Ground" which is a chassis/earth ground).

1. Hook up Powertail **1: +in** to your Hauntbox output **+5V**.
2. Hook up Powertail **2: -in** to your Hauntbox output **OC [1-6]**.
3. Plug the Powertail into a 120V outlet
4. Plug in whatever you want to control to the Powertail. Make sure the device is on so the Powertail can do the switching.