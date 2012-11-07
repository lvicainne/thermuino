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

#include "DS1307.h"

DS1307::DS1307() {
    Wire.begin();
}

void DS1307::readWire(void) {
	Wire.beginTransmission(ID_REGISTER);
	Wire.write((byte) _00H_SEC); //Set the pointer position to the first byte
	Wire.endTransmission();

	Wire.requestFrom(ID_REGISTER, LENGTH_REGISTER);//Ask for 8 byte to the DS1307

	for(int i = 0; i < LENGTH_REGISTER; i++) {
		bufferRegister[i] = Wire.read(); //Store BCD data into the correct buffer
	}
}

void DS1307::save(void)  {
	Wire.beginTransmission(ID_REGISTER);
	Wire.write((byte) _00H_SEC); //Set the pointer position to the first byte

	for(int i = 0; i < LENGTH_REGISTER; i++) {
		Wire.write(bufferRegister[i]); //Save the BCD data into the DS1307
	}
    
	Wire.endTransmission();
}

void DS1307::convertBCDtoINT() {
	isCHenable = (bufferRegister[_00H_SEC] & CLOCKHALT_MASK) >> 7;
	is12Hmode = (bufferRegister[_02H_HRS] & CLOCK_MOD_MASK) >> 6;
	isOUTenable = (bufferRegister[_07H_PIN] & OUT_MASK) >> 7;
	isSQWEenable = (bufferRegister[_07H_PIN] & SQWE_MASK) >> 4;
	waveFrequency = (bufferRegister[_07H_PIN] & RS_MASK);
	
	timeRegister[_00H_SEC] = (10 * ((bufferRegister[_00H_SEC] & X3BIT_DECADE_MASK) >> 4)) + (bufferRegister[_00H_SEC] & X4BIT_UNITS_MASK);
	timeRegister[_01H_MIN] = (10 * ((bufferRegister[_01H_MIN] & X3BIT_DECADE_MASK) >> 4)) + (bufferRegister[_01H_MIN] & X4BIT_UNITS_MASK);

	if (is12Hmode) {
		timeRegister[_02H_HRS] = (10 * ((bufferRegister[_02H_HRS] & X1BIT_DECADE_MASK) >> 4)) + (bufferRegister[_02H_HRS] & X4BIT_UNITS_MASK);
		isPostMeridiem = (bufferRegister[_02H_HRS] & CLK_AM_PM_MASK) >> 5;
	} else {
		timeRegister[_02H_HRS] = (10 * ((bufferRegister[_02H_HRS] & X2BIT_DECADE_MASK) >> 4)) + (bufferRegister[_02H_HRS] & X4BIT_UNITS_MASK);
	}
	
	timeRegister[_03H_DOW] = bufferRegister[_03H_DOW] & X3BIT_UNITS_MASK;
	timeRegister[_04H_DAT] = (10 * ((bufferRegister[_04H_DAT] & X2BIT_DECADE_MASK) >> 4)) + (bufferRegister[_04H_DAT] & X4BIT_UNITS_MASK);
	timeRegister[_05H_MTH] = (10 * ((bufferRegister[_05H_MTH] & X1BIT_DECADE_MASK) >> 4)) + (bufferRegister[_05H_MTH] & X4BIT_UNITS_MASK);
	timeRegister[_06H_YRS] = (10 * ((bufferRegister[_06H_YRS] & X4BIT_DECADE_MASK) >> 4)) + (bufferRegister[_06H_YRS] & X4BIT_UNITS_MASK);
	timeRegister[_07H_PIN] = bufferRegister[_07H_PIN];
}

void DS1307::convertINTtoBCD() {
	bufferRegister[_00H_SEC] = (isCHenable << 7) | ((timeRegister[_00H_SEC] / 10) << 4) | (timeRegister[_00H_SEC] % 10);
	bufferRegister[_01H_MIN] = ((timeRegister[_01H_MIN] / 10) << 4) | (timeRegister[_01H_MIN] % 10);
    
    if (is12Hmode) {
		bufferRegister[_02H_HRS] = (is12Hmode << 6) | (isPostMeridiem << 5) | ((timeRegister[_02H_HRS] / 10) << 4) | (timeRegister[_02H_HRS] % 10);
    } else {
		bufferRegister[_02H_HRS] = ((timeRegister[_02H_HRS] / 10) << 4) | (timeRegister[_02H_HRS] % 10);
	}
	
	bufferRegister[_03H_DOW] = timeRegister[_03H_DOW];
	bufferRegister[_04H_DAT] = ((timeRegister[_04H_DAT] / 10) << 4) | (timeRegister[_04H_DAT] % 10);
	bufferRegister[_05H_MTH] = ((timeRegister[_05H_MTH] / 10) << 4) | (timeRegister[_05H_MTH] % 10);
 
	bufferRegister[_06H_YRS] = ((timeRegister[_06H_YRS] / 10) << 4) | (timeRegister[_06H_YRS] % 10);
	bufferRegister[_07H_PIN] = (isOUTenable << 7) | (isSQWEenable << 4) | waveFrequency;
}

