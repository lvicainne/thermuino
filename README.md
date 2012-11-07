Thermuino
=========

Thermuino is an Arduino project for managing your heat.
In other words, Thermuino is a thermostat connected to the Internet.
It contains a sensor temp, a LCD screen and multiple buttons for interact with the box.

The aim is to build a system which can control your heat via Real buttons or via Computer Apps.

Currently, the project is a prototype and DO NOT WORK.
Thus, begin to help developpers before use it ;-)

**Nota : there is actually some files but they have never been tested in real conditions**


Programmation Environment
-------------------------

The project is currently compatible with the last version of Arduino wich is **v1.0**.
Thus, the sketch file has the ".ino" extension instead of the ".pde" one.

Hardware requiries
------------------

With the involve of the project, we currently use an Arduino Ethernet Rev v3.
Actually, you could use an rduino Uno or Arduino Mega with their Ethernet Shield but, be carreful, the pins are not the same !

What do you need :
* Arduino Ethernet
* LCD Screen (20x40 but you could use a 16x2 with some adaptations) **compatible HD44780**
* Temp sensors (the DS18B20 with the OneWire protocol is cool)
* Real Time Clock (not a requirement but useful) : the DS1307, used with I2C protocol and a battery.
* Relay compatible with your heat to control it. We use a 5v/230v relay compatible with the French electricity.
* Buttons for interacting with your thermostat without any PC

There are some bonus :
* A Box for all keeping from dust
*  etc.


Hardware configuration
-----------------------
For easing the use of the project, this is the PIN attributions :

* For the LCD, we use Analog pins as digital pins.
- LCD_RS : pin A1
- LCD_ENABLE : pin A2
- LCD_DATA4 : pin 
- LCD_DATA5 : pin 
- LCD_DATA6 : pin 
- LCD_DATA7 : pin 

* For the inputs :
-

* For the outputs :
- 

**Nota : pins 10, 11, 12, 13 are used for Ethernet on Arduino Ethernet Boards. You can't use it comfortably for other situations**


