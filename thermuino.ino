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

//LCD :
#include <LiquidCrystal.h> 

//Ethernet :
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetServer.h>

//Sonde 1Wire :
#include <OneWire.h>
#include "DS18B20.h"

//Horloge
#include <Wire.h>
#include "DallasTemperature.h"
#include "DS1307.h"

//Relay
#include "Relay.h"

//Pins
#define PIN_RELAY 1

#define PIN_BOUTON_PLUS 6
#define PIN_BOUTON_MOINS 7
#define PIN_BOUTON_MODE 8
#define PIN_LED_HEATING 9
#define PIN_LED_ALIMENTATION 10

#define PIN_ONE_WIRE 6
#define DS18B20_ADDRESS 0x28     // Adresse 1-Wire du DS18B20

#define LCD_RS A1
#define LCD_ENABLE A0
#define LCD_DATA4 5
#define LCD_DATA5 4
#define LCD_DATA6 3
#define LCD_DATA7 2



//Constant values
#define LIGNE1 0
#define LIGNE2 1
#define LIGNE3 0
#define LIGNE4 1

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_AUTO 2

#define MODE_FORCE_ON 3
#define MODE_FORCE_OFF 4

#define MAX_TEMPERATURE 30
#define MIN_TEMPERATURE 0

#define CELCIUS_CHAR_SERIAL 176
#define CELCIUS_CHAR_LCD 223 //The ° caracter

#define ETHERNET_REQUEST_MAX_SIZE 100

//Horlogerie
#define DS1307_ADDRESS 0x68


struct Date {
  uint8_t secondes;
  uint8_t minutes;
  uint8_t heures; // format 24h
  uint8_t jourDeLaSemaine; // 0~7 = lundi, mardi, ...
  uint8_t jour;
  uint8_t mois; 
  uint8_t annee; // format yy (ex 2012 -> 12)
};
typedef struct Date Date;

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };  
byte ip[] = {   192, 168, 1, 19};  
byte ip_arduino_server[] = { 192, 168, 0, 2};

LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_DATA4, LCD_DATA5, LCD_DATA6, LCD_DATA7);
EthernetServer server(80);
EthernetClient client;
OneWire oneWire(PIN_ONE_WIRE);
DS1307 ClockChip = DS1307();
DallasTemperature sensors(&oneWire);
Relay myRelay = Relay(PIN_RELAY);
Relay myHeatingLed = Relay(PIN_LED_HEATING);

float internTemperature = -1;
float progTemperature = 15;
float hysteresis = 1; //1°C Hysteresis
float externTemperature = 0;
int currentMode = 2;



byte bcd2dec(byte bcd) {
  return ((bcd / 16 * 10) + (bcd % 16)); 
}

byte dec2bcd(byte dec) {
  return ((dec / 10 * 16) + (dec % 10));
}

// Fonction configurant le DS1307 avec la date/heure fourni
void ecrireDate(struct Date *date) {
	Wire.beginTransmission(DS1307_ADDRESS); // Début de transaction I2C
Wire.write((byte) 0); // Arrête l'oscillateur du DS1307
  Wire.write(dec2bcd(date->secondes)); // Envoi des données
  Wire.write(dec2bcd(date->minutes));
  Wire.write(dec2bcd(date->heures));
  Wire.write(dec2bcd(date->jourDeLaSemaine));
  Wire.write(dec2bcd(date->jour));
  Wire.write(dec2bcd(date->mois));
  Wire.write(dec2bcd(date->annee));
  Wire.write((byte) 0); // Redémarre l'oscillateur du DS1307
  Wire.endTransmission(); // Fin de transaction I2C
}


// Fonction récupérant l'heure et la date courante à partir du DS1307
void lireDate(struct Date *date) {
  Wire.beginTransmission(DS1307_ADDRESS); // Début de transaction I2C
  Wire.write((byte) 0); // Demande les info à partir de l'adresse 0 (soit toutes les info)
  Wire.endTransmission(); // Fin de transaction I2C

  Wire.requestFrom(DS1307_ADDRESS, 7); // Récupère les info (7 octets = 7 valeurs correspondant à l'heure et à la date courante)

  date->secondes = bcd2dec(Wire.read()); // stockage et conversion des données reçu
  date->minutes = bcd2dec(Wire.read());
  date->heures = bcd2dec(Wire.read() & 0b111111);
  date->jourDeLaSemaine = bcd2dec(Wire.read());
  date->jour = bcd2dec(Wire.read());
  date->mois = bcd2dec(Wire.read());
  date->annee = bcd2dec(Wire.read());
}


