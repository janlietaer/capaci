#include <Arduino.h>
#include "dsmr.h"
#include <Time.h>

// to do , eenheden aanpassen zo dat eenheden niet mee ws zijn maar meer in detail?
volatile int32_t reedsverbruiktditkwartier;         // hoeveel is reeeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
volatile uint32_t maandpiek = 625;                  // gewenste maandelijkse piek  (625*4 = 2500)
volatile int32_t seconde_meterverbruik;             // hoeveel is de laatste seconde verbruikt volgens de slimme meter pa sop kan negatief zijn met pv
volatile uint16_t secondenverinkwartier = 0;        // hoeveel seconden ver zijn we al in het huidig kwartier
volatile int16_t maxverbruikperseconde = 10;        // hoeveel kan verbruikt worden per seconde, zonder geschakelde gebruikers (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde?)
volatile boolean boilerAan = false;                 // moet de boiler aan of uit ?
volatile uint8_t maand, kwartier; 

using MyData = ParsedData<
    /* String */ p1_version,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ power_delivered,
    /* FixedValue */ power_returned>;

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
        MyData data;
        String err;
        if (reader.parse(&data, &err))
        {
            //           1234567890123
            // 0-0:1.0.0(200830134039S)
            if (maand != data.timestamp.substring(3, 4).toInt())
            {
                maand = data.timestamp.substring(3, 4).toInt();
                maandpiek = 625;
            }
            if (kwartier != data.timestamp.substring(9, 10).toInt() / 15)
            {
                kwartier = data.timestamp.substring(9, 10).toInt() / 15;
      
                reedsverbruiktditkwartier = 0;

                if (reedsverbruiktditkwartier > maandpiek) // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
                {
                    maandpiek = reedsverbruiktditkwartier;
                }
            }

            secondenverinkwartier = (data.timestamp.substring(9, 10).toInt() - 15 * kwartier) * 60 + data.timestamp.substring(11, 12).toInt();

            seconde_meterverbruik = data.power_delivered + data.power_returned;

            reedsverbruiktditkwartier += seconde_meterverbruik; // updaten totaal reeds gebruikt in huidig kwartier

            if (reedsverbruiktditkwartier + maxverbruikperseconde * (900 - secondenverinkwartier) > maandpiek) // als vanaf nu voor de rest van het kwartier het volle bak verbruik is, komen we er dan nog?
            {
                boilerAan = false; // deze boolean met een pin verbinden en deze dan gebruiken voor een relaisof shelly, home assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, LOW);
            }
            else
            {
                boilerAan = true; // deze boolean met een pin verbinden en deze dan gebruiken voor een relais of shelly, home assitant,... aan te sturen
                digitalWrite(LED_BUILDIN, HIGH);
            }
            // nog niet erin
            //   wat als er data verloren gaat bij kwartier omaand overgang?
            //   wat als er een maatin overgeslagen wordt( op te lossen door meet duur tyoe te voegen , normaal 1 seconde kan meer zijn)
            //      meet duur moet ook neiuwe maand kwartier compatibel zijn
            // wat gebeurt er als het systeem faalt( alles uit of alles aan?
            // watch dog timer?
        }
        else
        {
            // Parser error, print error
            Serial.println(err);
        }
        reader.clear();
    }
}