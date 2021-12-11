#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <EEPROM.h>
/*

lisää säädettävä mosfet lähtö mahdolliselle valon kirkkuden säädölle 
lisää potikka mahdolliselle kirkkauden säädölle 
lisää inputteja jos haluat lisätä antureita lautaan 
luultavasti koodi on rikki XDD, osittain vieläkin
sivuvalikot ovat sekaisin... tallennus EEPROMILLE toimii... seuraavalla kerralla säädä kaikki sivu-valikot kuntoon.
*/

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7); // 0x27 is the I2C bus address for an unmodified module

//---------------------------------------Counter to change positions of pages---------------------------------------//
byte page_counter = 1;     //To move beetwen pages
byte numbOfPages = 4;      // valikossa näkyvien sivujen maksimimäärä, todellisuudessa sivuja enemmän.
byte subpage1_counter = 0; //To move submenu 1 rele aikojen säätö
byte subpage2_counter = 0; //To move submenu 2 auto on
byte subpage3_counter = 0; //To move submenu 3 auto off
byte subpage4_counter = 0; //To move submenu ajansäätö
byte subpage5_counter = 0; //To move submenu pumpun käyttö auto

byte pumpAutoMinute = 0;
byte pumpAutoSeconds = 0;

//--------------------------------------------------Manuaaliset kytkennät---------------------------------------------//
/*
boolean pump = LOW;         //StateArray[0]
boolean lights = LOW;       //StateArray[1]
boolean air = LOW;          //StateArray[2]
boolean lightsAuto = LOW;   //StateArray[3]
boolean airAuto = LOW;      //StateArray[4]
boolean pumpAuto = LOW;     //StateArray[5]
boolean pumpAutoON = LOW;   //StateArray[6]
boolean lightsAutoON = LOW; //StateArray[7]
boolean airAutoON = LOW;    //StateArray[8]
*/
const int numOfStates = 9;
boolean stateArray[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW}; //

boolean prevPump = LOW;
boolean prevLights = LOW;
boolean prevAir = LOW;
boolean prevLightsAuto = LOW;
boolean prevAirAuto = LOW;
boolean prevPumpAuto = LOW;
boolean prevPumpAutoON = LOW;
boolean prevLightsAutoON = LOW;
boolean prevAirAutoON = LOW;

boolean prevStateArray[numOfStates] = {
    prevPump, prevLights, prevAir, prevLightsAuto, prevAirAuto,
    prevPumpAuto, prevPumpAutoON, prevLightsAutoON, prevAirAutoON};

//--------------------------------------------------Releitten aikojen muuttamiset---------------------------------------------//

uint8_t time1Hours = 0, time1Minutes = 0;   // 0 1
uint8_t time2Hours = 0, time2Minutes = 0;   // 2 3
uint8_t time3Hours = 0, time3Minutes = 0;   // 4 5
uint8_t time4Hours = 0, time4Minutes = 0;   // 6 7
uint8_t time5Hours = 0, time5Minutes = 0;   // 8 9
uint8_t time6Hours = 0, time6Minutes = 0;   // 10 11
uint8_t time7Hours = 0, time7Minutes = 0;   // 12 13
uint8_t time8Minutes = 0, time8Seconds = 0; // 14 15

const int numOfVar = 16; // number of time variables

uint8_t timeArray[] = { // time array for saving to EEPROM
    time1Hours, time1Minutes, time2Hours, time2Minutes,
    time3Hours, time3Minutes, time4Hours, time4Minutes,
    time5Hours, time5Minutes, time6Hours, time6Minutes,
    time7Hours, time7Minutes, time8Minutes, time8Seconds};

uint8_t prevtime1Hours = 0, prevtime1Minutes = 0; // Holds the previous setup time
uint8_t prevtime2Hours = 0, prevtime2Minutes = 0; // Holds the previous setup time
uint8_t prevtime3Hours = 0, prevtime3Minutes = 0; // Holds the previous setup time
uint8_t prevtime4Hours = 0, prevtime4Minutes = 0; // Holds the previous setup time
uint8_t prevtime5Hours = 0, prevtime5Minutes = 0; // Holds the previous setup time
uint8_t prevtime6Hours = 0, prevtime6Minutes = 0; // Holds the previous setup time
uint8_t prevtime7Hours = 0, prevtime7Minutes = 0; // Holds the previous setup time
uint8_t prevtime8Minutes = 0, prevtime8Seconds = 0;

uint8_t prevTimeArray[numOfVar] = { // Array for checking previous times
    prevtime1Hours, prevtime1Minutes, prevtime2Hours, prevtime2Minutes,
    prevtime3Hours, prevtime3Minutes, prevtime4Hours, prevtime4Minutes,
    prevtime5Hours, prevtime5Minutes, prevtime6Hours, prevtime6Minutes,
    prevtime7Hours, prevtime7Minutes, prevtime8Minutes, prevtime8Seconds};

// jos näistä sais tehtyä arrayn niin koodi olisi paljon helpompi toteuttaa.
//------------------------------------------------Changing time variables------------------------------------------------//

byte hourupg;
byte minupg;
int yearupg;
byte monthupg;
byte dayupg;
byte menu = 0;

//------------------------------------------------------Variables for auto off-----------------------------------------------------//

unsigned long previousMillis = 0; //start time milliseconds
unsigned long interval = 60000;   //Desired wait time 60 seconds (as millis)

//----------------------------------------------Pins--------------------------------------------//