void initEthernet() {
	lcd.setCursor(0, LIGNE1);
	lcd.print(("Connexion reseau..."));
	lcd.setCursor(0, LIGNE2);

	if(Ethernet.begin(mac) == 0) {
		Ethernet.begin(mac, ip);
		lcd.print(("IP Fixe : "));

	} else {
		lcd.print(("IP Dynamique : ")); 
	}
	
	lcd.setCursor(0, LIGNE3);
	lcd.print(Ethernet.localIP());

	lcd.setCursor(0, LIGNE4);
	lcd.print(("Connecte !"));
  
	delay(2000);

}


void initButtons() {
	pinMode(PIN_BOUTON_PLUS, INPUT);
	pinMode(PIN_BOUTON_MOINS, INPUT);
	pinMode(PIN_BOUTON_MODE, INPUT);
  
	pinMode(PIN_LED_HEATING, OUTPUT);
}

void initTemperature() {
	sensors.begin();
	
	DeviceAddress tempDeviceAddress;


	// Grab a count of devices on the wire
	int numberOfDevices = sensors.getDeviceCount();

	// locate devices on the bus
	Serial.print("Locating devices...");

	Serial.print("Found ");
	Serial.print(numberOfDevices);
	Serial.println(" devices.");

	// report parasite power requirements
	Serial.print("Parasite power is: "); 
	if (sensors.isParasitePowerMode()) Serial.println("ON");
	else Serial.println("OFF");

	// Loop through each device, print out address
	for(int i=0;i<numberOfDevices; i++) {
		// Search the wire for address
		if(sensors.getAddress(tempDeviceAddress, i)) {
			Serial.print("Found device ");
			Serial.print(i);
			Serial.print(" with address: ");
			
			for (uint8_t i = 0; i < 8; i++) {
				if (tempDeviceAddress[i] < 16) Serial.print("0");
				Serial.print(tempDeviceAddress[i], HEX);
			}
  
			Serial.println();

		}else{
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print(" but could not detect address. Check power and cabling");
		}
	}

}

void initHorloge() {
	ClockChip.setTimeRegister(2,50,0,6,26,11,11);
	ClockChip.startClock();  
}


void changeMode() {
	currentMode = (currentMode + 1) % 5;
}

void incrementeTemp() {
  progTemperature += 1.0;
  
  if(progTemperature > MAX_TEMPERATURE) {
    progTemperature = MAX_TEMPERATURE;
  }
}

void decrementeTemp() {
  progTemperature -= 1.0;
  
  if(progTemperature < MIN_TEMPERATURE) {
    progTemperature = MAX_TEMPERATURE;
  }
}

void updateButtons() {
  boolean modePin = digitalRead(PIN_BOUTON_MODE) == HIGH;
  boolean plusPin = digitalRead(PIN_BOUTON_PLUS) == HIGH;
  boolean moinsPin = digitalRead(PIN_BOUTON_MOINS) == HIGH;
  
  if(modePin == true) {
    changeMode();
  }
  
  if(plusPin == true && moinsPin == false) {
    incrementeTemp();
  }

  if(moinsPin == true && plusPin == false) {
    decrementeTemp();
  }

}

void printMode() {
	if(currentMode == MODE_OFF) {
		lcd.print(("Veille")); 
	} else if(currentMode == MODE_ON) {
		lcd.print(("Marche"));
	} else if(currentMode == MODE_FORCE_ON) {
		lcd.print(("Force"));
	} else if(currentMode == MODE_FORCE_OFF) {
		lcd.print(("Arret"));
	} else {
		lcd.print(("Auto")); 
	}
}


