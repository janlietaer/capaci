#include <Arduino.h>
#include "dsmr.h"
#include <Time.h>

// to do , neenhden aanpassen zo dat eenheden niet mee ws zijn maar meer in detail?
volatile int32_t reedsverbruiktditkwartier;         // hoeveel is reeeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
volatile uint32_t maandpiek = 625;                  // gewenste maandelijkse piek  (625*4 = 2500)
volatile int32_t seconde_meterverbruik;             // hoeveel is de laatste soconde verbruikt volgens de slimme meter pa sop kan negatief zijn met pv
volatile boolean nieuwkwartier = false;             // is er een nieuw kwartier begonnen?
volatile boolean nieuwemaand = false;               // is er een nieuwe maand begonnen?
volatile uint16_t secondenverinkwartier = 0;        // hoeveel seconden ver zijn we al in het huidig kwartier
volatile int16_t maxverbruikperseconde = 10;        // hoeveel kan verbruikt worden per seconde (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde?)
volatile boolean boilerAan = false;                 // moet de boiler aan of uit ?
volatile boolean Nieuwe_Meter_Communicatie = false; // is er nieuwe communicatie vanuit de meter?

volatile uint8_t maand, kwartier, secondeinkwartier;
float janf;
uint32_t janint;

String janss;

using MyData = ParsedData<
    /* String */ identification,
    /* String */ p1_version,
    /* String */ timestamp,
    /* String */ equipment_id,
    /* FixedValue */ energy_delivered_tariff1,
    /* FixedValue */ energy_delivered_tariff2,
    /* FixedValue */ energy_returned_tariff1,
    /* FixedValue */ energy_returned_tariff2,
    /* String */ electricity_tariff,
    /* FixedValue */ power_delivered,
    /* FixedValue */ power_returned,
    /* FixedValue */ electricity_threshold,
    /* uint8_t */ electricity_switch_position,
    /* uint32_t */ electricity_failures,
    /* uint32_t */ electricity_long_failures,
    /* String */ electricity_failure_log,
    /* uint32_t */ electricity_sags_l1,
    /* uint32_t */ electricity_sags_l2,
    /* uint32_t */ electricity_sags_l3,
    /* uint32_t */ electricity_swells_l1,
    /* uint32_t */ electricity_swells_l2,
    /* uint32_t */ electricity_swells_l3,
    /* String */ message_short,
    /* String */ message_long,
    /* FixedValue */ voltage_l1,
    /* FixedValue */ voltage_l2,
    /* FixedValue */ voltage_l3,
    /* FixedValue */ current_l1,
    /* FixedValue */ current_l2,
    /* FixedValue */ current_l3,
    /* FixedValue */ power_delivered_l1,
    /* FixedValue */ power_delivered_l2,
    /* FixedValue */ power_delivered_l3,
    /* FixedValue */ power_returned_l1,
    /* FixedValue */ power_returned_l2,
    /* FixedValue */ power_returned_l3,
    /* uint16_t */ gas_device_type,
    /* String */ gas_equipment_id,
    /* uint8_t */ gas_valve_position,
    /* TimestampedFixedValue */ gas_delivered,
    /* uint16_t */ thermal_device_type,
    /* String */ thermal_equipment_id,
    /* uint8_t */ thermal_valve_position,
    /* TimestampedFixedValue */ thermal_delivered,
    /* uint16_t */ water_device_type,
    /* String */ water_equipment_id,
    /* uint8_t */ water_valve_position,
    /* TimestampedFixedValue */ water_delivered>;

P1Reader reader(&Serial1, 2);
#define  LED_BUILDIN 22
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

void bereken(boolean lnieuwkwartier, boolean lnieuwemaand, uint32_t lseconde_meterverbruik, int16_t lsecondenverinkwartier)
{
    if (lnieuwkwartier == true) // een nieuw kwartier begon
    {
        reedsverbruiktditkwartier = 0;

        if (reedsverbruiktditkwartier > maandpiek) // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
        {
            maandpiek = reedsverbruiktditkwartier;
        }
    }

    if (lnieuwemaand == true) // een nieuwe maand begon
    {

        maandpiek = 625;
    }

    reedsverbruiktditkwartier += lseconde_meterverbruik; // updaten totaal reeds gebruikt in huidig kwartier

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

            seconde_meterverbruik = 0;

            nieuwemaand = false;
            nieuwkwartier = false;

            if (maand != data.timestamp.substring(3, 4).toInt())
            {
                maand = data.timestamp.substring(3, 4).toInt();
                nieuwemaand = true;
            }
            if (kwartier != data.timestamp.substring(9, 10).toInt() / 15)
            {
                kwartier = data.timestamp.substring(9, 10).toInt() / 15;
                nieuwkwartier = true;
            }
            secondeinkwartier = (data.timestamp.substring(9, 10).toInt() - kwartier) * 60 + data.timestamp.substring(11, 12).toInt();

            // jan= data power_delivered.

            if (data.power_delivered >= 0)
            {
                seconde_meterverbruik = data.power_delivered;
            }
            if (data.power_returned < 0)
            {
                seconde_meterverbruik = data.power_returned;
            }

            bereken(nieuwemaand, nieuwkwartier, seconde_meterverbruik, secondeinkwartier);
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