#define up 3      //Up button
#define sel 4     //Sel button
#define down 6    //Down button
#define esc 5     //Esc button
#define relay1 10 //relay1
#define relay2 11 //relay2
#define relay3 12 //relay3
#define led 13

const int numOfInputs = 4;            //number off inputs
const int inputPins[] = {3, 4, 5, 6}; //array for input pins
byte numOfOutputs = 4;                //number of outputs
byte outputPins[] = {10, 11, 12, 13}; //array for output pins
//------------------------------------------------Storage debounce function--------------------------------------------//

boolean current_up = HIGH;   //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean last_up = HIGH;      //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean last_down = HIGH;    //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean current_down = HIGH; //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean current_sel = HIGH;  //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean last_sel = HIGH;     //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean current_esc = HIGH;  //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)
boolean last_esc = HIGH;     //muuta nämä LOW tilaan kun saa uudet napit(kytketään +5V)

//---------------------------------------------------Kuviot--------------------------------------------//

//-------Custom return char (return kuvake)-------//

byte back[8] = {
    0b00100,
    0b01000,
    0b11111,
    0b01001,
    0b00101,
    0b00001,
    0b00001,
    0b11111};

//--------Custom arrow char (nuoli)---------//

byte arrow[8] = {
    0b01000,
    0b00100,
    0b00010,
    0b11111,
    0b00010,
    0b00100,
    0b01000,
    0b00000};
//------------------------------------------------ReadFromEEPROM-------------------------------------//
void ReadFromEEPROM()
{
  Serial.println("reading previous time variables from EEPROM");
  for (uint8_t i = 0; i < numOfVar; i++) //recieving previous times from EEPROM addresses 0-15
  {
    timeArray[i] = EEPROM.read(i);
    prevTimeArray[i] = timeArray[i]; //saving timeArray to previous timeArray
  }

  for (int i = 0; i < numOfVar; i++)
  {
    Serial.print("variable: ");
    Serial.println(timeArray[i]);
  }

  Serial.println("reading previous states from EEPROM");

  for (uint8_t i = 0; i < numOfStates; i++) //recieving previous times from EEPROM addresses 16-25
  {

    stateArray[i] = EEPROM.read(i + numOfVar);
    prevStateArray[i] = stateArray[i]; //saving statearray to previous state array
  }

  for (int i = 0; i < numOfStates; i++)
  {
    Serial.print("variable: ");
    Serial.println(stateArray[i]);
  }
}
//------------------------------------------------Save to EEPROM-------------------------------------//
void SaveToEEPROM()
{

  for (int i = 0; i < numOfVar; i++) //Saves to EEPROM adresses 0-15
  {
    if (timeArray[i] != prevTimeArray[i])
    {
      EEPROM.write(i, timeArray[i]);
      prevTimeArray[i] = timeArray[i];
      Serial.print("writing times to EEPROM addr: ");
      Serial.println(i);
    }
  }
  for (int i = 0; i < numOfStates; i++) //Saves to EEPROM adresses 16-25
  {
    if (stateArray[i] != prevStateArray[i])
    {
      EEPROM.write(i + numOfVar, stateArray[i]);
      prevStateArray[i] = stateArray[i];
      Serial.print(" writing states to EEPROM addr: ");
      Serial.println(i + numOfVar);
    }
  }
}

