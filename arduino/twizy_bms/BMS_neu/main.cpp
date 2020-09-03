/*

Programm empfängt auf dem CAN Bus mit 500kbps und 16MhZ.
Sobald eine Nachricht mit der ID = F6 empfangen wird, sendet es eine Nachricht mit der ID = BB zurück.

Pin-Belegung für WeMos D1 zu MCP2515:
VCC -> 5V/vin
GND -> GND
CS -> D8
SO -> D6 (MISO)
SI -> D7 (MOSI)
SCK -> D5 (SCK)
INT -> -

H -> CAN HIGH
L -> CAN LOW

Nicht vergessen: Quarz mit einem 16MhZ Quarz umlöten/tauschen!

TODO: Inverter Nachrichten senden.

Example: https://github.com/dexterbg/Twizy-Virtual-BMS/blob/master/src/TwizyVirtualBMS.h
*/

//Abfrage:
//Wemos eigener Name?
//Kein Delay benutzt?

#include "main.h"

void setup()
{
    Serial.begin(2000000);
    delay(10);
    setup_wifi();
    setupCan();
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);

    pinMode(D1, OUTPUT); //Setzte Dc/DcWandler
    pinMode(D2, OUTPUT); // Setze Sevcon
    pinMode(D3, OUTPUT); //Setzte Hauptschutz

    digitalWrite(D1, LOW); //DC/DC Wandler
    digitalWrite(D2, LOW); //Sevcon
    digitalWrite(D3, LOW); //Main Relay
}

void setup_wifi()
{
    if (millis() - lastTry > 6000) {
        SPI.endTransaction();
        delay(200);
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
        SPI.begin();
    }
}

void setupCan()
{
    SPI.begin();
    // set baud rate, frequency and mode for CAN communication
    mcp2515.reset();
    mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
    mcp2515.setNormalMode();
    Serial.println("CAN initialisiert");
    lastTryCan = millis();
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");
    char msg[length + 1];
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    Serial.println();

    msg[length] = '\0';
    Serial.println(msg);

    if (strcmp(topic, "bms/mainRelay") == 0)
        mainRelay = atoi(msg);
    if (strcmp(topic, "bms/dcdcRelay") == 0)
        dcdcRelay = atoi(msg);
    if (strcmp(topic, "bms/sevconRelay") == 0)
        sevconRelay = atoi(msg);
    lastMsgReceived = millis();
    return;
}

void reconnect()
{
    if (millis() - lastTryMqtt > 3000 && WiFi.status() == WL_CONNECTED) {
        SPI.endTransaction();
        if (client.connect("arduinoClient1", "Sandy", "Sandy2018")) {
            client.subscribe("bms/mainRelay");
            client.subscribe("bms/dcdcRelay");
            client.subscribe("bms/sevconRelay");
        }
        else {
            Serial.println("MQTT nicht verbunden. Versuche Verbindung erneut herzustellen in 3s.");
            lastTryMqtt = millis();
        }
        SPI.begin();
    }
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED) {
         setup_wifi();
    }
    if (!client.connected()) {
        reconnect();
        delay(200);
    }
   // if (millis() - lastMillisReceived > 5000 && millis() - lastTryCan > 5000) {
   //     setupCan();
   // }

    checkBMSState();
    sendMessages();

    switchRelays();
    read12Voltage();
    receiveMessages();

    client.loop();
}

// METHODS

