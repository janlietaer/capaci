#include <Arduino.h>
#include "dsmr.h"
#include <Time.h>
/* missing  geschakeld vermogen(s) moeten ook meestpelen in het algorithme  */

using MyData = ParsedData<
    /* String */ p1_version,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ power_delivered,
    /* FixedValue */ power_returned>;

int32_t reedsverbruiktditkwartier;      // hoeveel is reeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
uint32_t maandpiek = 2250000;           // gewenste maandelijkse piek  (2500 *900 = 2250000) Watt seconden als default
int32_t seconde_meterverbruik;          // hoeveel is de laatste seconde verbruikt volgens de slimme meter pas op kan negatief zijn met PV
uint16_t secondenverinkwartier = 0;     // hoeveel seconden ver zijn we al in het huidig kwartier 0>900
int16_t maxverbruikperseconde = 12000;  // hoeveel kan verbruikt worden per seconde, zonder geschakelde gebruikers (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde? eenheid is Watt seconde)
boolean boilerAan = false;              // moet de boiler aan of uit ?
uint8_t maand, kwartier;                // maand  1>12 , kwartier 
String err;
MyData data;

P1Reader reader(&Serial1, 2);
#define LED_BUILDIN 22
void setup()
{
    Serial.begin(115200);  // om voor debugging
    Serial1.begin(115200); // voor meter uit te lezen
    pinMode(LED_BUILDIN, OUTPUT);
    // start a read right away
    reader.enable(true);
}

void loop()
{
    if (reader.available())
    {
        if (reader.parse(&data, &err))
        {   //            01234567890123
            // 0-0:1.0.0(200830134039S)
            if (maand != data.timestamp.substring(2, 4).toInt()) // is de maand juist voorbij?  zet het maand piek terugn op 2250000 Wsec
            {
                maand = data.timestamp.substring(2, 4).toInt();
                maandpiek = 2250000;
                Serial.println("Nieuwemaand ");
            }
            if (kwartier != data.timestamp.substring(8, 10).toInt() / 15) // is het kwartier juist voorbij?
            {
                kwartier = data.timestamp.substring(8, 10).toInt() / 15;
                if (reedsverbruiktditkwartier > maandpiek) // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
                {
                    maandpiek = reedsverbruiktditkwartier;
                }
                Serial.print("kwartiereindverbuiktWs \t");
                Serial.println(reedsverbruiktditkwartier);
                reedsverbruiktditkwartier = 0; // zet het kwartier verbruik terug op 0
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
                Serial.println("\tuit ");
            }
            else
            {
                boilerAan = true; // deze boolean met een pin verbinden en deze dan gebruiken voor een  relais of Shelly, Home Assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, HIGH);
                Serial.print("secondenverinkwartier \t");
                Serial.print(secondenverinkwartier);
                Serial.print("\tkwartiereindverbuiktWs\t");
                Serial.print(reedsverbruiktditkwartier);
                Serial.println("\taan ");
            }
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
}