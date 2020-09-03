#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D5

#include "main.h"

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
 
void setup()
{
    Serial.begin(1000000);
    delay(10);
    setup_wifi();
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);

    pinMode(D1, OUTPUT); //Setzte Dc/DcWandler
    pinMode(D2, OUTPUT); // Setze Sevcon
    pinMode(D3, OUTPUT); //Setzte Hauptschutz

    digitalWrite(D1, LOW); //DC/DC Wandler
    digitalWrite(D2, LOW); //Sevcon
    digitalWrite(D3, LOW); //Main Relay
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
    Serial.println("test");
    delay(100);

    client.loop();
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
