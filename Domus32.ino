/*
Domus 3.2 - IoT Datalogger Actuactor
Dan Pejeroni (C) Copyright 2015

Input                                       Morsettiera
A0 - Sensore corrente SCT-013-030           [1-2]  (Attenzione alla freccia del verso)
A1 - Sensore voltaggio 220/9v AC/AC         [3-4]
D3 - Sensore flusso acqua FS-200A           [5(-)6(+)7(s)] (interupt 1)
D2 - BUS Sensori temperatura DS18B20        [8(-)9(+)10(s)]
A2 - Sensore umidità terreno                [11(-)12(+)13(s)]
D9 - Sensore pioggia o allagamento          [14(-)15(+)16(s)]
A3 - Fotoresistenza VT90N3                  [17(-)18(+)]
D8 - Contatto reed (allarme caldaia)        [19(-)20(+)]
D10,11,12,13 riservati a shield Ethernet W5100
D4-7 - Relay Board

Comandi     Risposta
/Emon    : tensione(V);corrente(A);potenza(W)
/Water   : flusso acqua(L/Hour)
/Temp0   : Temperatura Zona Giorno(°C)
/Temp1   : Temperatura Zona Notte(°C)
/Soil    : Umidità terreno (val)
/Rain    : Pioggia(0=pioggia, 1=asciutto)
/Lux     : Luminosità(Lux) aprox
/Alarm   : Allarme blocco caldaia (0=blocco, 1=ok)
/01_on   : Attiva relè01 e restiruisce stato di tutti(1000)
/01_off  : Disattiva relè01 e restiruisce stato di tutti(0000)
(vale per i relè 01-04)

*/

#include "EmonLib.h"                 // Include Emon Library
EnergyMonitor emon1;                 // Crea un'istanza
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
EthernetServer server(85);

// Sonde temperatura DS18D20
#define ONE_WIRE_BUS 2               // Data wire della sonda collegata al pin 2 digitale
OneWire oneWire(ONE_WIRE_BUS);       // Setup dell'istanza oneWire per comunicare con dispositivi OneWire
DallasTemperature sensors(&oneWire); // Passa il reference di oneWire a Dallas Temperature. 

// Water Flow Sensor
volatile int NbTopsFan;             //Measuring the rising edges of the signal
int Calc;                               
int hallsensor = 3;                 //The pin location of the sensor
void rpm ()                         //The function that the interupt calls 
{ 
  NbTopsFan++;                      //Measures the rising and falling edge of the hall effect sensors signal
} 

#define fotoresistenza A3

int soil=0;                         //Sensore umidità terreno
int umidita=0;                      //Sensore umidità terreno

// Inizializzo i 4 pin per i relays
const byte relay01 = 4;             // Select pin for relay01
const byte relay02 = 5;             // Select pin for relay02
const byte relay03 = 6;             // Select pin for relay03
const byte relay04 = 7;             // Select pin for relay04

String readString;