void switchRelays()
{
    if (millis() - lastPublishedStatus > 3000) {
        if (bmsStatus == 1) {
            publishMsg("bms/status", "BMS Bereit.");
            Serial.println("BMS Bereit");
            if (millis() - lastMsgReceived > 600000) {
                mainRelay = 1;
                dcdcRelay = 1;
            }
        }
        else {
            publishMsg("bms/status", errorMsgChar);
            Serial.println(errorMsgChar);
            Serial.println(maxInPower);
            Serial.println(maxOutPower);
            Serial.println(maxTemp);
            Serial.println(lastMillisReceived);
            Serial.println(batteryPercentage);
            Serial.println(maxInPower);

            dcdcRelay = 0;
            sevconRelay = 0;
            delay(100);
            mainRelay = 0;
            
        }
        lastPublishedStatus = millis();
        publishMsg("bms/mainRelay2", mainRelay);
        publishMsg("bms/dcdcRelay2", dcdcRelay);
        publishMsg("bms/sevconRelay2", sevconRelay);
    }

    if (mainRelay == 1 && bmsStatus == 1) {
        digitalWrite(D3, HIGH);
    }
    else {
        digitalWrite(D3, LOW);
    }

    if (dcdcRelay == 1 && bmsStatus == 1) {
        digitalWrite(D1, HIGH);
    }
    else {
        digitalWrite(D1, LOW);
    }

    if (sevconRelay == 1 && bmsStatus == 1) {
        digitalWrite(D2, HIGH);
    }
    else {
        digitalWrite(D2, LOW);
    }
}

void receiveMessages()
{
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK /*&& canMsg.can_id != lastCanMsg.can_id*/) {
        printRawMsg(canMsg);
        decodeMsg(canMsg);
        lastCanMsg = canMsg;
        if (canMsg.can_id == 0x0155 || canMsg.can_id == 0x0424 || canMsg.can_id == 0x0425) {
            lastMillisReceived = millis();
        }
    }

    if(step < 4) { errorMsg = "BMS konnte nicht intiatilisiert werden!";}
    else if(maxInPower < 100) { errorMsg = "MaxInPower kleiner als 100W";}
    else if(maxOutPower < 100) { errorMsg = "MaxOutPower kleiner als 100W";}
    else if(maxTemp > 35) { errorMsg = "Batterietemperatur größer als 35°C";}
    else if(millis() - lastMillisReceived > 1000) { errorMsg = "Länger als 1s keine Nachricht erhalten";}
    else if(batteryPercentage <= 0) { errorMsg = "Ladezustand kleiner gleich 0%";}
    else {
        bmsStatus = 1;
        lastStatus = millis();
    }

    errorMsg.toCharArray(errorMsgChar, 50);

    if(millis() - lastStatus > 600000) {
        bmsStatus = 0; 
    }
}

void read12Voltage()
{
    if (millis() - lastPublished12V > 10000) {
        double sensorValue = analogRead(A0) * 0.065;
        if(publishMsg("bms/12V", sensorValue) == 1) lastPublished12V = millis();
    }
}

// HELPERS