void DS1307::updateRegister() {
	readWire();
	convertBCDtoINT();
}

void DS1307::updateParameter() {
	convertINTtoBCD();
	save();
}

void DS1307::startClock() {
	isCHenable = false;
	updateParameter();
}

void DS1307::stopClock() {
	updateRegister();
	isCHenable = true;
	updateParameter();
}

void DS1307::startWave() {
	updateRegister();
	isSQWEenable = true;
	isOUTenable = true;
	updateParameter();
}

void DS1307::stopWave() {
	updateRegister();
	isSQWEenable = false;
	updateParameter();
}

void DS1307::startOUT() {
	updateRegister();
	isOUTenable = true;
	updateParameter();
}

void DS1307::stopOUT() {
	updateRegister();
	isOUTenable = false;
	isSQWEenable = false;
	updateParameter();
}

void DS1307::setSecond(int second) {
    if ( second > -1 && second < 61) {
		timeRegister[_00H_SEC] = second;
	}
}

void DS1307::setMinute(int minute) {
	if ( minute > -1 && minute < 61) {
		timeRegister[_01H_MIN] = minute;
	}
}

void DS1307::setHour(int hour) {
    if (is12Hmode) {
		if ( hour > 0 && hour < 13) {
			timeRegister[_02H_HRS] = hour;
		}
    } else {
		if ( hour > -1 && hour < 24) {
			timeRegister[_02H_HRS] = hour;
		}

    }
}

void DS1307::setDay(int day) {
    if(day > 0 && day < 32) {
		timeRegister[_04H_DAT] = day;
	}
}

void DS1307::setDayOfMonth(int dow) {
	if(dow > 0 && dow < 8 ) {
		timeRegister[_03H_DOW] = dow;
	}
}

void DS1307::setMonth(int month) {
    if(month > 0 && month < 13) {
		timeRegister[_05H_MTH] = month;
	}
}

void DS1307::setYear(int year) {
	if (year > -1 && year < 100) {
		timeRegister[_06H_YRS] = year;
	}
}

void DS1307::setTime(int hour, int minute, int second) {
    setSecond(second);
    setMinute(minute);
    setHour(hour);
}

void DS1307::setCalendar(int dow, int day, int month, int year) {
    setDay(day);
    setDayOfMonth(dow);
    setMonth(month);
    setYear(year);
}

void DS1307::setTimeRegister(int hour, int minute, int second, int dow, int day, int month, int year) {
    setTime(hour, minute, second);
    setCalendar(dow, day, month, year);
}

void DS1307::setWaveFrequency(int frequence)  {
    if (frequence > -1 && frequence < 5) {
		waveFrequency = frequence;
    }
}

void DS1307::switchMeridiem() {
    updateRegister();
    if (is12Hmode) {
        if (isPostMeridiem) {
            isPostMeridiem = false;

        } else {
            isPostMeridiem = true;

        }
    }
    updateParameter();
}

void DS1307::switchMode() {
    updateRegister();
    if(is12Hmode) {
        if (isPostMeridiem == true && (timeRegister[_02H_HRS] != 12)) {
            timeRegister[_02H_HRS] += 12;

        } else if (isPostMeridiem == false && (timeRegister[_02H_HRS] == 12)) {
            timeRegister[_02H_HRS] = 0;
        }
		is12Hmode = false;
		isPostMeridiem = false;

    } else {
		is12Hmode = true;
        if (timeRegister[_02H_HRS] > 12 && timeRegister[_02H_HRS] < 24) {
            isPostMeridiem = true;
            timeRegister[_02H_HRS] -= 12;

        } else if (timeRegister[_02H_HRS] == 12) {
            isPostMeridiem = true;

        } else if (timeRegister[_02H_HRS] == 0) {
            isPostMeridiem = false;
            timeRegister[_02H_HRS] = 12;
        }
    }
    updateParameter();
}


boolean DS1307::getIs12Hmode(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(is12Hmode);
}

boolean DS1307::getIsPostMeridiem(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(isPostMeridiem);
}

boolean DS1307::getIsCHenable(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(isCHenable);
}

boolean DS1307::getIsSQWEenable(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(isSQWEenable);
}

boolean DS1307::getIsOUTenable(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(isOUTenable);
}

int DS1307::getWaveFrequency(boolean isRefresh) {
    if(isRefresh) {
        updateRegister();
    }
    return(waveFrequency);
}

int* DS1307::getTimeRegister(boolean isRefresh) {
    if(isRefresh)     {
        updateRegister();
    }
    return(timeRegister);
}