void setup() {
  
pinMode(relay01, OUTPUT);         // Define pin for relay01
pinMode(relay02, OUTPUT);         // Define pin for relay02
pinMode(relay03, OUTPUT);         // Define pin for relay03
pinMode(relay04, OUTPUT);         // Define pin for relay04

pinMode(9, INPUT);                // Sensore pioggia
pinMode(8, INPUT);                // Blocco Caldaia
pinMode(A3,INPUT);                // Fotoresistenza

// Start up the library
 sensors.begin(); 
  
Ethernet.begin(mac, ip, dns, gateway);
server.begin();

// Emon Library
emon1.current(0, 30);             // Current: input pin, calibration
emon1.voltage(1, 234.26, 1.7);    // Voltage: input pin, calibration, phase_shift

// Seeduino
pinMode(hallsensor, INPUT);       //initializes digital pin 3 as an input
attachInterrupt(1, rpm, RISING);  //and the interrupt is attached

}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
      char c = client.read();
      if (readString.length() < 100)
      {
      readString = readString + c;// Store characters to string
      }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          // client.println("<!DOCTYPE HTML>"); // nel caso di servizi REST non servono
          // client.println("<html>"); // nel caso di servizi REST non servono 
        
          // Se la richiesta è /Emon leggo e restituisco i valori di energia elettrica       
          if(readString.indexOf("/Emon") > 0) {
          emon1.calcVI(20,2000);                          // Calculate all. No.of half wavelengths (crossings), time-out
          // emon1.serialprint();                         // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
          float realPower       = emon1.realPower;        //extract Real Power into variable
          float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
          float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
          float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
          float Irms            = emon1.Irms;             //extract Irms into Variable
          if(emon1.Irms < 0,1) {                          // correzione 0 A
          emon1.Irms = 0;                               
          }
//        client.print(emon1.Vrms);                     // Restituisco la tensione
          client.print(220);                            // In assenza di sensore fisso il valore di tensione
          client.print(";");
          client.print(emon1.Irms);                     // Restituisco la corrente
          client.print(";");
//        client.print(emon1.powerFactor);              // Restituisco il cosfi
//        client.print(";");
//        client.print(emon1.apparentPower);            // Restituisco la potenza apparente
//        client.print(emon1.realPower);                // Restituisco la potenza reale
          client.print(emon1.Irms*220);                 // In assenza di sensore calcolo il valore di potenza
          }
        
          // Se la richiesta è /Water leggo e restituisco i valori di flusso acqua      
          if(readString.indexOf("/Water") > 0) {
          NbTopsFan = 0;                    //Set NbTops to 0 ready for calculations
          sei();                            //Enables interrupts
          delay (1000);                     //Wait 1 second
          cli();                            //Disable interrupts
          Calc = (NbTopsFan * 60 / 5.5);    //(Pulse frequency x 60) / 5.5Q, = flow rate in L/hour 
          client.print (Calc, DEC);         //Prints the number calculated above (L/hour)
          }
        
          // Se la richiesta è /Temp0 leggo e restituisco il valore della sonda DS18D20        
          if(readString.indexOf("/Temp0") > 0) {
          sensors.requestTemperatures();            // Send the command to get temperatures
          client.print(sensors.getTempCByIndex(0)); // Restituisce la temperatura della prima sonda
          }
          
          // Se la richiesta è /Temp1 leggo e restituisco il valore della sonda DS18D20        
          if(readString.indexOf("/Temp1") > 0) {
          sensors.requestTemperatures();            // Send the command to get temperatures
          client.print(sensors.getTempCByIndex(1)); // Restituisce la temperatura della seconda sonda
          }
          
          // Se la richiesta è /Soil leggo e restituisco il valore umidità terreno        
          if(readString.indexOf("/Soil") > 0) {
          soil = analogRead(2);
          umidita = map (soil, 100, 970, 100, 0);
          client.print(umidita); 
          delay(100);
          }
          
          // Se la richiesta è /Rain leggo e restituisco il valore input digitale 9
          if(readString.indexOf("/Rain") > 0) { 
          int sensorReading = digitalRead(9);     // output the value of digital input pin 9
          client.print(sensorReading);
          }
          
          // Se la richiesta è /Lux leggo e restituisco il valore della luce        
          if(readString.indexOf("/Lux") > 0) {
          int luce = analogRead(fotoresistenza);  // salvo il valore fotoresistenza dentro alla variabile val
          if(luce < 1020) {
          luce = (3000 / luce);                   // calcolo approssimativo valore in Lux
          }
          else
          {
            luce = 0;
          }
          client.print(luce, DEC);               // Scrivo il valore della fotoresistenza, espresso in numeri decimali
          }
          
          // Se la richiesta è /Alarm leggo e restituisco il valore input digitale 8
          if(readString.indexOf("/Alarm") > 0) { 
          int sensorReading = digitalRead(8);   // output the value of digital input pin 8
          client.print(sensorReading);
          }
          
          // Se la richiesta è /Relay leggo e restituisco la condizione di tutti i Relè
          if(readString.indexOf("/Relay") > 0) { 
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          // Setto i Relè
          }
          if(readString.indexOf("/01_on") > 0) { 
          digitalWrite(relay01, HIGH);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/01_off") > 0) { 
          digitalWrite(relay01, LOW);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/02_on") > 0) { 
          digitalWrite(relay02, HIGH);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/02_off") > 0) { 
          digitalWrite(relay02, LOW);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/03_on") > 0) { 
          digitalWrite(relay03, HIGH);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/03_off") > 0) { 
          digitalWrite(relay03, LOW);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/04_on") > 0) { 
          digitalWrite(relay04, HIGH);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }
          if(readString.indexOf("/04_off") > 0) { 
          digitalWrite(relay04, LOW);
          String relay = "";
          relay += digitalRead(4);
          relay += digitalRead(5);
          relay += digitalRead(6);
          relay += digitalRead(7);
          client.print(relay);
          }

          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();         
    readString = "";// Clearing string for next read    
  }
}
