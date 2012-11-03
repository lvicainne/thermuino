/** Thermuino, a control for heat with Arduino
Copyright (C) 2012 Louis VICAINNE <louis.vicainne@gmail.com>
http://www.vicainne.fr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//LCD :
#include <LiquidCrystal.h> 

//Ethernet :
#include <SPI.h>
//#include <Dhcp.h>
#include <Ethernet.h>
#include <EthernetServer.h>

//Sonde 1Wire :
#include <OneWire.h>

//Horloge
#include <Wire.h>

//Pins
#define PIN_RELAY 1

#define PIN_BOUTON_PLUS 1
#define PIN_BOUTON_MOINS 2
#define PIN_BOUTON_MODE 3
#define PIN_LED_CHAUFFE 4
#define PIN_LED_ALIMENTATION 5

#define PIN_ONE_WIRE 6
#define DS18B20 0x28     // Adresse 1-Wire du DS18B20

#define LCD_RS 12
#define LCD_ENABLE 11
#define LCD_DATA4 5
#define LCD_DATA5 4
#define LCD_DATA6 3
#define LCD_DATA7 2





//Constant values
#define LIGNE1 0
#define LIGNE2 1
#define LIGNE3 2
#define LIGNE4 3

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_AUTO 2

#define MAX_TEMPERATURE 30
#define MIN_TEMPERATURE 0

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


LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_DATA4, LCD_DATA5, LCD_DATA6, LCD_DATA7);
EthernetServer server(80);
OneWire ds(PIN_ONE_WIRE);

float internTemperature = -1;
float progTemperature = 15;
float externTemperature = 0;
int currentMode = 0;



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
  lcd.print(PSTR("Connexion au réseau..."));
  lcd.setCursor(0, 1);

  if(Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip);
    lcd.print(PSTR("IP Fixe : "));

  } 
  else {
    lcd.print(PSTR("IP Dynamique : ")); 
  }

  lcd.print(Ethernet.localIP());   


  lcd.print(PSTR("Connecté !"));
}


void initButtons() {
  pinMode(PIN_BOUTON_PLUS, INPUT);
  pinMode(PIN_BOUTON_MOINS, INPUT);
  pinMode(PIN_BOUTON_MODE, INPUT);
  
  pinMode(PIN_LED_CHAUFFE, OUTPUT);
}

void initTemperature() {
  
  
}

void initHorloge() {
  Wire.begin(); //horloge
  
  
}


void changeMode() {
  currentMode = (currentMode + 1) % 3;
  
}

void incrementeTemp() {
  progTemperature += 1;
  
  if(progTemperature > MAX_TEMPERATURE) {
    progTemperature = MAX_TEMPERATURE;
  }
}

void decrementeTemp() {
  progTemperature -= 1;
  
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
  lcd.print(PSTR("Veille")); 
 } else if(currentMode == MODE_OFF) {
    lcd.print(PSTR("Marche"));
  } else {
   lcd.print(PSTR("Auto")); 
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
    lcd.print(PSTR("LU "));
    break;

  case 2:
    lcd.print(PSTR("MA "));
    break;

  case 3:
    lcd.print(PSTR("ME "));
    break;

  case 4:
    lcd.print(PSTR("JE "));                                                                    
    break;

  case 5:
    lcd.print(PSTR("VE "));
    break;

  case 6:
    lcd.print(PSTR("SA "));
    break;

  case 7:
    lcd.print(PSTR("DI "));
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
  lcd.print(PSTR("Maison: "));
  printTemperature(internTemperature);

  lcd.setCursor(0, LIGNE2);
  lcd.print(PSTR("Prog: "));
  printTemperature(progTemperature);

  lcd.setCursor(0, LIGNE3);  
  lcd.print(PSTR("Mode: "));
  printMode();
  
  lcd.setCursor(0, LIGNE4);  
  lcd.print(PSTR("Horlogerie: "));
  Date date;
  lireDate(&date);
  lcd.print((String) date.jour + ":" + (String) date.mois + ":" + (String) date.annee + " ");
  lcd.print((String) date.heures + ":" + (String) date.minutes + ":" + (String) date.secondes);
}


void updateTemperature() {
  getTemperature(&internTemperature);
}


boolean getTemperature(float *temp){
  byte data[9], addr[8];
  // data : Données lues depuis le scratchpad
  // addr : adresse du module 1-Wire détecté

  if (!ds.search(addr)) { // Recherche un module 1-Wire
    ds.reset_search();    // Réinitialise la recherche de module
    return false;         // Retourne une erreur
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) // Vérifie que l'adresse a été correctement reçue
    return false;                        // Si le message est corrompu on retourne une erreur

  if (addr[0] != DS18B20) // Vérifie qu'il s'agit bien d'un DS18B20
    return false;         // Si ce n'est pas le cas on retourne une erreur

  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20
  
  ds.write(0x44, 1);      // On lance une prise de mesure de température
  delay(800);             // Et on attend la fin de la mesure
  
  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20
  ds.write(0xBE);         // On envoie une demande de lecture du scratchpad

  for (byte i = 0; i < 9; i++) // On lit le scratchpad
    data[i] = ds.read();       // Et on stock les octets reçus
  
  // Calcul de la température en degré Celsius
  *temp = ((data[1] << 8) | data[0]) * 0.0625; 
  
  // Pas d'erreur
  return true;
}

void printTemperature(float valeur) {
  lcd.print(valeur);
  lcd.write(176); //Caractere Celcius
  lcd.write("C");
}

void setup() {
  lcd.begin(16,2);
  lcd.print(PSTR("Chargement..."));

  initEthernet();
  initButtons();
  initHorloge();
  initTemperature();
  
  
}

void loop() {
    String requete="";
    
    updateTemperature();
    updateButtons();
    updateScreen();

    EthernetClient client = server.available();
    if (client) {
      //Serial.println("New Client");
      int charCounter = 0;

      while (client.connected()) {

        while (client.available()) { // tant que des octets sont disponibles en lecture

            char c = client.read(); // lit l'octet suivant reçu du client (pour vider le buffer au fur à mesure !)
          charCounter = charCounter+1; // incrémente le compteur de caractère reçus

          //--- on ne mémorise que les n premiers caractères de la requete reçue
          if (charCounter <= 100) {
            requete = requete + c; // ajoute le caractère reçu au String pour les N premiers caractères 
          } 
          else {
            client.flush();
          }
          //Serial.print(c); // message debug - affiche la requete entiere reçue

        } // --- fin while client.available

        client.println(PSTR("HTTP/1.1 200 OK"));
        client.println(PSTR("Content-Type: text/plain"));
        client.println(PSTR("Connnection: close"));
        client.println();


        client.print(PSTR("INTERN_TEMPERATURE="));
        client.print(internTemperature);
        client.write(176);
        client.println("C");
        
        client.print(PSTR("PROG_TEMPERATURE="));
        client.print(progTemperature);
        client.write(176);
        client.println("C");
       
        client.print(PSTR("MODE="));
        client.println(currentMode);
        

        client.stop();
      }

      // give the web browser time to receive the data
      delay(1);
      // close the connection:
      client.stop();
      //Serial.println("Client Disconnected");
    }
    
    //Petit retard pour les boutons, l'actualisation, etc...
    delay(500);


  }