void printRawMsg(can_frame msg)
{
    if (1 == 0) {
        Serial.print("ID: ");
        Serial.print(msg.can_id, HEX); // print ID
        Serial.print(" Size: ");
        Serial.print(msg.can_dlc, HEX); // print DLC (Länge des Nachrichteninhaltes)
        Serial.print(" Data: ");
        for (int i = 0; i < msg.can_dlc; i++) { // print the data
            Serial.print(msg.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println("");
    }

    int isInRegister = 0;
    for (int i = 0; i < 20; i++) {
        if (canBusMessages[i].ID == msg.can_id) {
            isInRegister = 1;
            if (millis() - canBusMessages[i].lastSend > 1000) {
                //char output[100];
                //dtostrf(msg.can_id, 7, 0, output);
                //publishMsg("bms/canMsg", output);
                canBusMessages[i].lastSend = millis();
                Serial.print("ID: ");
                Serial.print(msg.can_id, HEX); // print ID
                Serial.print(" Size: ");
                Serial.print(msg.can_dlc, HEX); // print DLC (Länge des Nachrichteninhaltes)
                Serial.print(" Data: ");
                for (int i = 0; i < msg.can_dlc; i++) { // print the data
                    Serial.print(msg.data[i], HEX);
                    Serial.print(" ");
                }
                Serial.println("");
            }
        }
    }

    if (isInRegister == 0) {
        for (int i = 0; i < 20; i++) {
            if (canBusMessages[i].ID == 0) {
                canBusMessages[i].ID = msg.can_id;
                i = 20;
            }
        }
    }
}

void decodeMsg(can_frame msg)
{
    switch (msg.can_id) {
    case 0x0155:
        batteryPercentage = twoBytes2int(msg.data[4], msg.data[5]) / 400;
        chargePower = msg.data[0] * 300;
        currentPower = msg.data[2] | ((char)(msg.data[1] << 4) >> 4) << 8;
        currentPower = (currentPower - 2000) / 4.;
        currentPower = currentPower;

        if (millis() - lastPublishedPercentage > 10000) {
            publishMsg("bms/percentage", batteryPercentage);
            lastPublishedPercentage = millis();
        }

        if (millis() - lastPublishedCurrent > 1000) {
            publishMsg("bms/currentPower", currentPower);
            lastPublishedCurrent = millis();
        }
        break;

    case 0x0424:
        maxInPower = msg.data[2] * 500;
        maxOutPower = msg.data[3] * 500;
        stateOfHealth = msg.data[5];
        break;

    case 0x055F:
        voltage = msg.data[5] << 4 | msg.data[6] >> 4;
        voltage = voltage / 10.;

        if (millis() - lastPublishedVoltage > 10000) {
            publishMsg("bms/voltage", voltage);
            lastPublishedVoltage = millis();
        }
        break;

    case 0x0554:
        maxT = 0;
        for (int i = 0; i < 7; i++) {
            if ((uint)msg.data[i] > maxT)
                maxT = (uint)msg.data[i];
        }
        maxTemp = maxT - 40;

        if (millis() - lastPublishedTemp > 10000 && maxTemp < 215) {
            publishMsg("bms/temp", maxTemp);
            lastPublishedTemp = millis();
        }
        break;

    case 0x059B:

        if (millis() - lastPublishedPedal > 1000) {
            if(publishMsg("sevcon/pedal", msg.data[3])) lastPublishedPedal = millis();
        }
        if (millis() - lastPublishedGang > 1000) {
            if(publishMsg("sevcon/gang", msg.data[0])) lastPublishedGang = millis();   
        }
        if (millis() - lastPublishedMotorStatus > 1000) {
            if(publishMsg("sevcon/status", msg.data[4])) lastPublishedMotorStatus = millis();   
        }
        break;

    case 0x0196:

        if (millis() - lastPublishedMotorTemp > 10000) {
            if(publishMsg("sevcon/motorTemp", msg.data[5] - 40)) lastPublishedMotorTemp = millis();
        }

        break;

    case 0x059E:

        if (millis() - lastPublishedControllerTemp > 10000) {
            if(publishMsg("sevcon/controllerTemp", msg.data[5] - 40)) lastPublishedControllerTemp = millis();
        }

        break;

    case 0x019F:

        if (millis() - lastPublishedMotorRpm > 1000) {
            int rpm = msg.data[2] << 4 | msg.data[3] >> 4;
            rpm = (rpm - 2000) *10;
            publishMsg("sevcon/rpm", rpm);
            lastPublishedMotorRpm = millis();
        }
        break;
    }
}

unsigned int twoBytes2int(unsigned char a, unsigned char b)
{
    return int((unsigned char)a << 8 | (unsigned char)b);
}

double threeBytes2double(unsigned char a, unsigned char b, unsigned char c)
{
    return double((unsigned char)c | (unsigned char)b << 8 | (unsigned char)a << 16);
}

bool publishMsg(char topic[10], char msg[20])
{
    if (client.connected()) {
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        return 1;
    } else {
        return 0;
    }
};

bool publishMsg(char topic[10], int value)
{
    if (client.connected()) {
        char msg[7];
        dtostrf(value, 7, 0, msg);
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        return 1;
    } else {
        return 0;
    }
};

bool publishMsg(char topic[10], double value)
{
    if (client.connected()) {
        char msg[7];
        dtostrf(value, 7, 1, msg);
        client.publish(topic, msg);
        Serial.print(msg);
        Serial.print(" wurde im Topic ");
        Serial.print(topic);
        Serial.println(" gepublished.");
        return 1;
    } else {
        return 0;
    }
};

void checkBMSState() {
  switch(step) {
    case 0:
      if(stepForDefault > 3) {
        step++;
      }
      break;

    case 1:
      if (mcp2515.readMessage(&canRes) == MCP2515::ERROR_OK) {
        if (canRes.can_id == 0x0155 && (canRes.data[1] >> 4) == 0x0A) {
          received2a = true;
        }
        if (canRes.can_id == 0x0424 && (canRes.data[0] == 0x11 || canRes.data[0] == 0x21)) {
          received2b = true;
        }
        if (canRes.can_id == 0x0425 && canRes.data[0] == 0x24) {
          received2c = true;
        }
        if(received2a == true && received2b == true && received2c == true) {
          step++;
          Serial.println("Empfange 424_0 -> 11, 425_0 -> 24, 155_2 -> A");
        }
      }
      break;


    case 2:
      if (mcp2515.readMessage(&canRes) == MCP2515::ERROR_OK) {
        if (canRes.can_id == 0x0155 && (canRes.data[1] >> 4) == 0x09) {
          received3a = true;
        }
        if (canRes.can_id == 0x0155 && canRes.data[3] == 0x54) {
          received3b = true;
        }
        if(received3a == true && received3b == true) {
          step++;
          Serial.println("Empfange 155_3 -> 54,  155_2 -> 9 ");
        }
      }
      break;


    case 3:
      if (mcp2515.readMessage(&canRes) == MCP2515::ERROR_OK) {
        if (canRes.can_id == 0x0425 && canRes.data[0] == 0x2A) {
          received4a = true;
        }
        if(received4a == true) {
          step++;
          Serial.println("Empfange 425_0 -> 2A");
        }
      }
      break;


    default:
      if (canRes.can_id == 0x0155 && canRes.data[3] != 0x54){
        step = 1;
        Serial.println("Fehler im BMS. Versuche neu zu starten.");
      }
      break;
  }

  if(((millis() - lastStatus > 20000) || (millis()- lastMillisReceived > 5000)) && millis()- lastInit > 5000){
    newInit();
  }

  if(millis()-lastStep >= 100)
  {
    Serial.print("BMSInit Step: ");
    Serial.println(step);
    lastStep = millis();
  }
}

void sendMessages() {
  sendID5D7();
  sendID599();
  sendID436();
  sendID69F();
  sendID423();
  sendID426();
  sendID597();
  sendID627();
}


void sendID423() {
  if(millis() - lastID423 >= 100) {
    switch(step) {
      case 0: //  00 00 FF FF 00 E0 00 00
        msg423.can_id = 0x0423;
        msg423.can_dlc = 8;
        msg423.data[0] = 0x00;
        msg423.data[1] = 0x00;
        msg423.data[2] = 0xFF;
        msg423.data[3] = 0xFF;
        msg423.data[4] = 0x00;
        msg423.data[5] = 0xE0;
        msg423.data[6] = 0x00;
        msg423.data[7] = 0x00;

        mcp2515.sendMessage(&msg423);
        printRawMsg(msg423);
        stepForDefault++;

        break;

      case 1: //03 00 FF FF 00 E0 00 00   
        msg423.data[0] = 0x03;
        msg423.data[7] = 0x2E;

        mcp2515.sendMessage(&msg423);
        printRawMsg(msg423);
        break;

      case 3: //03 00 FF FF 00 E0 00 BD
        msg423.data[0] = 0x03;
        msg423.data[1] = 0x00;
        msg423.data[2] = 0xFF;
        msg423.data[3] = 0xFF;
        msg423.data[4] = 0x00;
        msg423.data[5] = 0xE0;
        msg423.data[6] = 0x00;
        msg423.data[7] = 0xBD;

        mcp2515.sendMessage(&msg423);
        printRawMsg(msg423);
        break;

      default: //03 36 FF FF 00 E0 00 E5
        msg423.data[1] = 0x36;
        msg423.data[7] = 0xE5;

        mcp2515.sendMessage(&msg423);
        printRawMsg(msg423);
        break;
    }
    lastID423 = millis();
  }
}

void sendID597() {
  if(millis() - lastID597 >= 100) {
    switch(step) {
      case 0: //00 F0 1B A2 2F 00 01 32
        msg597.can_id = 0x0597;
        msg597.can_dlc = 8;
        msg597.data[0] = 0x00;
        msg597.data[1] = 0xF0;
        msg597.data[2] = 0x1B;
        msg597.data[3] = 0xA2;
        msg597.data[4] = 0x2F;
        msg597.data[5] = 0x00;
        msg597.data[6] = 0x01;
        msg597.data[7] = 0x32;

        mcp2515.sendMessage(&msg597);
        printRawMsg(msg597);
        break;

      case 1: //00 F0 1B A2 2F 00 01 32
        msg597.can_id = 0x0597;
        msg597.can_dlc = 8;
        msg597.data[0] = 0x00;
        msg597.data[1] = 0xF0;
        msg597.data[2] = 0x1B;
        msg597.data[3] = 0xA2;
        msg597.data[4] = 0x2F;
        msg597.data[5] = 0x00;
        msg597.data[6] = 0x01;
        msg597.data[7] = 0x32;

        mcp2515.sendMessage(&msg597);
        printRawMsg(msg597);
        break;

      case 2: //00 F0 1C A1 2F 00 01 32
        msg597.data[3] = 0xA1;

        mcp2515.sendMessage(&msg597);
        printRawMsg(msg597);
        break;

      case 3: //00 F4 09 C1 2F 00 01 32
      msg597.data[0] = 0x00;
      msg597.data[1] = 0xF4;
      msg597.data[2] = 0x53;
      msg597.data[3] = 0xC1;
      msg597.data[4] = 0x2F;
      msg597.data[5] = 0x00;
      msg597.data[6] = 0x01;
      msg597.data[7] = 0x32;

        mcp2515.sendMessage(&msg597);
        printRawMsg(msg597);
        break;

      default: //00 95 08 41 2F 00 01 32 
        msg597.data[1] = 0x95;
        msg597.data[3] = 0x41;

        mcp2515.sendMessage(&msg597);
        printRawMsg(msg597);
        break;
    }
    lastID597 = millis();
  }
}

void sendID426() {
  if(millis() - lastID426 >= 100) {
    switch(step) {
      case 0: //00 18 02 00 00 00 00
        msg426.can_id = 0x0426;
        msg426.can_dlc = 7;
        msg426.data[0] = 0x00;
        msg426.data[1] = 0x18;
        msg426.data[2] = 0x02;
        msg426.data[3] = 0x00;
        msg426.data[4] = 0x00;
        msg426.data[5] = 0x00;
        msg426.data[6] = 0x00;

        mcp2515.sendMessage(&msg426);
        printRawMsg(msg426);
        break;

      case 1: //00 18 02 00 A4 DF 00
        msg426.data[4] = 0xA4;
        msg426.data[5] = 0xDF;

        mcp2515.sendMessage(&msg426);
        printRawMsg(msg426);
        break;

        case 3: //00 78 01 00 A4 DF 00   
        msg426.data[0] = 0x00;
        msg426.data[1] = 0x78;
        msg426.data[2] = 0x01;
        msg426.data[3] = 0x00;
        msg426.data[4] = 0xA4;
        msg426.data[5] = 0xDF;
        msg426.data[6] = 0x00;

          mcp2515.sendMessage(&msg426);
          printRawMsg(msg426);
          break;

      default:

        mcp2515.sendMessage(&msg426);
        printRawMsg(msg426);
        break;
    }
    lastID426 = millis();
  }
}

void sendID627() {
  if(millis() - lastID627 >= 100) {
    switch(step) {
      case 0: //00 00 00
        msg627.can_id = 0x0627;
        msg627.can_dlc = 3;
        msg627.data[0] = 0x00;
        msg627.data[1] = 0x00;
        msg627.data[2] = 0x00;

        mcp2515.sendMessage(&msg627);
        printRawMsg(msg627);
        break;

      default:

        mcp2515.sendMessage(&msg627);
        printRawMsg(msg627);
        break;
    }
    lastID627 = millis();
  }
}

void sendID5D7() {  
  if(millis() - lastID5D7 >= 100) {
    switch(step) {
      case 0: //00 00 04 06 75 20 00  
        msg5D7.can_id = 0x05D7;
        msg5D7.can_dlc = 7;
        msg5D7.data[0] = 0x00;
        msg5D7.data[1] = 0x00;
        msg5D7.data[2] = 0x04;
        msg5D7.data[3] = 0x06;
        msg5D7.data[4] = 0x76;
        msg5D7.data[5] = 0x80;
        msg5D7.data[6] = 0x00;

        mcp2515.sendMessage(&msg5D7);
        printRawMsg(msg5D7);
        break;

      default:

        mcp2515.sendMessage(&msg5D7);
        printRawMsg(msg5D7);
        break;
    }
    lastID5D7 = millis();
  }
}

void sendID599() {
  if(millis() - lastID599 >= 100) {   
    switch(step) {
      case 0: //00 00 A4 DF FF FF 00 00
        msg599.can_id = 0x0599;
        msg599.can_dlc = 8;
        msg599.data[0] = 0x00;
        msg599.data[1] = 0x00;
        msg599.data[2] = 0xA4;
        msg599.data[3] = 0xDF;
        msg599.data[4] = 0xFF;
        msg599.data[5] = 0x2E;
        msg599.data[6] = 0x00;
        msg599.data[7] = 0x00;

        mcp2515.sendMessage(&msg599);
        printRawMsg(msg599);
        break;

      default:

        mcp2515.sendMessage(&msg599);
        printRawMsg(msg599);
        break;
    }
    lastID599 = millis();
  }
}

void sendID436() {
  if(millis() - lastID436 >= 100) {
    switch(step) {
      case 0: //00 01 78 87 00 00
        msg436.can_id = 0x0436;
        msg436.can_dlc = 6;
        msg436.data[0] = 0x00;
        msg436.data[1] = 0x01;
        msg436.data[2] = 0x79;
        msg436.data[3] = 0x30;
        msg436.data[4] = 0x00;
        msg436.data[5] = 0x00;

        mcp2515.sendMessage(&msg436);
        printRawMsg(msg436);
        break;

      default:

        mcp2515.sendMessage(&msg436);
        printRawMsg(msg436);
        break;
    }
    lastID436 = millis();
  }
}

void sendID69F() {
  if(millis() - lastID69F >= 1000) {
    switch(step) {
      case 0: //F0 82 87 37 //Neue Vin 1 28 33 25 -> F5 23 38 21
        msg69F.can_id = 0x069F;
        msg69F.can_dlc = 4;
        msg69F.data[0] = 0xF5;
        msg69F.data[1] = 0x23;
        msg69F.data[2] = 0x38;
        msg69F.data[3] = 0x21;

        mcp2515.sendMessage(&msg69F);
        printRawMsg(msg69F);
        break;

      default:

        mcp2515.sendMessage(&msg69F);
        printRawMsg(msg69F);
        break;
    }
    lastID69F = millis();
  }
}


void newInit() {
  Serial.println("Initialisiere BMS neu");
  setupCan();
  step = 0;
  stepForDefault = 0;
  received2a = false;
  received2b = false;
  received2c = false;
  received3a = false;
  received3b = false;
  received4a = false;
  lastInit = millis();
}
