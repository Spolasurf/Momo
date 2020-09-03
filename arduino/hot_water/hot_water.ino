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
long int interval = 600000;
long int warte = -interval;
unsigned long int ersatzDelay = 0;
unsigned long int lastPublishedStatus = 0;
unsigned long int lastPublishedTemps = 0;


char buffer[7];
std::string statusMsg;

typedef struct Thermometer
{
   int temp; //Temperaturwerte
   char name[20]; //MQTT temperaturverzeichnis
 } Thermometer;

//Thermometer initialisieren
Thermometer temp1 = {-9999, "warmwasser/temp1"};
Thermometer temp2 = {-9999, "warmwasser/temp2"};
Thermometer temp3 = {-9999, "warmwasser/temp3"};


//Thermometer in ein Array schreiben
Thermometer Thermometers[3] = {temp1, temp2, temp3};

typedef struct Consumer //angeschlossene Relais
{
   int pin; //PIN
   int state; //Status (an/aus)
   char name[20]; //MQTT-Sende-Verzeichnis
   char name2[20]; //MQTT-Empfangsverzeichniss
 } Consumer;

//Consumer initialisieren
Consumer Relay12V = {D6, LOW, "warmwasser/12V", "warmwasser/12V2"}; //Warmwasser


//Consumer in ein Array schreiben
Consumer relays[1] = {Relay12V};

int soll = 30; //Solltemperatur
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
  
    digitalWrite(D6, LOW);
}

int lastTry = -60000;
int lastTryMqtt = -10000;

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

    if(strcmp(topic, "warmwasser/OnOff") == 0) OnOff = atoi(msg);
    if(strcmp(topic, "warmwasser/solltemp") == 0) soll = atoi(msg);
    return;
}

void reconnect()
{
    if (millis() - lastTryMqtt > 3000 && WiFi.status() == WL_CONNECTED) {
        if (client.connect("arduinoClient4", "Sandy", "Sandy2018")) {
          client.subscribe("warmwasser/OnOff");
          client.subscribe("warmwasser/solltemp");
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



    if (millis() - lastPublishedTemps > 60000)
    {
          Serial.println("Lese Temperaturwerte....");
          lastPublishedTemps = millis();
          for (int i = 0; i < 3; i++)
          {
                sensors.requestTemperatures(); //Thermometer werden ausgelesen...
                Thermometers[i].temp = sensors.getTempCByIndex(i);
                publishMsg(Thermometers[i].name, Thermometers[i].temp);
                Serial.print(Thermometers[i].name);
                Serial.print(": ");
                Serial.println(Thermometers[i].temp);
                  
                  
                if (Thermometers[i].temp == -9999)
                {
                  publishMsg("warmwasser/status", "Thermometer einstecken. BITTE");
                  break;
                } 
           }
           Serial.println("Lese Temperaturwerte Ende.");
    }     

    if(millis()-lastPublishedStatus > 1000) {
      char msg[60];
      strcpy(msg, statusMsg.c_str());
      publishMsg("warmwasser/status", msg);
      lastPublishedStatus  = millis();

      Serial.print("Solltemp: ");
      Serial.println(soll);
    }

    if(OnOff == 1) {
        statusMsg = "Wassertank an.";

        if (soll > Thermometers[0].temp)
         {
            if(millis() > warte + interval && relays[0].state == LOW) 
            {
              schalt(relays[0], HIGH);
              warte = millis();
              Serial.println("Boiler an.");   
            }
         }
         else 
         {
            if(relays[0].state == HIGH && millis() > warte + interval)
            {
                schalt(relays[0], LOW);
                warte = millis();                
                Serial.println("Boiler aus");
            }
         } 
    } else {
        if(relays[0].state == HIGH) {
            schalt(relays[0], 0);
        }
        statusMsg = "Wassertank bereit.";
        warte = -interval;
    }

    client.loop();
}


//An-/Ausschaltfunktion mit MQTT
void schalt(Consumer& relay, int state)
{   
   relay.state = state;
   digitalWrite(relay.pin, relay.state);
   publishMsg(relay.name, relay.state);
   Serial.println("Schalte");      
}


void publishMsg(char topic[10], char msg[20])
{
    if (isMqttPublished == 0 && client.connected()) {
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        //isMqttPublished = 1;
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
        //isMqttPublished = 1;
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
        //isMqttPublished = 1;
    }
};
