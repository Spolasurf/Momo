#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
 
WiFiClient espClient;
PubSubClient client(espClient);
 
const char* SSID = "openplotter";
const char* PSK = "Sandy2018";
const char* MQTT_BROKER = "192.168.1.133";



int isMqttPublished = 0;
int isPublished = 0;
int interval = 600000;
unsigned long int warte = -interval;
unsigned long int ersatzDelay = 0;
unsigned long int lastPublishedStatus = 0;

char buffer[7];
std::string statusMsg;

typedef struct Thermometer
{
   int temp; //Temperaturwerte
   int temptemp; //temporäre Temperaturwerte
   char name[20]; //MQTT temperaturverzeichnis
 } Thermometer;

//Thermometer initialisieren
Thermometer temp1 = {-9999, -9999, "heizung/temp1"};
Thermometer temp2 = {-9999, -9999, "heizung/temp2"};
Thermometer temp3 = {-9999, -9999, "heizung/temp3"};
Thermometer temp4 = {-9999, -9999, "heizung/temp4"};
Thermometer temp5 = {-9999, -9999, "heizung/temp5"};


//Thermometer in ein Array schreiben
Thermometer Thermometers[5] = {temp1, temp2, temp3, temp4, temp5};

typedef struct Consumer //angeschlossene Relais
{
   int pin; //PIN
   int state; //Status (an/aus)
   char name[20]; //MQTT-Sende-Verzeichnis
   char name2[20]; //MQTT-Empfangsverzeichniss
 } Consumer;

//Consumer initialisieren
Consumer komp = {D1, LOW, "heizung/komp", "heizung/kompon"}; //Kompressor
Consumer luft = {D2, LOW, "heizung/luft", "heizung/lufton"}; //Lüfter
Consumer hcswitch = {D3, LOW, "heizung/hcswitch", "heizung/hcswitchon"}; //Schalter zw. Heizen/Kühlen


//Consumer in ein Array schreiben
Consumer relays[3] = {komp, luft, hcswitch};

int soll = 18; //Solltemperatur
int OnOff = 0;
int hcSwitch = 0;
 
void setup() {
    Serial.begin(115200);
    delay(10);
    setup_wifi();
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);

    for(int i = 0; i < 3; i++)
    {
      pinMode(relays[i].pin, OUTPUT);
    }    
    digitalWrite(D3, LOW);
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
}

unsigned long int lastTry = -60000;
unsigned long int lastTryMqtt = -10000;

void setup_wifi()
{
    if (millis() - lastTry > 6000) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
        }
        else {
            Serial.println("WIFI nicht verbunden. Versuche Verbindung herzustellen in 6s.");
            WiFi.mode(WIFI_STA);
            WiFi.begin(SSID, PSK);
        }
        lastTry = millis();
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    Serial.println();
 
    msg[length] = '\0';
    Serial.println(msg);

    if(strcmp(topic, "heizung/OnOff") == 0) OnOff = atoi(msg);
    if(strcmp(topic, "heizung/solltemp") == 0) soll = atoi(msg);
    if(strcmp(topic, "heizung/hcswitch") == 0) hcSwitch = atoi(msg);
    return;
}
 
void reconnect()
{
    if (millis() - lastTryMqtt > 3000 && WiFi.status() == WL_CONNECTED) {
        if (client.connect("arduinoClient2", "Sandy", "Sandy2018")) {
          client.subscribe("heizung/OnOff");
          client.subscribe("heizung/solltemp");
          for (int j = 0; j < 3; j++)
          {
            client.subscribe(relays[j].name2);
          } 
        }
        else {
            Serial.println("MQTT nicht verbunden. Versuche Verbindung erneut herzustellen in 3s.");
            lastTryMqtt = millis();
        }
    }
}

void loop() {
    isMqttPublished = 0;
    if(WiFi.status() != WL_CONNECTED) {
        setup_wifi();
    }
    if (!client.connected()) {
        reconnect();
    }
    
    for (int i = 0; i < 5; i++) //fall sich der Temperaturwert geändert hat, wird er geändert
      {
          sensors.requestTemperatures(); //Thermometer werden ausgelesen...
          Thermometers[i].temptemp = sensors.getTempCByIndex(i);

        if (Thermometers[i].temp != Thermometers[i].temptemp)
          {
            Thermometers[i].temp = Thermometers[i].temptemp;
            if (Thermometers[i].temp == 85) break;
            publishMsg(Thermometers[i].name, Thermometers[i].temp);
            if (Thermometers[i].temptemp == -9999)
            {
              publishMsg("heizung/status", "Thermometer einstecken. BITTE");
            }
            break;
          }
      }     

    if(millis()-lastPublishedStatus > 1000) {
      char msg[60];
      strcpy(msg, statusMsg.c_str());
      publishMsg("heizung/status", msg);
      lastPublishedStatus  = millis();
    }

    if(millis() - ersatzDelay > 60000) {
      if(OnOff == 1)
      {
          if(hcSwitch == 1)
          {
              kuehlen();  
          } else 
          {
              heizen();
          }
  
          if (Thermometers[1].temp > 75 && relays[0].state == 1)
          {
              statusMsg = "ACHTUNG Kompressor zu heiß! Notabschaltung erfolgt";
              schalt(relays[0], LOW);  
              warte= millis();     
              ersatzDelay = millis();
              isPublished = 0;
          }
      
          if (Thermometers[3].temp < -10 && relays[0].state == 1)
          {
              statusMsg = "ACHTUNG Verdampfer zu kalt! Notabschaltung erfolgt";
              schalt(relays[0], LOW);       
              warte = millis(); 
              ersatzDelay = millis();
              isPublished = 0;
          }    
      } 
      else
      {
          for(int i=0; i < 3; i++)
          {
            if(relays[i].state == 1) 
            {
              schalt(relays[i], LOW);
              warte = millis();
            }
          }
          if(isPublished != 5)
          {
             statusMsg = "100% Betriebsbereit!!";
             isPublished = 5;
          }
      } 
    }
    client.loop();
}