//----------------------------------------------------RELAYS---------------------------------------------------//
void relays()
{
  DateTime now = rtc.now();

  //-----------relay1 VALOT-----------//

  if (now.hour() == timeArray[0] && now.minute() == timeArray[1] && now.second() >= 0 && now.second() <= 1)
  {
    // DateTime now = rtc.now();
    digitalWrite(relay1, HIGH); //valot päälle kellonajan mukaan
    digitalWrite(stateArray[7], HIGH);
    digitalWrite(led, HIGH); //led päälle
    Serial.print("valot päällä      ");
    delay(10);
  }

  if (now.hour() == timeArray[2] && now.minute() == timeArray[3] && now.second() >= 0 && now.second() <= 1)
  {
    //DateTime now = rtc.now();

    digitalWrite(relay1, LOW); //valot pois kellonajan mukaan
    digitalWrite(stateArray[7], LOW);
    digitalWrite(led, LOW); //led päälle
    Serial.print("valot pois      ");
    delay(10);
  }

  if (stateArray[1] == HIGH && stateArray[3] == LOW)
  {
    digitalWrite(relay1, HIGH); //VALOT PÄÄLLE
  }
  if (stateArray[1] == LOW)
  {
    if (stateArray[3] == LOW)
    {

      digitalWrite(relay1, LOW); //valot pois
    }
  }

  //-------relay2 BUBBLER-------//
  if (now.hour() == timeArray[4] && now.minute() == timeArray[5] && now.second() >= 0 && now.second() <= 1)
  {
    // DateTime now = rtc.now();
    digitalWrite(relay2, HIGH); //bubbler päälle kellonajan mukaan
    //digitalWrite(stateArray[8], HIGH);                                            // tämä tallentuu joka kerta EEPROMILLE
    Serial.print("ilma päällä      ");
    delay(10);
  }

  if (now.hour() == timeArray[6] && now.minute() == timeArray[7] && now.second() >= 0 && now.second() <= 1)
  {
    //DateTime now = rtc.now();
    digitalWrite(relay2, LOW); //bubbler pois kellonajan mukaan
    //digitalWrite(stateArray[8], LOW);                                           // tämä tallentuu joka kerta EEPROMILLE
    Serial.print("ilma pois      ");
    delay(10);
  }
  if (stateArray[2] == HIGH && stateArray[4] == LOW)
  {
    digitalWrite(relay2, HIGH); //bubbler PÄÄLLE
  }
  if (stateArray[2] == LOW)
  {
    if (stateArray[4] == LOW)
    {

      digitalWrite(relay2, LOW); //bubbler pois
    }
  }

  //-------relay3  PUMPPU--------//

  if (stateArray[5] == HIGH)
  {
    if ((now.hour() == (timeArray[8])) || (now.hour() == (timeArray[10])) || (now.hour() == (timeArray[12])))
    {

      if ((now.minute() == (timeArray[9])) || (now.minute() == (timeArray[11])) || (now.minute() == (timeArray[13])))
      {

        if ((now.second() >= 0) && (now.second() <= 1))
        {

          DateTime now = rtc.now();
          digitalWrite(relay3, HIGH); //PUMPPU PÄÄLLE
          pumpAutoMinute = now.minute();
          pumpAutoSeconds = now.second();
          stateArray[6] = HIGH; // tämä tallentuu joka kerta EEPROMILLE
          Serial.println("pumppu päällä      ");
          delay(10);
        }
      }
    }
  }

  if (stateArray[0] == HIGH && stateArray[5] == LOW)
  {
    digitalWrite(relay3, HIGH); //PUMPPU PÄÄLLE
  }
  if (stateArray[0] == LOW)
  {
    if (stateArray[5] == LOW)
    {
      digitalWrite(relay3, LOW); //pumppu pois päältä
    }
  }

  if ((stateArray[6] == HIGH) && (now.minute() >= (pumpAutoMinute + timeArray[14])))
  {

    if (now.second() >= (pumpAutoSeconds + timeArray[15]))
    {
      pumpAutoMinute = 0;
      pumpAutoSeconds = 0;
      stateArray[6] = LOW;
      digitalWrite(relay3, LOW); //pumppu pois
      Serial.println("pumppu pois      ");
      delay(10);
    }
  }
}
//------------------------------------------------SETTING TIME----------------------------------------------------------//
void settingTime(uint8_t HOURS, uint8_t MINS) // int HOURS ON timeArray[HOURS], int MINS ON timeArray[MINS]
{
  if (last_up == HIGH && current_up == LOW)
  {
    Serial.print("time");
    Serial.print(HOURS);
    Serial.print("hours:");

    Serial.println(timeArray[HOURS]);

    Serial.print("time");
    Serial.print(MINS);
    Serial.print("mins:");
    Serial.println(timeArray[MINS]);

    if (timeArray[MINS] >= 30)
    {
      timeArray[MINS] = 0;

      if (timeArray[HOURS] >= 23)
      {
        timeArray[HOURS] = 0;
      }
      else
      {
        timeArray[HOURS] = timeArray[HOURS] + 1;
      }
    }
    else
    {
      timeArray[MINS] = timeArray[MINS] + 30;
    }
  }
  last_up = current_up;

  if (last_down == HIGH && current_down == LOW)
  {
    Serial.println(HOURS);
    Serial.println(MINS);

    Serial.println(timeArray[HOURS]);
    Serial.println(timeArray[MINS]);
    if (timeArray[MINS] == 0)
    {
      timeArray[MINS] = 30;
      if (timeArray[HOURS] == 0)
      {
        timeArray[HOURS] = 23;
      }
      else
      {
        timeArray[HOURS] = timeArray[HOURS] - 1;
      }
    }
    else
    {
      timeArray[MINS] = timeArray[MINS] - 30;
    }
  }
  last_down = current_down;
}
//-----------------------------------------De-bouncing function for all buttons-------------------------------------------------//

boolean debounce(boolean last, int pin)
{
  boolean current = digitalRead(pin);
  if (!last != current)
  {
    delay(5);
    current = digitalRead(pin);
  }
  return current;
}

void buttons()
{
  //------------------------------------------kytkimien debouncet-----------------------------------------------//

  current_up = debounce(last_up, up);       //Debounce for Up button
  current_sel = debounce(last_sel, sel);    //Debounce for Select  button
  current_down = debounce(last_down, down); //Debounce for Down button
  current_esc = debounce(last_esc, esc);    //Debounce for Esc button

  //----------------------------------------------Page counter function to move pages----------------------------------------------//

  if (subpage1_counter == 0 && subpage2_counter == 0 && subpage3_counter == 0 && subpage4_counter == 0 && subpage5_counter == 0)
  { // up/down buttons enabled if subcounters are 0,Disabled if 1,2..etc to work on submenus

    //------------------------------------------Page Up---------------------------------------------------//

    if (last_up == HIGH && current_up == LOW)
    { //When up button is pressed ((LOW ja HIGH käännetään toisin päin uusilla napeilla

      lcd.clear(); //When page is changed, lcd clear to print new page
      if (page_counter < numbOfPages)
      {                                  //Page counter never higher than numbOfPages(total of pages)
        page_counter = page_counter + 1; //Page up
      }
      else
      {
        page_counter = 1; // Moves Back to page 1
      }
    }

    last_up = current_up;

    //------------------------------------------Page Down-------------------------------------------------//

    if (last_down == HIGH && current_down == LOW)
    { //When down button is pressed

      lcd.clear(); //When page is changed, lcd clear to print new page
      if (page_counter > 1)
      {                                  //Page counter never lower than 3 (total of pages)
        page_counter = page_counter - 1; //Page down
      }
      else
      {
        page_counter = numbOfPages; //Moves back to last page
      }
    }

    last_down = current_down;

    //---------------------------------------Back to homepage (ESC pressed)---------------------------------------//

    if (last_esc == HIGH && current_esc == LOW)
    { //When esc button is pressed ((LOW ja HIGH käännetään toisin päin uusilla napeilla

      lcd.clear(); //When page is changed, lcd clear to print new page

      page_counter = 1;
    }

    last_esc = current_esc;
    //SaveToEEPROM();
  }
}