// Fonction vérifiant la présence d'une demande de synchronisation en provenance du pc et récupérant les données de synchronisation ou à défaut récupérant la date et l'heure courante 
void synchronisation(Date *date) {
  Serial.print("SYNC"); // Envoi de la commande de synchronisation
  int i;
  for(i = 0 ; (Serial.available() < 3) && (i < 6) ; i++) // Attente de 3s max 
    delay(500);
  if(i != 6) { // Il n'y as pas eu de timeout
    if(Serial.read() == 'A') 
      if(Serial.read() == 'C')
        if(Serial.read() == 'K') { // Si le pc à répondu par une commande ACK c'est que la synchronisation peut commencer
          while(Serial.available() < 7); // Attente des 7 octets de configuration
          date->secondes = Serial.read(); // Réception et conversion des données reçu
          date->minutes = Serial.read();
          date->heures = Serial.read();
          date->jourDeLaSemaine = Serial.read();
          date->jour = Serial.read();
          date->mois = Serial.read();
          date->annee = Serial.read();
          ecrireDate(date); // Stockage dans le DS1307 des donnes reçu
        }
  } 
  else 
    lireDate(date); // Si le pc n'as pas répondu à la demande de synchronisation la fonction ce content de lire les données courante du DS1307
}


void updatePrintDate(struct Date *date) {
  //clear(0, 0, 16); // Efface la 1er ligne de l'écran
  lcd.setCursor(0, LIGNE4); // Place le curseur à (0,0)
  
  switch(date->jourDeLaSemaine) { // Affiche sur 3 lettres le jour en cours
  case 1:
    lcd.print(("LU "));
    break;

  case 2:
    lcd.print(("MA "));
    break;

  case 3:
    lcd.print(("ME "));
    break;

  case 4:
    lcd.print(("JE "));                                                                    
    break;

  case 5:
    lcd.print(("VE "));
    break;

  case 6:
    lcd.print(("SA "));
    break;

  case 7:
    lcd.print(("DI "));
    break;
  }
  
  lcd.setCursor(3, LIGNE4); // Place le curseur à (0,4) (juste aprés les 3 lettres du jour + espace)
  lcd.print(date->jour / 10, DEC);// Affichage du jour sur deux caractéres
  lcd.setCursor(4, LIGNE4);
  lcd.print(date->jour % 10, DEC);
  lcd.setCursor(5, LIGNE4);
  lcd.print("/");
  lcd.setCursor(6, LIGNE4);
  lcd.print(date->mois / 10, DEC);// Affichage du mois sur deux caractéres
  lcd.setCursor(7, LIGNE4);
  lcd.print(date->mois % 10, DEC);
  lcd.setCursor(8, LIGNE4);
  lcd.print("/");
  lcd.setCursor(9, LIGNE4);
  lcd.print(date->annee / 10, DEC);// Affichage de l'année sur deux caractéres
  lcd.setCursor(10, LIGNE4);
  lcd.print(date->annee % 10, DEC);
  
  lcd.setCursor(12, LIGNE4); // Place le curseur en début de la 2eme ligne
  lcd.print(date->heures / 10, DEC); // Affichage de l'heure sur deux caractéres
  lcd.setCursor(13, LIGNE4);
  lcd.print(date->heures % 10, DEC);
  lcd.setCursor(14, LIGNE4);
  lcd.print(":");
  lcd.setCursor(15, LIGNE4);
  lcd.print(date->minutes / 10, DEC); // Affichage des minutes sur deux caractéres
  lcd.setCursor(16, LIGNE4);
  lcd.print(date->minutes % 10, DEC);
  lcd.setCursor(17, LIGNE4);
  lcd.print(":");
  lcd.setCursor(18, LIGNE4);
  lcd.print(date->secondes / 10, DEC); // Affichage des secondes sur deux caractéres
  lcd.setCursor(19, LIGNE4);
  lcd.print(date->secondes % 10, DEC);

}

void updateScreen() {
  lcd.setCursor(0, LIGNE1);
  lcd.print(("Maison: "));
  printTemperature(internTemperature);

  lcd.setCursor(0, LIGNE2);
  lcd.print(("Prog: "));
  printTemperature(progTemperature);

  lcd.setCursor(0, LIGNE3);  
  lcd.print(("Mode: "));
  printMode();
  
  lcd.setCursor(0, LIGNE4);  
  lcd.print(("H: "));
  Date date;
  lireDate(&date);
//  updatePrintDate();
}


void updateTemperature() {
	sensors.requestTemperatures(); // Send the command to get temperatures
	internTemperature = sensors.getTempCByIndex(0);
	
}
	
void printTemperature(float valeur) {
	lcd.print(valeur);
	lcd.write(CELCIUS_CHAR_LCD); //Caractere Celcius
	lcd.write("C");
}



void updateExternData() {
/*	if (client.connect(ip_arduino_server, 80)) {
		Serial.println("connected");
		client.println("GET /temp HTTP/1.0");
		client.println();
	} else {
		Serial.println("connection failed");
	}*/
	
}

