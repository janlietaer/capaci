#include <Arduino.h>
#include "dsmr.h"
#include <Time.h>
using MyData = ParsedData<
    /* String */ p1_version,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ power_delivered,
    /* FixedValue */ power_returned>;

int32_t reedsverbruiktditkwartier;  // hoeveel is reeeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
uint32_t maandpiek = 625;           // gewenste maandelijkse piek  (625*4 = 2500)
int32_t seconde_meterverbruik;      // hoeveel is de laatste seconde verbruikt volgens de slimme meter pa sop kan negatief zijn met pv
uint16_t secondenverinkwartier = 0; // hoeveel seconden ver zijn we al in het huidig kwartier
int16_t maxverbruikperseconde = 10; // hoeveel kan verbruikt worden per seconde, zonder geschakelde gebruikers (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde?)
boolean boilerAan = false;          // moet de boiler aan of uit ?
uint8_t maand, kwartier;
String err;
MyData data;

P1Reader reader(&Serial1, 2);
#define LED_BUILDIN 22
void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);
    pinMode(LED_BUILDIN, OUTPUT);
#ifdef VCC_ENABLE
    // This is needed on Pinoccio Scout boards to enable the 3V3 pin.
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
#endif
    // start a read right away
    reader.enable(true);
}

void loop()
{
    if (reader.available())
    {
        if (reader.parse(&data, &err))
        { //           1234567890123
            // 0-0:1.0.0(200830134039S)
            if (maand != data.timestamp.substring(3, 4).toInt()) // is de maand juist voorbij?  zet het maand piek terugn op 625 wh
            {
                maand = data.timestamp.substring(3, 4).toInt();
                maandpiek = 625;
                Serial.print("Nieuwemaand ");
            }
            if (kwartier != data.timestamp.substring(9, 10).toInt() / 15) // is het kwartier juist voorbij?
            {
                kwartier = data.timestamp.substring(9, 10).toInt() / 15;
                if (reedsverbruiktditkwartier > maandpiek) // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
                {
                    maandpiek = reedsverbruiktditkwartier;
                }
                Serial.print("kwartiereindverbuikt \t");
                Serial.println(reedsverbruiktditkwartier);
                reedsverbruiktditkwartier = 0; // zet het kwartier verbruik terug op 0
            }
            secondenverinkwartier = (data.timestamp.substring(9, 10).toInt() - 15 * kwartier) * 60 + data.timestamp.substring(11, 12).toInt();
            reedsverbruiktditkwartier += data.power_delivered + data.power_returned;                           // updaten totaal reeds gebruikt in huidig kwartier PAS OP GAAT MIS ALS DIT BERICHT NIET IEDERE SECONDE KOMT
            if (reedsverbruiktditkwartier + maxverbruikperseconde * (900 - secondenverinkwartier) > maandpiek) // als vanaf nu voor de rest van het kwartier het volle bak verbruik is, komen we er dan nog?
            {
                boilerAan = false; // deze boolean met een pin verbinden en deze dan gebruiken voor een relaisof shelly, home assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, LOW);

                Serial.print("secondenverinkwartier \t");
                Serial.print(secondenverinkwartier);
                Serial.print("kwartiereindverbuikt \t");
                Serial.println(reedsverbruiktditkwartier);
                Serial.println("\t uit ");
            }
            else
            {
                boilerAan = true; // deze boolean met een pin verbinden en deze dan gebruiken voor een relais of shelly, home assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, HIGH);
                Serial.print("secondenverinkwartier \t");
                Serial.print(secondenverinkwartier);
                Serial.print("kwartiereindverbuikt \t");
                Serial.print(reedsverbruiktditkwartier);
                Serial.println("\t aan ");
            }
            // nog niet erin
            //   wat als er een meeting overgeslagen wordt( op te lossen door meet duur tyd toe te voegen , normaal 1 seconde kan meer zijn)
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