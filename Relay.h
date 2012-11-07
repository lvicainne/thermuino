/*
* Thermuino, a control heat with Arduino
* Copyright (C) 2012 Louis VICAINNE <louis.vicainne@gmail.com>
* http://www.vicainne.fr
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* A part of the file has been created by SiliciumCorp.com
*/

#ifndef RELAY_HEADER
#define RELAY_HEADER

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class Relay {
	private:
		int pin;
		boolean isOFF;

	public:
		Relay(int mypin);
		void setOff();
		void setOn();
		boolean isStateOFF();
		void switchState();
};

#endif
