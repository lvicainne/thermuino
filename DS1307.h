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

#ifndef DS1307_HEADER
#define DS1307_HEADER 

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <Wire.h>

#define _00H_SEC 0
#define _01H_MIN 1
#define _02H_HRS 2
#define _03H_DOW 3
#define _04H_DAT 4
#define _05H_MTH 5
#define _06H_YRS 6
#define _07H_PIN 7
 
#define _1Hz 0
#define _4kHz 1
#define _8kHz 2
#define _32kHz 3
 
#define START_YEAR_REGISTER 2000
#define LENGTH_REGISTER 8 //8 data blocks given by the DS1307
#define LENGTH_DATE_REGISTER 4
#define LENGTH_CLOCK_REGISTER 3
#define ID_REGISTER B1101000  
 
/*BINARY MASKS*/
#define X4BIT_UNITS_MASK     B00001111
#define X3BIT_UNITS_MASK     B00000111
 
#define X4BIT_DECADE_MASK    B11110000
#define X3BIT_DECADE_MASK    B01110000
#define X2BIT_DECADE_MASK    B00110000
#define X1BIT_DECADE_MASK    B00010000
 
#define CLOCKHALT_MASK       B10000000
#define CLOCK_MOD_MASK       B01000000
#define CLK_AM_PM_MASK       B00100000
#define OUT_MASK             B10000000
#define SQWE_MASK            B00010000
#define RS_MASK              B00000011

class DS1307 {
	private:
		byte bufferRegister[LENGTH_REGISTER];
		int timeRegister[LENGTH_REGISTER];
		void readWire(void);
		void save(void); 
		
		void convertBCDtoINT();
		void convertINTtoBCD();
		
		void updateParameter();                    //Permet d'effectuer une mise à jours des parametres.
		void updateRegister();                     //Permet une mise a jour du registre BCD de la classe
 
		
		boolean is12Hmode;          //True : Mode 12h selectioné
		boolean isPostMeridiem;     //True : En mode 12h : PM
		boolean isCHenable;         //True : Horloge eteinte
		boolean isOUTenable;        //True : Pin 7 sous tension
		boolean isSQWEenable;       //True : Signal carré en fonction
	
		int waveFrequency;          //Fréquence du signal carré : 0=1hz, 1=4KHz, 2=8KHz, 3=32KHz
 
	
	public:
		DS1307();
		
		int* getTimeRegister(boolean = true);           //Donne heure, date et pin 7
		boolean getIs12Hmode(boolean = true);           //Donne son mode de fonctionnement (12/24h). true = 24h
		boolean getIsPostMeridiem(boolean = true);      //Donne l'etat de la gestion AM/PM (True si refresh demandé)
		boolean getIsCHenable(boolean = true);          //Donne l'état de fonctionnement de l'horloge. true = horloge off
		boolean getIsSQWEenable(boolean = true);        //Donne l'état du signal carré en sortie du pin 7. true = signal ON
		boolean getIsOUTenable(boolean = true);         //Donne l'état logique du pin 7.
		int getWaveFrequency(boolean = true);           //Donne la fréquence du signal carré en sortie du pin 7 0=1hz, 1=4KHz, 2=8KHz, 3=32KHz
	
		void setTime(int,int,int);                             //Régle l'heure setTime(heures, minutes, secondes).
		void setCalendar(int,int,int,int);                     //Régle la date setCalendar (jour de la semaine, date, mois, année)
		void setTimeRegister(int,int,int,int,int,int,int);     //Régle date et heure setTimeRegister
		void setSecond(int);                                   //Régle les secondes
		void setMinute(int);                                   //Régle les minutes
		void setHour(int);                                     //Régle les heures
		void setDay(int);                                      //Régle les jour du mois
		void setDayOfMonth(int);                               //Régle le jour de la semaine
		void setMonth(int);                                    //Régle les mois
		void setYear(int);                                     //Régle les années
		void setWaveFrequency(int);                            //Régle la fréquence du signal carré // frequences : 0=1hz, 1=4KHz, 2=8KHz, 3=32KHz                               
	
		void startOUT();                  //Place l'état logique du port de sortie du pin 7 à 1.
		void stopOUT();                   //Place l'état logique du port de sortie du pin 7 à 0. Eteint en meme temps le signal carré.
		void startClock();                //Démarre l'horloge et met à jour les parametres
		void stopClock();                 //Stop l'horloge
		void startWave();                 //Démarre le signal Carré
		void stopWave();                  //Stop le signal carré
		void switchMode();                //Inverse le mode 12/24 et prend en charge la conversion des heures.
		void switchMeridiem();            //Inverse la valeur de isPostMeridiem si le mode 12h est actif.
};

#endif
