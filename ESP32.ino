/*
  This program takes string input over Bluetooth to power an LED strip, play a
  jingle, and turn off once the ultrasonic sensor is tripped.
  Copyright (C) 2020 Sean Brandabur, Maxwell Walters, Jake Way

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// Bluetooth Libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// LED Library
#include <Adafruit_NeoPixel.h>

BLECharacteristic *pCharacteristic;
BLECharacteristicCallbacks *pCharacteristicCallbacks;
bool deviceConnected = false;
float txValue = 0;
long duration;
int distance;

// Define pins
#define READPIN 32
#define TRIGPIN 4
#define ECHOPIN 5
#define LEDPIN     2
#define SPEAKERPIN 13

// Define LED objects
#define NUMPIXELS 30
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);
int green = pixels.Color(0, 150, 0);
int red = pixels.Color(150, 0, 0);
int blue = pixels.Color(0, 0, 150);

// Define Buzzer
char notes[] = "ccggaagffeeddc "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
int tempo = 300;

// Bluetooth IDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Checks Bluetooth device connection
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// Take input from Bluetooth device to control LED colors
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");

      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }

      // Do stuff based on the command received from the app
      if (rxValue.find("R") != -1) {
        Serial.print("Turning RED");
        for (int i = 0; i < NUMPIXELS; i++){ 
          pixels.setPixelColor(i, red);
        }
        pixels.show(); // Send the updated pixel colors to the hardware.
      }
      else if (rxValue.find("B") != -1) {
        Serial.print("Turning BLUE");
        for (int i = 0; i < NUMPIXELS; i++){ 
          pixels.setPixelColor(i, blue);
        }
        pixels.show(); // Send the updated pixel colors to the hardware.
      }
      else if (rxValue.find("G") != -1) {
        Serial.print("Turning Green");
        for (int i = 0; i < NUMPIXELS; i++){ 
          pixels.setPixelColor(i, green);
        }
        pixels.show(); // Send the updated pixel colors to the hardware.
      }

      Serial.println();
      Serial.println("*********");
    }
  }
};

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(SPEAKERPIN, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SPEAKERPIN, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pixels.begin(); // INITIALIZE NeoPixel strip object
  
  // INITIALIZE ultrasonic sensor pins
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  // INITIALIZE Piezo buzzer
  pinMode(SPEAKERPIN, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("Ring Light Control"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

int calculateDistance() { 
  digitalWrite(TRIGPIN, HIGH); // Sets the TRIGPIN on HIGH state for 10 micro seconds
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH); // Reads the ECHOPIN, returns the sound wave travel time in microseconds
  distance = duration * 0.006752; //Calculates distance in inches 
  return distance;
}

void loop() {
  distance = calculateDistance();
  if (deviceConnected) {
    Serial.println(distance);
    if (distance <= 5) {
      pixels.clear();
      pixels.show();
      for (int i = 0; i < 15; i++) {
        if (notes[i] == ' ') {
          delay(beats[i] * tempo); // rest
        }
        else {
          playNote(notes[i], beats[i] * tempo);
        }

        // pause between notes
        delay(tempo / 2);
      }
    }
    pixels.clear();
  }
}
