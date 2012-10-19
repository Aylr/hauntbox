# Read Me

Welcome to the source code of the Hauntbox.

## Hardware Requirements
- Arduino MEGA (or an all in one hauntbox board)
- Ethernet Shield
- SD card

## Software Requirements
- A browswer capable of zeroconf/bonjour. If you have a Mac, iPhone, iPad, iOS device, a Windows machine with iTunes installed you are just fine.
- twitter's BOOTSTRAP framework (in SD Web Files)
- Arduino libraries:
	- TinyWebServer (requires Flash.h library) https://github.com/ovidiucp/TinyWebServer
	- Ethernet.h
	- SD.h
	- EthernetBonjour.h (for easy network discovery) https://github.com/neophob/EthernetBonjour
	- ...

## Installation
- Download the SD Web Files folder and put all the contents onto an empty, formatted SD card
- Insert that SD card into your hauntbox/arduino
- Plug the hauntbox/arduino into your network
- Power the hauntbox/arduino
- Make sure your controlling device (laptop, desktop, smartphone, tablet, etc) is on WiFi **on the same network** as the hauntbox.
- Open your browser and navigate to http://hauntbox.local
- Enjoy configuring your hauntbox.