//-----------------------------------------------RELAY SET----------------------------------------------------------//
/*
int relaySet(int as,int ds, int sf){
    as++;
    ds++;
    sf++;
    return(as,ds,sf);
  }
 */
//------------------------------------------------VOID MAINPAGE 1--------------------------------------------------//
void mainPage1()
{
  //Design of home page 1
  /*
  tällä säästää monta riviä koodia, muuta kun kerkeät ja pääset testaamaan että toimii..

    lcd.setCursor(0,0); //Start at character 0 on line 0
    lcd.print("TIME :");
    lcd.setCursor(6,0); 
    print2digits(tm.Hour);
    lcd.print(':');
    print2digits(tm.Minute);
    lcd.print(':');
    print2digits(tm.Second);

  void print2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.print('0');
  }
  lcd.print(number);
}
  */
  DateTime now = rtc.now();

  lcd.setCursor(1, 1);
  lcd.print("Aika : "); //aika tunnit

  if (now.hour() <= 9)
  {
    lcd.print("0");
  }
  lcd.print(now.hour(), DEC); //aika tunnit
  lcd.print(":");
  if (now.minute() <= 9) //aika minuutit
  {
    lcd.print("0");
  }
  lcd.print(now.minute(), DEC); //aika minuutit
  lcd.print(":");
  if (now.second() <= 9) //aika sekunnit
  {
    lcd.print("0");
  }
  lcd.print(now.second(), DEC); //aika sekunnit

  lcd.setCursor(1, 0);
  lcd.print("Pvm  : "); //päivämäärä
  if (now.day() <= 9)
  {
    lcd.print("0");
  }
  lcd.print(now.day(), DEC); //päivämäärä päivä
  lcd.print("/");
  if (now.month() <= 9)
  {
    lcd.print("0");
  }
  lcd.print(now.month(), DEC); //päivämäärä kk
  lcd.print("/");
  lcd.print(now.year(), DEC); //päivämäärä vuosi

  //----------------SIVU1-----------------------------käyttöjen indikointi SIVU 1--------------------------------------------------//

  lcd.setCursor(1, 2);
  lcd.print("valo");
  lcd.setCursor(7, 2);
  lcd.print("ilma");
  lcd.setCursor(13, 2);
  lcd.print("pumppu");

  lcd.setCursor(2, 3); //valojen tilatieto asetus ON
  if (stateArray[1] == HIGH)
  {
    lcd.print("    ");
    lcd.setCursor(2, 3);
    lcd.print("ON");
  }

  if (stateArray[3] == HIGH)
  { //valojen tilatieto asetus AUTO
    lcd.setCursor(2, 3);
    lcd.print("    ");
    lcd.setCursor(2, 3);
    lcd.print("AUTO");
  }
  /*
if(stateArray[1]==lightsAuto) voiko molemmat olla yhtäaikaa high? 
*/
  if (stateArray[1] == LOW && stateArray[3] == LOW)
  { //valojen tilatieto asetus OFF
    lcd.setCursor(2, 3);
    lcd.print("    ");
    lcd.setCursor(2, 3);
    lcd.print("OFF");
  }

  lcd.setCursor(8, 3); //ilmapumpun tilatieto asetus ON
  if (stateArray[2] == HIGH)
  {
    lcd.print("    ");
    lcd.setCursor(8, 3);
    lcd.print("ON");
  }

  if (stateArray[4] == HIGH)
  { //ilmapumpun tilatieto asetus AUTO
    lcd.setCursor(8, 3);
    lcd.print("    ");
    lcd.setCursor(8, 3);
    lcd.print("AUTO");
  }

  if (stateArray[2] == LOW && stateArray[4] == LOW)
  { //ilmapumpun tilatieto asetus OFF
    lcd.setCursor(8, 3);
    lcd.print("    ");
    lcd.setCursor(8, 3);
    lcd.print("OFF");
  }

  lcd.setCursor(15, 3); //vesipumpun tilatieto asetus ON
  if (stateArray[0] == HIGH)
  {
    lcd.print("    ");
    lcd.setCursor(15, 3);
    lcd.print("ON");
  }

  if (stateArray[5] == HIGH)
  { //vesipumpun tilatieto asetus AUTO
    lcd.setCursor(15, 3);
    lcd.print("    ");
    lcd.setCursor(15, 3);
    lcd.print("AUTO");
  }

  if (stateArray[0] == LOW && stateArray[5] == LOW)
  { //vesipumpun tilatieto asetus OFF
    lcd.setCursor(15, 3);
    lcd.print("    ");
    lcd.setCursor(15, 3);
    lcd.print("OFF");
  }

  //---------------SIVU1-----------------------------------------Subpage setit pääsivulle SIVU 1----------------------------------------------------//

  if (last_sel == HIGH && current_sel == LOW)
  { //select button pressed
    if (subpage1_counter < 3)
    {                     // subpage counter never higher than 3 (total of items)
      subpage1_counter++; //subcounter to move beetwen submenu
    }
    else
    { //If subpage higher than 1 (total of items) return to first item
      subpage1_counter = 1;
    }
  }
  last_sel = current_sel; //Save last state of select button
  //---------------SIVU1------------------------------------KÄYTTÖJEN SÄÄTÖ SIVU subpage 1 ------------------------------------------//
  switch (subpage1_counter)
  {
  case 1:
    lcd.setCursor(12, 2); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(0, 2);  //Place the arrow
    lcd.write(byte(2));   //Place the arrow

    //--------------SIVU1---------------------------------------------Setting time---------------------------------------------------------//

    if (last_up == HIGH && current_up == LOW)
    {
      stateArray[1] = HIGH; //Valot päälle
      stateArray[3] = LOW;  //Automaatti POIS
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[1] = LOW;
      stateArray[3] = LOW; //Automaatti POIS
    }
    last_down = current_down;

    break;
  case 2:

    lcd.setCursor(0, 2); //remove last arrow
    lcd.print(" ");      //remove last arrow
    lcd.setCursor(6, 2); //Place the arrow
    lcd.write(byte(2));

    //------------SIVU1----------------------Setting time---------------------------//

    if (last_up == HIGH && current_up == LOW)
    {
      stateArray[2] = HIGH; //air päälle
      stateArray[4] = LOW;  //Automaatti POIS
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[2] = LOW;
      stateArray[4] = LOW; //Automaatti POIS
    }
    last_down = current_down;

    break;
  case 3:

    lcd.setCursor(6, 2);
    lcd.print(" ");       //Delete last arrow position
    lcd.setCursor(12, 2); //Place the arrow
    lcd.write(byte(2));

    //--------------SIVU1----------------Setting time---------------------------//

    if (last_up == HIGH && current_up == LOW)
    {

      stateArray[0] = HIGH; //stateArray[0]pu päälle
      stateArray[5] = LOW;  //Automaatti POIS
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[0] = LOW;
      stateArray[5] = LOW; //Automaatti POIS
    }
    last_down = current_down;

    break;
  }

  //-------------SIVU1------------------KÄYTTÖJEN SÄÄTÖ SIVU 1 ----------------------------------------------//

  //AIKA 2 SÄÄTÖ LOPPUU

  //--------------SIVU1----------------------- KÄYTTÖJEN SÄÄTÖ SIVU 1-----------------------------------//

  //-----------SIVU1---------------Back to pages(ESC pressed)-----------------------///

  if (last_esc == HIGH && current_esc == LOW)
  { //Esc button pressed
    lcd.setCursor(0, 2);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(6, 2);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(12, 2);
    lcd.print(" ");       //Delete arrow
    subpage1_counter = 0; //if sub page 0, exit sub menu, up/down pages enabled
  }
  last_esc = current_esc;
}

