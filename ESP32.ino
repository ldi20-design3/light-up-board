/*
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
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
const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
// Define Ultrasonic
const int trigPin = 4;
const int echoPin = 5;
long duration;
int distance;

// Define LED stuff
#define PIN        2
#define NUMPIXELS 30 
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int green = pixels.Color(0, 150, 0);
int red = pixels.Color(150, 0, 0);
int blue = pixels.Color(0, 0, 150);

// Define Buzzer
int speakerPin = 13;

char notes[] = "ccggaagffeeddc "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
int tempo = 300;



//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

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
        if (rxValue.find("R") != -1){
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
        else if (rxValue.find("G") != -1){
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
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
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

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(speakerPin, OUTPUT);

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

int calculateDistance(){ 
  digitalWrite(trigPin, HIGH); // Sets the trigPin on HIGH state for 10 micro seconds
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH); // Reads the echoPin, returns the sound wave travel time in microseconds
  distance= duration*0.006752; //Calculates distance in inches 
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
