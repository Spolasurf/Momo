#include <SPI.h>
#include <mcp2515.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

  
const char* SSID = "openplotter";
const char* PSK = "Sandy2018";
const char* MQTT_BROKER = "192.168.1.133";


WiFiClient espClient;
PubSubClient client(espClient);

struct can_frame canRes;
struct can_frame canMsg;
struct can_frame lastCanMsg;

struct SCanBusMsg {
  int ID = 0;
  unsigned long lastSend = 0;
  };

SCanBusMsg canBusMessages[30];

MCP2515 mcp2515(D8); // Setze CS Pin

int step = 0;
int stepForDefault = 0;

unsigned long lastMillisSend = 0;        // will store last time
unsigned long lastMillisReceived = 0;        // will store last time
unsigned long lastConnectedTry = 0;
unsigned long lastCutOff = 60000;
unsigned long lastMsgReceived = 0;

unsigned long lastPublishedGang = 0;
unsigned long lastPublishedPedal = 0;
unsigned long lastPublishedTemp = 0;
unsigned long lastPublishedStatus = 0;
unsigned long lastPublishedVoltage = 0;
unsigned long lastPublished12V = 0;

unsigned long int lastTry = -60000;
unsigned long int lastTryMqtt = -10000;
unsigned long int lastTryCan = 0;

int batteryPercentage = 0;
int maxInPower = 0;
int maxOutPower = 0;
int stateOfHealth = 0;
int chargePower = 0;
int ladeStatus = 0;
double currentPower = 0;
int maxTemp = 0;
int maxT = 0;
double voltage = 0.;

long lastMsg = 0;
char msg[50];
unsigned long lastStatus = 0;
String errorMsg;
char errorMsgChar[50];

bool publishMsg(char topic[10], char msg[100]);
bool publishMsg(char topic[10], int value);
bool publishMsg(char topic[10], double value);

int isMqttPublished = 0;

int mainRelay = 0;
int dcdcRelay = 0;
int sevconRelay = 0;

int bmsStatus = 0;

void receiveMessages();

void read12Voltage();

void setup_wifi();
void setupCan();
void reconnect();

void printRawMsg(can_frame msg);
void decodeMsg(can_frame msg);
void switchRelays();


void callback(char* topic, byte* payload, unsigned int length);

unsigned int twoBytes2int(unsigned char a, unsigned char b);
double threeBytes2double(unsigned char a, unsigned char b, unsigned char c);


bool received2a = false;
bool received2b = false;
bool received2c = false;
bool received3a = false;
bool received3b = false;
bool received4a = false;

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