void mainPage2()
{
  //Design of page 2
  lcd.setCursor(5, 0);
  lcd.print("AUTOMAATTI");
  lcd.setCursor(0, 1);
  lcd.print("VALO:");
  lcd.setCursor(6, 1);
  if (stateArray[3] == HIGH)
  { // AUTO/MAN asetus valoille

    lcd.print("AUTO");
  }
  else
  {
    lcd.print("    ");
    lcd.setCursor(6, 1);
    lcd.print("MAN");
  }
  lcd.setCursor(10, 1);
  lcd.print("ILMA:");
  lcd.setCursor(16, 1);

  if (stateArray[4] == HIGH)
  { //AUTO/MAN asetus ilmapumpulle

    lcd.print("AUTO");
  }
  else
  {
    lcd.print("    ");
    lcd.setCursor(16, 1);
    lcd.print("MAN");
  }

  //----------------SIVU1-----------------------------ajoituksien näkymä SIVU 2--------------------------------------------------//

  lcd.setCursor(2, 2);
  if (timeArray[0] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[0], DEC);
  lcd.print(":");
  if (timeArray[1] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[1], DEC);
  lcd.setCursor(2, 3);
  if (timeArray[2] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[2], DEC);
  lcd.print(":");
  if (timeArray[3] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[3], DEC);
  lcd.setCursor(12, 2);
  if (timeArray[4] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[4], DEC);
  lcd.print(":");
  if (timeArray[5] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[5], DEC);

  lcd.setCursor(12, 3);
  if (timeArray[6] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[6], DEC);
  lcd.print(":");
  if (timeArray[7] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[7], DEC);

  //-------------SIVU2-----Sub counter automaatti control(select control)-----------//

  if (last_sel == HIGH && current_sel == LOW)
  { //select button pressed
    if (subpage2_counter < 6)
    {                     // subpage counter never higher than 1 (total of items)
      subpage2_counter++; //subcounter to move beetwen submenu
    }
    else
    { //If subpage higher than 1 (total of items) return to first item
      subpage2_counter = 1;
    }
  }
  last_sel = current_sel; //Save last state of select button

  //--------------SIVU2--------VALOJEN AUTOMAATTINEN AJASTUS--------------------//
  switch (subpage2_counter)
  {
  case 1:                 //
    lcd.setCursor(11, 3); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(5, 1);  //Place the arrow
    lcd.write(byte(2));   //Place the arrow

    if (last_up == HIGH && current_up == LOW)
    {
      stateArray[3] = HIGH; //valojen automaattiohjaus päälle
      stateArray[1] = LOW;  // valojen manuaaliohjaus pois päältä
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[3] = LOW; // valojen automaattiohjaus pois päältä
      stateArray[1] = LOW; // valojen manuaaliohjaus pois päältä
    }
    last_down = current_down;
    break;
  case 2:
    lcd.setCursor(5, 1); //remove last arrow
    lcd.print(" ");      //remove last arrow
    lcd.setCursor(1, 2); //Place the arrow
    lcd.write(byte(2));  //Place the arrow

    //-------------SIVU2-------------Valojen automaattisen käynnistyksen ajan määritys -------------------//

    settingTime(0, 1); // 0 = time1Hours, 1 = time1Minutes
    //******************************************************************************************************//

    break;
  case 3:
    lcd.setCursor(1, 2); //remove last arrow
    lcd.print(" ");      //remove last arrow
    lcd.setCursor(1, 3); //Place the arrow
    lcd.write(byte(2));  //Place the arrow

    //-------------SIVU2-------------Valojen automaattisen sammutuksen ajan määritys-------------------//
    settingTime(2, 3); // 2 = time2Hours, 3 = time2Minutes
    break;
  case 4:
    lcd.setCursor(1, 3);  //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(15, 1); //Place the arrow
    lcd.write(byte(2));   //Place the arrow

    if (last_up == HIGH && current_up == LOW)
    {
      stateArray[4] = HIGH; //ilmapumpun automaattiohjaus päälle
      stateArray[2] = LOW;  // ilmapumpun manuaaliohjaus pois päältä
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[4] = LOW; //ilmapumpun automaattiohjaus pois päältä
      stateArray[2] = LOW; // ilmapumpun manuaaliohjaus pois päältä
    }
    last_down = current_down;
    break;
  case 5:
    lcd.setCursor(15, 1); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(11, 2); //Place the arrow
    lcd.write(byte(2));

    //----------------SIVU2-----------------ilmapumpun automaattisen käynnistyksen ajan määritys-------------------//

    settingTime(4, 5); // 4 = time3Hours, 5 = time3Minutes
    break;
  case 6:
    lcd.setCursor(11, 2); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(11, 3); //Place the arrow
    lcd.write(byte(2));   //Place the arrow

    //-------------SIVU2-------------ilmapumpun automaattisen sammutuksen ajan määritys-------------------//
    settingTime(6, 7); // 6 = time4Hours, 7 = time4Minutes
    break;
  }
  //--------------------------Back to pages(ESC pressed)-----------------------///

  if (last_esc == HIGH && current_esc == LOW)
  { //Esc button pressed
    lcd.setCursor(5, 1);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(1, 2);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(1, 3);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(15, 1);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(11, 2);
    lcd.print(" "); //Delete arrow
    lcd.setCursor(11, 3);
    lcd.print(" ");       //Delete arrow
    subpage2_counter = 0; //if sub page 0, exit sub menu, up/down pages enabled
  }
  last_esc = current_esc;
}
void mainPage3()
{
  //Design of page 3
  lcd.setCursor(2, 1);
  lcd.print("AJAN MUUTTAMINEN");
  lcd.setCursor(5, 2);
  lcd.print("PAINA SEL");

  //---------------Sub counter control(select control)------------------------//

  if (last_sel == HIGH && current_sel == LOW)
  { //select button pressed
    lcd.clear();
    page_counter = 5; //laittaa sivuHOURSn
  }

  //SIVU 3 LOPPU
}

