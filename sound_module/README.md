# Hauntbox Sound Module Instructions

<a href="http://hauntbox.net"><img src="http://hauntbox.net/storage/remotelogo/hblogo.png"></a>
<a href="http://www.flickr.com/photos/hauntbox/9821918795/" title="sound_module_overview_v01 by hauntbox, on Flickr"><img src="http://farm3.staticflickr.com/2839/9821918795_dd0524d179_c.jpg" width="800" height="798" alt="sound_module_overview_v01"></a>

- The sound module can operate in a few modes depending on your wishes.
- Normally, the sound module loops your "ambient.wav" sound file until it is triggered on either of the trigger screw terminals.
- When triggered, it will play a single .wav file.
- If you have multiple files, it will keep track and play the next numbered file on the next trigger.
- Want to mix it up? Make a file called "random.txt" on the SD card
- **ALWAYS reset your sound module after inserting an SD card**

## The Files

- **ambient.wav**: This is the optional background ambient noise/music/etc you want looped. Want silence between triggers? delete this file.
- **trigger sounds**: You can have up to **30** sounds named numerically from "0.wav" to "29.wav"
- **random.txt**: If you create a file named "random.txt" and put it on the SD card whenever you trigger the module a random trigger sound will be played.

## Sound File Details

- The Hauntbox sound modules requires .wav files. These can be generated from many programs including Audacity (audacity.org), iTunes (apple.com/itunes), and many other sound editing programs.

## Pinout details

### Input terminals:

1. **V in** (Voltage Input). This is to be connected to **+V** of the Hauntbox. Note that this can be connected to (+) of a 12V (or 24V) DC power supply if used as a stand-alone device.
2. **GND** (Ground). This is to be connected to **GND** of the Hauntbox. Note that this can be connected to (-) of a 12V (or 24V) DC power supply if used as a stand-alone device.
3. **OC TRG** (Open-Collector Trigger). This triggers the sound module when the trigger signal goes low or is connected to GND. It is typically triggered by **OC [1-6]** of the Hauntbox.
4. **LOG TRG** (Logic Trigger). This triggers the sound module when the trigger signal goes high (5V, 12V, or 24V DC).  It can be triggered by a logic pulse from a micro-controller (like an Arduino) or by **L [1-6]** of the Hauntbox.

### Output terminals:

1. **SPK +** (Speaker +). This is the positive (+) connection to drive a speaker directly from the sound module.
2. **SPK -** (Speaker -). This is the positive (+) connection to drive a speaker directly from the sound module.

Note that the 3.5mm jack can be used instead of the speaker screw terminals.  Also note that the volume can be adjusted by turning the potentiometer labeled **VOL**.

## Wiring Examples

### Hauntbox to Sound Module hook up

<a href="http://www.flickr.com/photos/hauntbox/9821935396/" title="sound_module_wiring_v01 by hauntbox, on Flickr"><img src="http://farm4.staticflickr.com/3786/9821935396_ebfab7807c_c.jpg" width="522" height="800" alt="sound_module_wiring_v01"></a>

1. Hook up Sound Module **V in** to your Hauntbox output **+V**.
2. Hook up Sound Module **GND** to your Hauntbox output **GND**.
3. Hook up Sound Module **OC TRG** to your Hauntbox output **OC [1-6]**. Note that you can hook up Sound Module **LOG TRG** to your Hauntbox output **L [1-6]** instead.

### Reed switch (door sensor) to Sound Module hook up

[Insert picture here]

This scenario is for triggering the Hauntbox Sound Module directly from a reed switch, without using a Hauntbox controller. It requires a separate 12V (or 24V) DC power supply.

1. Hook up Sound Module **V in** to **(+)** of a 12V DC power supply.
2. Hook up Sound Module **GND** to **(-)** of a 12V DC power supply.
3. Hook up reed switch **Terminal 1** to Sound Module **V in**.
4. Hook up reed switch **Terminal 2** to Sound Module **LOG TRG**.

## Where to get help

- For the most current instructions, tips, tricks and hacks, visit [hauntbox.net](http://hauntbox.net) and [our wiki https://github.com/Aylr/theHB/wiki](https://github.com/Aylr/theHB/wiki) . Also, please help us make it better.
- Find ideas and tutorials on [our youtube channel](http://www.youtube.com/thehauntbox)
- We are [@hauntbox](http://twitter.com/hauntbox) on twitter. [Tweet to us](https://twitter.com/intent/tweet?screen_name=hauntbox)
- Feel free to email us at [info@hauntbox.net](info@hauntbox.net)
- Please do your duty and [report all bugs](https://github.com/Aylr/theHB/issues/new)
- You can find, hack, and contribute to [all our source code at github](https://github.com/Aylr/theHB/).
- A mad shoutout to our [awesome Kickstarter backers](http://www.kickstarter.com/projects/1020117671/hauntbox/backers) who made this project possible.