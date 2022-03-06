#include <Arduino.h>
// to do , neenhden aanpassen zo dat eenheden niet mee ws zijn maar meer in detail?
volatile int32_t reedsverbruiktditkwartier;          // hoeveel is reeeds verbruik in het huidig kwartier, pas op kan negatief zijn als er zonnepanelen zijn
volatile uint32_t maandpiek = 625;                  // gewenste maandelijkse piek  (625*4 = 2500)
volatile int32_t seconde_meterverbruik;             // hoeveel is de laatste soconde verbruikt volgens de slimme meter pa sop kan negatief zijn met pv
volatile boolean nieuwkwartier = false;             // is er een nieuw kwartier begonnen?
volatile boolean nieuwemaand = false;               // is er een nieuwe maand begonnen?
volatile uint16_t secondenverinkwartier = 0;        // hoeveel seconden ver zijn we al in het huidig kwartier
volatile int16_t maxverbruikperseconde = 10;        // hoeveel kan verbruikt worden per seconde (ofwel max vermogen dat door de hoofd zekering kan, ofwel historische piek per seconde?)
volatile boolean boilerAan = false;                 // moet de boiler aan of uit ?
volatile boolean Nieuwe_Meter_Communicatie = false; // is er nieuwe communicatie vanuit de meter?

void loop()
{
    //  hier moet iets komen dat 
    //  de meter uitleest in de variabele seconde_meterverbruik 
    //  nieuw kwartier  op true zet als er een nieuw kwartier begint
    //  nieuwe maand  op true zet als er een nieuw maand begint

    if (Nieuwe_Meter_Communicatie == true)              // er is een nieuw bericht uit de meter
    {
        if (nieuwkwartier == true)                      // een nieuw kwartier begon
        {
            nieuwkwartier = false;
            reedsverbruiktditkwartier = 0;
            secondenverinkwartier = 0;
            if ( reedsverbruiktditkwartier>maandpiek)   // in kwartier dat juist voorbij is was het verbruik hoger dan de vorige maand record
            {
                maandpiek = reedsverbruiktditkwartier;
            }
        }

        if (nieuwemaand == true)                        // een nieuwe maand begon
        {
            nieuwemaand = false;
            maandpiek = 625;
        }

        secondenverinkwartier++;                             // we zijn een seconde verder  , dit kan beter door de tijd uit de meter communicatie te halen?
        reedsverbruiktditkwartier +=  seconde_meterverbruik; // updaten totaal reeds gebruikt in huidig kwartier
      
        if (reedsverbruiktditkwartier + maxverbruikperseconde * (900 - secondenverinkwartier) > maandpiek)   // als vanaf nu voor de rest van het kwartier het volle bak verbruik is, komen we er dan nog?
        {
            boilerAan = false;  // deze boolean met een pin verbinden en deze dan gebruiken voor een relaisof shelly, home assitant,... aan te sturen
        }
        else
        {
            boilerAan = true;   // deze boolean met een pin verbinden en deze dan gebruiken voor een relais of shelly, home assitant,... aan te sturen
        }
 
    }
}