void mainPage4()
{
  lcd.setCursor(0, 0);
  lcd.print("AUTOMAATTINEN PUMPPU");

  lcd.setCursor(1, 1);
  lcd.print("KASTELU:");
  lcd.setCursor(1, 2);
  if (timeArray[14] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[14], DEC);
  lcd.print("min");
  if (timeArray[15] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[15], DEC);
  lcd.print("sek");
  lcd.setCursor(0, 3);
  lcd.print("TILA:");
  lcd.setCursor(6, 3);
  if (stateArray[5] == HIGH)
  {

    lcd.print("AUTO");
  }
  else
  {
    lcd.print("    ");
    lcd.setCursor(6, 3);
    lcd.print("MAN");
  }
  lcd.setCursor(12, 1);
  lcd.print("1.");
  lcd.setCursor(15, 1);
  if (timeArray[8] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[8], DEC);
  lcd.print(":");
  if (timeArray[9] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[9], DEC);
  lcd.setCursor(12, 2);
  lcd.print("2.");
  lcd.setCursor(15, 2);
  if (timeArray[10] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[10], DEC);
  lcd.print(":");
  if (timeArray[11] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[11], DEC);
  lcd.setCursor(12, 3);
  lcd.print("3.");
  lcd.setCursor(15, 3);
  if (timeArray[12] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[12], DEC);
  lcd.print(":");
  if (timeArray[13] <= 9)
  {
    lcd.print("0");
  }
  lcd.print(timeArray[13], DEC);
  //-----------------------------Back to pages(ESC pressed)--------------------------///

  if (last_esc == HIGH && current_esc == LOW)
  { //Esc button pressed
    lcd.clear();
    subpage4_counter = 0;
    page_counter = 1; //if sub page 0, exit sub menu, up/down pages enabled
  }
  last_esc = current_esc;
  //-----------------------------Sub counter control(select control)-------------------------//

  if (last_sel == HIGH && current_sel == LOW)
  { //select button pressed
    if (subpage4_counter < 5)
    {                     // subpage counter never higher than 1 (total of items)
      subpage4_counter++; //subcounter to move beetwen submenu
    }
    else
    { //If subpage higher than 1 (total of items) return to first item
      subpage4_counter = 1;
    }
  }
  last_sel = current_sel; //Save last state of select button

  //-------------------------------sivu 5------------------------//
  switch (subpage4_counter)
  {
  case 1:
    lcd.setCursor(14, 3); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(0, 2);  //Place the arrow
    lcd.write(byte(2));   //Place the arrow  (

    //-------------SIVU5-------------VESIPUMPUN AUTOMAATTISEN KÄYNNISTYKSEN KASTELUAJAN ASETTAMINEN -------------------//
    if (last_up == HIGH && current_up == LOW)
    {

      if (timeArray[15] >= 30) //timeArray[15] = time8Minutes
      {
        timeArray[15] = 0; //timeArray[15] = time8Minutes

        if (timeArray[14] >= 59) //timeArray[14] = time8Seconds
        {
          timeArray[14] = 0;
        }
        else
        {
          timeArray[14] = timeArray[14] + 1;
        }
      }
      else
      {
        timeArray[15] = timeArray[15] + 30;
      }
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {

      if (timeArray[15] == 0)
      {
        timeArray[15] = 30;
        if (timeArray[14] == 0)
        {
          timeArray[14] = 59;
        }
        else
        {
          timeArray[14] = timeArray[14] - 1;
        }
      }
      else
      {
        timeArray[15] = timeArray[15] - 30;
      }
    }
    last_down = current_down;
    break;
  case 2:

    lcd.setCursor(0, 2); //remove last arrow
    lcd.print(" ");      //remove last arrow
    lcd.setCursor(5, 3); //Place the arrow
    lcd.write(byte(2));  //Place the arrow

    if (last_up == HIGH && current_up == LOW)
    {
      stateArray[5] = HIGH; //Pumpun automaattiohjaus päälle
      stateArray[0] = LOW;  //Pumpun manuaaliohjaus pois päältä
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      stateArray[5] = LOW; //Pumpun automaattiohjaus pois päältä
      stateArray[0] = LOW; // Pumpun manuaaliohjaus pois päältä
    }
    last_down = current_down;
    break;
  case 3:
    lcd.setCursor(5, 3);  //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(14, 1); //Place the arrow
    lcd.write(byte(2));   //Place the arrow
    settingTime(8, 9);    // 8 = time5Hours, 9 = time5Minutes
    break;
  case 4:
    lcd.setCursor(14, 1); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(14, 2); //Place the arrow
    lcd.write(byte(2));   //Place the arrow
    settingTime(10, 11);  // 10 = time6Hours, 11 = time6Minutes
    break;
  case 5:
    lcd.setCursor(14, 2); //remove last arrow
    lcd.print(" ");       //remove last arrow
    lcd.setCursor(14, 3); //Place the arrow
    lcd.write(byte(2));   //Place the arrow

    settingTime(12, 13); // 12 = time7Hours, 13 = time7Minutes
    break;

    //-----------------------------Back to pages(ESC pressed)--------------------------///

    if (last_esc == HIGH && current_esc == LOW)
    { //Esc button pressed
      lcd.clear();
      subpage4_counter = 0;
      page_counter = 1; //if sub page 0, exit sub menu, up/down pages enabled
    }
    last_esc = current_esc;
  }
}
void mainPage5()

{ // Design of page 4

  //-----------------------------Sub counter control(select control)-------------------------//

  if (last_sel == HIGH && current_sel == LOW)
  { //select button pressed
    lcd.clear();
    if (subpage5_counter < 7)
    {                     // subpage counter never higher than 1 (total of items)
      subpage5_counter++; //subcounter to move beetwen submenu
    }
    else
    { //If subpage higher than 1 (total of items) return to first item
      subpage5_counter = 1;
    }
  }
  last_sel = current_sel; //Save last state of select button

  //-------------------------------AJAN MUUTTAMISEEN SETIT------------------------//

  switch (subpage5_counter)
  {
  case 1:
  {
    lcd.setCursor(0, 0);
    lcd.print("Muuta haluttua arvoa");
    lcd.setCursor(0, 1);
    lcd.print("+ / - painikkeilla.");
    lcd.setCursor(0, 2);
    lcd.print("Paina sel, kun arvo");
    lcd.setCursor(0, 3);
    lcd.print("on asetettu.");
    lcd.setCursor(14, 3);
    lcd.write(byte(2));
    lcd.print(" sel");

    // We show the current date and time

    DateTime now = rtc.now();

    hourupg = now.hour();
    minupg = now.minute();

    dayupg = now.day();
    monthupg = now.month();
    yearupg = now.year();
  }
  break;

  case 2:

    if (last_up == HIGH && current_up == LOW)
    {
      if (hourupg == 23)
      {
        hourupg = 0;
        lcd.setCursor(8, 2);
        lcd.print("  ");
      }
      else
      {
        hourupg = hourupg + 1;
      }
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      if (hourupg == 0)
      {
        hourupg = 23;
      }
      else
      {
        hourupg = hourupg - 1;
      }
    }
    last_down = current_down;

    lcd.setCursor(4, 1);
    lcd.print("Aseta tunti:");
    lcd.setCursor(4, 1);
    lcd.setCursor(8, 2);
    lcd.print(hourupg, DEC);

    break;
  case 3:
    // Setting the minutes
    if (last_up == HIGH && current_up == LOW)
    {
      if (minupg == 59)
      {
        minupg = 0;
        lcd.setCursor(8, 2);
        lcd.print("  ");
      }
      else
      {
        minupg = minupg + 1;
      }
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      if (minupg == 0)
      {
        minupg = 59;
      }
      else
      {
        minupg = minupg - 1;
      }
    }
    last_down = current_down;

    lcd.setCursor(4, 1);
    lcd.print("Aseta min  :");
    lcd.setCursor(8, 2);
    lcd.print(minupg, DEC);

    break;

  case 4:
    // setting the year
    if (last_up == HIGH && current_up == LOW)
    {
      yearupg = yearupg + 1;
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      yearupg = yearupg - 1;
    }
    last_down = current_down;

    lcd.setCursor(4, 1);
    lcd.print("Aseta vuosi:");
    lcd.setCursor(7, 2);
    lcd.print(yearupg, DEC);

    break;

  case 5:
    // Setting the month
    if (last_up == HIGH && current_up == LOW)
    {
      if (monthupg == 12)
      {
        monthupg = 1;
        lcd.setCursor(8, 2);
        lcd.print("  ");
      }
      else
      {
        monthupg = monthupg + 1;
      }
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      if (monthupg == 1)
      {
        monthupg = 12;
        lcd.setCursor(8, 2);
        lcd.print("  ");
      }
      else
      {
        monthupg = monthupg - 1;
      }
    }
    last_down = current_down;

    lcd.setCursor(4, 1);
    lcd.print("Aseta kk   :");
    lcd.setCursor(8, 2);
    lcd.print(monthupg, DEC);

    break;

  case 6:
    // Setting the day
    if (last_up == HIGH && current_up == LOW)
    {
      if (dayupg == 31)
      {
        dayupg = 1;
        lcd.setCursor(8, 2);
        lcd.print("  ");
      }
      else
      {
        dayupg = dayupg + 1;
      }
    }
    last_up = current_up;

    if (last_down == HIGH && current_down == LOW)
    {
      if (dayupg == 1)
      {
        dayupg = 31;
      }
      else
      {
        dayupg = dayupg - 1;
      }
    }
    last_down = current_down;

    lcd.setCursor(4, 1);
    lcd.print("Aseta pvm:");
    lcd.setCursor(8, 2);
    lcd.print(dayupg, DEC);
    break;

  case 7:
    // Variable saving
    lcd.setCursor(4, 1);
    lcd.print("TALLENNETAAN");
    lcd.setCursor(5, 2);
    lcd.print("ASETUKSIA");
    rtc.adjust(DateTime(yearupg, monthupg, dayupg, hourupg, minupg, 0));
    delay(500);
    lcd.clear();
    subpage5_counter = 0;
    page_counter = 1;
    break;
  }
}

void setup() //----------------------------------------------------VOID SETUP--------------------------------------//
{

  Serial.begin(9600);

  if (!rtc.begin())
  {
    while (1)
      ;
  }

  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);   // NOTE: You can turn the backlight off by setting it to LOW instead of HIGH
  lcd.begin(20, 4);         //näytön koko
  lcd.createChar(1, back);  //kuvio 1
  lcd.createChar(2, arrow); //kuvio 2

  for (int i = 0; i < numOfInputs; i++) // input pullup
  {
    pinMode(inputPins[i], INPUT_PULLUP);
  }
  for (int i = 0; i < numOfOutputs; i++) // output
  {
    pinMode(outputPins[i], OUTPUT);
  }

  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);

  ReadFromEEPROM(); //reads and saves EEPROM data;
}

