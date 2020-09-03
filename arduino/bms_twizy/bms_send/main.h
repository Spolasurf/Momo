#include <SPI.h>
#include <mcp2515.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

struct can_frame canRes;
struct can_frame canMsg;
struct can_frame lastCanMsg;
MCP2515 mcp2515(D8); // Setze CS Pin

int step = 0;
int stepForDefault = 0;

bool received2a = false;
bool received2b = false;
bool received2c = false;
bool received3a = false;
bool received3b = false;
bool received4a = false;

unsigned long lastMillisSend = 0;        // will store last time
unsigned long lastMillisReceived = 0;        // will store last time
unsigned long lastConnectedTry = 0;
unsigned long lastCutOff = 60000;

unsigned long lastPublishedCurrent = 0;
unsigned long lastPublishedPercentage = 0;
unsigned long lastPublishedPower = 0;
unsigned long lastPublishedTemp = 0;
unsigned long lastPublishedStatus = 0;
unsigned long lastPublishedVoltage = 0;
unsigned long lastPublished12V = 0;
unsigned int test = 9999;



WiFiClient espClient;
PubSubClient client(espClient);
char msgMqtt[50];

struct can_frame msg423, msg597, msg426, msg627, msg5D7, msg599, msg436, msg69F;

int lastID423 = 100;
int lastID597 = 100;
int lastID426 = 100;
int lastID627 = 100;
int lastID5D7 = 100;
int lastID599 = 100;
int lastID436 = 100;
int lastID69F = 1000;
int lastStep = 100;
int lastInit = 0;

void checkBMSState();
void sendMessages();


void newInit();

void sendID423();
void sendID597();
void sendID426();
void sendID627();
void sendID5D7();
void sendID599();
void sendID436();
void sendID69F();


void printRawMsg(can_frame msg);

unsigned int twoBytes2int(unsigned char a, unsigned char b);
double threeBytes2double(unsigned char a, unsigned char b, unsigned char c);
