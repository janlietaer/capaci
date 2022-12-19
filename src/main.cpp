#include <Arduino.h>
#include "dsmr.h"
#include <Time.h>
#include <TFT_eSPI.h>
// #include "TTGO_T_Display.h"
/* missing  geschakeld vermogen(s) moeten ook meestpelen in het algorithme  */

using MyData = ParsedData<
    /* String */ p1_version,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ power_delivered,
    /* FixedValue */ power_returned>;

int32_t reedsverbruiktditkwartier;        // hoeveel is reeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
uint32_t maandpiek = 2250000;             // gewenste maandelijkse piek  (2500 *900 = 2250000) Watt seconden als default
int32_t seconde_meterverbruik;            // hoeveel is de laatste seconde verbruikt volgens de slimme meter pas op kan negatief zijn met PV
uint16_t secondenverinkwartier = 0;       // hoeveel seconden ver zijn we al in het huidig kwartier 0>900
uint16_t secondenverinkwartierSwitch = 0; // hoeveel seconden ver zijn we al in het huidig kwartier 0>900
int16_t maxverbruikperseconde = 12000;    // hoeveel kan verbruikt worden per seconde, zonder geschakelde gebruikers (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde? eenheid is Watt seconde)
boolean boilerAan = false;                // moet de boiler aan of uit ?
uint8_t maand, kwartier;                  // maand  1>12 , kwartier
String err;
MyData data;

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

P1Reader reader(&Serial1, 2);
#define LED_BUILDIN 22
void setup()
{
    Serial.begin(115200);  // om voor debugging
    //Serial1.begin( 115200,,27,,,,); // voor meter uit te lezen
    Serial1.begin(  115200, SERIAL_8N1, 27, 26 );
    pinMode(LED_BUILDIN, OUTPUT);
    // start a read right away
    reader.enable(true);
 
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);
    tft.setTextDatum(0);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void loop()
{
    if (reader.available())
    {
        if (reader.parse(&data, &err))
        { //            01234567890123  
            // 0-0:1.0.0(200830134039S) 
            if (kwartier != data.timestamp.substring(8, 10).toInt() / 15) // is het kwartier juist voorbij?
            {
                tft.drawString("Switchtijd vorig: " + String(secondenverinkwartierSwitch, 1) + " sec ", 1, 90, 4);
                tft.drawString("Verbruik vorig: " + String(reedsverbruiktditkwartier, 1) + " Ws ", 1, 110, 4);

                kwartier = data.timestamp.substring(8, 10).toInt() / 15;
                if (reedsverbruiktditkwartier > maandpiek) // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
                {
                    maandpiek = reedsverbruiktditkwartier;
                }
                Serial.print("kwartiereindverbuiktWs \t");
                Serial.println(reedsverbruiktditkwartier);
                reedsverbruiktditkwartier = 0; // zet het kwartier verbruik terug op zeor 0
                secondenverinkwartierSwitch = 0;
            }
            if (maand != data.timestamp.substring(2, 4).toInt()) // is de maand juist voorbij?  zet het maand piek terugn op 2250000 Wsec
            {
                maand = data.timestamp.substring(2, 4).toInt();
                maandpiek = 2250000;
                Serial.println("Nieuwemaand ");
            }
            secondenverinkwartier = (data.timestamp.substring(8, 10).toInt() - 15 * kwartier) * 60 + data.timestamp.substring(10, 12).toInt();
            reedsverbruiktditkwartier += data.power_delivered + data.power_returned;                           // updaten totaal reeds gebruikt in huidig kwartier PAS OP GAAT MIS ALS DIT BERICHT NIET IEDERE SECONDE KOMT
            if (reedsverbruiktditkwartier + maxverbruikperseconde * (900 - secondenverinkwartier) > maandpiek) // als vanaf nu voor de rest van het kwartier het volle bak verbruik is, komen we er dan nog?
            {
                boilerAan = false; // deze boolean met een pin verbinden en deze dan gebruiken voor een relais of Shelly, Home Assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, LOW);
                Serial.print("secondenverinkwartier\t");
                Serial.print(secondenverinkwartier);
                Serial.print("\tkwartiereindverbuiktWs\t");
                Serial.println(reedsverbruiktditkwartier);
                Serial.println("\tUIT");
            }
            else
            {
                boilerAan = true; // deze boolean met een pin verbinden en deze dan gebruiken voor een  relais of Shelly, Home Assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, HIGH);
                secondenverinkwartierSwitch = secondenverinkwartier;
                Serial.print("secondenverinkwartier \t");
                Serial.print(secondenverinkwartier);
                Serial.print("\tkwartiereindverbuiktWs\t");
                Serial.print(reedsverbruiktditkwartier);
                Serial.println("\tAAN");
            }

            //   boilerAan
            // nog niet erin
            //   wat als er een meeting overgeslagen wordt( op te lossen door meet duur tijd toe te voegen , normaal 1 seconde kan meer zijn)
            //      meet duur moet ook nieuwe maand kwartier compatibel zijn
            // wat gebeurt er als het systeem faalt( alles uit of alles aan?
            // watch dog timer?
        }
        else
        {
            Serial.println(err); // Parser error, print error
        }
        reader.clear(); 
    }

    delay(250);
    tft.drawString("Tijd: " + String(secondenverinkwartier, 1) + " sec ", 1, 0, 4);
    tft.drawString("Verbruik: " + String(reedsverbruiktditkwartier, 1) + " Ws ", 1, 20, 4);
    tft.drawString("Maandpiek: " + String(maandpiek, 1) + " Ws ", 1, 40, 4);
    if (boilerAan == true)
    {
        tft.drawString(" AAN ", 1, 65, 4);
    }
    else
    {
        tft.drawString(" UIT ", 1, 65, 4);
    }
}