void heizen()
{
     if(relays[2].state == 1 && relays[0].state == 0)
     {
         schalt(relays[2], LOW);
     }
     if(relays[2].state == 1 && relays[0].state == 1)
     {  
        schalt(relays[0], LOW);
        statusMsg = "Zu viel Druck zum umschalten. Momentchen noch...";
        isPublished = 0;
        ersatzDelay = millis();
        schalt(relays[2], LOW);
     }
  
     if (soll > Thermometers[2].temp)
     {
        if(millis() > warte + interval && relays[0].state == 0) 
        {
          schalt(relays[0], HIGH);
          warte = millis();
          statusMsg = "Brrr kalt! Volle Heizkraft!"; 
          isPublished = 0;
          Serial.println("Komp an");   
        }
        else
        { 
          if(relays[0].state == 0 && isPublished != 1)
          {   
              statusMsg = "Ich steh in den Startlöchern.. Gleich heiz ich euch ein!";  
              isPublished = 1; 
          }
        }
     }
     else 
     {
        if(relays[0].state == 1 && millis() > warte + interval)
        {
            schalt(relays[0], LOW);
            warte = millis();
            statusMsg = "hmmm schön warmhier! Ich mach mal Pause";
            isPublished = 2;
            
            Serial.println("Komp aus");
        }
       if(relays[0].state == 0 && isPublished != 2) 
       {
           statusMsg = "hmmm schön warmhier! Ich mach mal Pause";
           isPublished = 2;
       }
     } 

    if (relays[0].state == 1 && relays[1].state == 0 && (Thermometers[0].temp >= (Thermometers[2].temp + 10) || Thermometers[1].temp > 50))
    {
        schalt(relays[1], HIGH);     
        Serial.println("Lüfter an");   
    }
    if (relays[0].state == 0 && relays[1].state == 1 && Thermometers[0].temp <= (Thermometers[2].temp + 5))
    {
        schalt(relays[1], LOW); 
        Serial.println("Lüfter aus");          
    }
}

void kuehlen()
{
     if(relays[2].state == 0 && relays[0].state == 0)
     {
         schalt(relays[2], HIGH);
     }
     if(relays[2].state == 0 && relays[0].state == 1)
     {  
        schalt(relays[0], LOW);
        statusMsg = "Zu viel Druck zum umschalten. Momentchen noch...";
        ersatzDelay = millis();
        schalt(relays[2], HIGH);
        isPublished = 0;
     }

     if (soll < Thermometers[2].temp)
     {
        if(millis() > warte + interval && relays[0].state == 0) 
        {
          schalt(relays[0], HIGH);
          warte = millis();
          statusMsg = "Uff viel zu heiß hier! Ich kühl mal lieber runter."; 
          isPublished = 3;
          Serial.println("Komp an");   
        }
        else
        { 
          if(relays[0].state == 0 && isPublished != 3)
          {
              statusMsg = "Ich steh in den Startlöchern.. Gleich mach ich euch zur Frostbeule!";  
              isPublished = 3;
          }
        }
     }
     else 
     {
        if(relays[0].state == 1 && millis() > warte + interval)
        {
            schalt(relays[0], LOW);
            warte = millis();
            statusMsg = "Seeeehr sehr angenehm hier drin. Zeit für eine Verschnaufpause!";
            Serial.println("Komp aus");
            isPublished = 4;
        }
        if(relays[0].state == 0 && isPublished !=4)
        {
          statusMsg = "Seeeehr sehr angenehm hier drin. Zeit für eine Verschnaufpause!";
          isPublished = 4; 
        }
     } 

    if (relays[0].state == 1 && relays[1].state == 0 && Thermometers[0].temp <= (Thermometers[2].temp-10))
    {
        schalt(relays[1], HIGH);     
        Serial.println("Lüfter an");   
    }
    if (relays[0].state == 0 && relays[1].state == 1 && Thermometers[0].temp >= (Thermometers[2].temp-5))
    {
        schalt(relays[1], LOW); 
        Serial.println("Lüfter aus");          
    }
}

//An-/Ausschaltfunktion mit MQTT
void schalt(Consumer& relay, int state)
{
    if(relay.name == "hcswitch")
    {
        if(relays[0].state == 0)
        {     
          relay.state = state;
          digitalWrite(relay.pin, relay.state);
          publishMsg(relay.name, relay.state);
          Serial.println("Schalte");          
        }
        else
        {
            statusMsg = "Obacht! HCSwitch nur wenn der Kompressor aus ist.";
        }
    } 
    else
    {   
        relay.state = state;
        digitalWrite(relay.pin, relay.state);
        publishMsg(relay.name, relay.state);
        Serial.println("Schalte");      
    } 
}


void publishMsg(char topic[10], char msg[20])
{
    if (isMqttPublished == 0 && client.connected()) {
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        isMqttPublished = 1;
    }
};

void publishMsg(char topic[10], int value)
{
    if (isMqttPublished == 0 && client.connected()) {
        char msg[7];
        dtostrf(value, 7, 0, msg);
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        isMqttPublished = 1;
    }
};

void publishMsg(char topic[10], double value)
{
    if (isMqttPublished == 0 && client.connected()) {
        char msg[7];
        dtostrf(value, 7, 1, msg);
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        isMqttPublished = 1;
    }
};