void loop()
{

  relays();
  //relaySet(2, 4, 5);
  buttons();

  switch (page_counter)
  {
  case 1: //pääsivu
    mainPage1();
    //subPage1();
    break;
  case 2: //valojen ja ilman säätö
    mainPage2();
    //subPage2();
    break;
  case 3: //
    mainPage3();
    //subPage3();
    break;
  case 4: //
    mainPage4();
    //subPage4();
    break;
  case 5: //
    mainPage5();
    //subPage5();
    break;
  }

  // poista tämä käytöstä niin ei tarvitse resettaa n.50 päivän välein.. sen jälkeen ei enää toimi...
  //-----------Backlight auto off function---------------//

  unsigned long currentMillis = millis(); //call current millis

  if (currentMillis - previousMillis < 0)
  {
    previousMillis = currentMillis;
  }

  if (currentMillis - previousMillis > interval)
  {                                 //If interval is reached :
    previousMillis = currentMillis; //replace previous millis by current millis as new start point
    lcd.clear();
    lcd.setBacklight(LOW); //Turn off backlight

    subpage1_counter = 0;
    subpage2_counter = 0;
    subpage3_counter = 0;
    subpage4_counter = 0;
    subpage5_counter = 0;
    page_counter = 1;
    SaveToEEPROM();
  }

  if (digitalRead(3) == LOW || digitalRead(4) == LOW || digitalRead(6) == LOW || digitalRead(5) == LOW)
  { // Reset millis counter If any button is pressed
    previousMillis = currentMillis;

    //This function only works if backlight is off
    lcd.setBacklight(HIGH); //Turn on backlight
    delay(100);             //Delay to don´t change the page,delete if you don´t care to change page
  }
}
//loop end