void updateRelay() {
	float tempreference = -1;
  
	if(currentMode == MODE_ON) {
		tempreference = progTemperature;
    
	} else if (currentMode == MODE_OFF) {
		tempreference = progTemperature - 5;
	
	} else if(currentMode == MODE_AUTO) {
		//Régulation du truc
	}
 
	if (currentMode == MODE_FORCE_OFF) {
		myRelay.setOff();
		myHeatingLed.setOff();
                
	} else if (currentMode == MODE_FORCE_ON) {
		myRelay.setOn();
		myHeatingLed.setOn();
	} else { //Auto-regulation
      
		if(myRelay.isStateOFF() && ((tempreference - hysteresis) > internTemperature)) {
			myRelay.setOn();
			myHeatingLed.setOn();
		} else if(!myRelay.isStateOFF() && ((tempreference + hysteresis) < internTemperature)) {
			myRelay.setOff();
			myHeatingLed.setOff();
		}
		
	}
}


void printEthernetPage(EthernetClient client, String requete) {
			
	if(requete.indexOf(String("MODE=OFF")) > 0) {
		currentMode = MODE_OFF;
	} else if(requete.indexOf(String("MODE=ON")) > 0) {
		currentMode = MODE_ON;
	} else if(requete.indexOf(String("MODE=AUTO")) > 0) {
		currentMode = MODE_AUTO;
	} else if(requete.indexOf(String("MODE=FORCE_ON")) > 0) {
		currentMode = MODE_FORCE_ON;
	} else if(requete.indexOf(String("MODE=FORCE_OFF")) > 0) {
		currentMode = MODE_FORCE_OFF;
	} 
			
	client.print(("INTERN_TEMPERATURE="));
	client.print(internTemperature);
	client.write(CELCIUS_CHAR_SERIAL);
	client.println("C");
	
	client.print(("PROG_TEMPERATURE="));
	client.print(progTemperature);
	client.write(CELCIUS_CHAR_SERIAL);
	client.println("C");

	client.print(("MODE="));

	switch(currentMode) {
		case MODE_FORCE_OFF:
		client.println("FORCE_OFF");
		break;
		
		case MODE_FORCE_ON:
		client.println("FORCE_ON");
		break;

		case MODE_ON:
		client.println("ON");
		break;

		case MODE_OFF:
		client.println("FORCE_ON");
		break;

		default:
		client.println("AUTO");
		break;

	}
	
	client.print("HEAT=");
	if(myRelay.isStateOFF()) {
		client.println("OFF");
	} else {
		client.println("ON");
	}
}

void setup() {
	Serial.begin(9600);
	lcd.begin(16,2);

	lcd.print(("Chargement..."));

	initEthernet();	
	initButtons();
	initHorloge();
	initTemperature();
}

void loop() {
	updateTemperature();
	updateButtons();
	updateExternData();
	updateScreen();
	updateRelay();


	client = server.available();
	if (client) {
		
		String requete = "";
		int charCounter = 0;
		
		while (client.connected()) {

			while (client.available()) { //while there is any byte to read

				char c = client.read(); //read the next byte/char
				charCounter = charCounter+1; //count the number of received chars

				//Only the n first chars are saved for the request
				if (charCounter <= ETHERNET_REQUEST_MAX_SIZE) {
					requete = requete + c;
				} else {
					client.flush();
				}
	
			} 

			Serial.println(requete);
			int pos = -1;
			
			if((pos = requete.indexOf(String("TEMP=")))   > 0) {
				int posEnd = requete.indexOf(String("C"), pos);
					  
				if(posEnd > 0) {
					String sub = requete.substring(pos + 5, posEnd);
					//5correspond à la taille de TEMP=
					lcd.print(sub);

					char charBuf[6];
					sub.toCharArray(charBuf, 6);
					progTemperature = atof(charBuf);
				}
			}


		
			client.println(("HTTP/1.1 200 OK"));
			client.println(("Content-Type: text/plain"));
			client.println(("Connnection: close"));
			client.println();
		
			//print the page
			printEthernetPage(client, requete);
			
			// give the web browser time to receive the data
			delay(1);
			
			// close the connection:
			client.stop();
			
			//Serial.println("Client Disconnected");
				
			client.stop();
		}



	}
	
	//Delay for buttons, updates, etc.
	delay(500);


}

