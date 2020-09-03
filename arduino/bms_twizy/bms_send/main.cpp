/*
TODO!
-> 2A vs 24.
-> Bei stockender Initialisierung neu versuchen


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

#include "main.h"

// SETUP

void setup() {
	// set serial monitor baud rate and begin SPI protocoll
	// for communication with CAN adapter
	Serial.begin(1000000);

	SPI.begin();

	// set baud rate, frequency and mode for CAN communication
	mcp2515.reset();
	mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
	mcp2515.setNormalMode();
	Serial.println("CAN initialisiert");
}

// MAIN LOOP

void loop() {

	checkBMSState();
	sendMessages();

  if(millis() - lastMillisReceived > 1000) //Länger als 5 Sekunden keine Nachricht mehr empfangen
  {
  		Serial.println("Keine Nachrichten mehr erhalten!");
	  	newInit();
	  	lastMillisReceived = millis();
  }
}

// METHODS

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

	if(step < 3 && millis()- lastInit > 5000){
		newInit();
	}

	if(millis()-lastStep >= 100)
	{
		Serial.println(step);
		lastStep = millis();
	}
	if(canRes.can_id == 0x0155 || canRes.can_id == 0x0424 || canRes.can_id == 0x0425) {
		lastMillisReceived = millis();
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
			case 0: //	00 00 FF FF 00 E0 00 00
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

			default: //03 00 FF FF 00 E0 00 24
				msg423.data[7] = 0x24;

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
			msg597.data[2] = 0x09;
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
	delay(10000);
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

// HELPERS

void printRawMsg(can_frame msg) {
	Serial.print("ID: ");
	Serial.print(msg.can_id, HEX); // print ID
	Serial.print(" Size: ");
	Serial.print(msg.can_dlc, HEX); // print DLC (Länge des Nachrichteninhaltes)
	Serial.print(" Data: ");
	for (int i = 0; i < msg.can_dlc; i++)  {  // print the data
		Serial.print(msg.data[i], HEX);
		Serial.print(" ");
	}
	Serial.println("");
}



unsigned int twoBytes2int(unsigned char a, unsigned char b) {
	return int((unsigned char)a << 8 | (unsigned char)b);
}

double threeBytes2double(unsigned char a, unsigned char b, unsigned char c) {
	return double((unsigned char)c | (unsigned char)b << 8 | (unsigned char)a << 16);
}
