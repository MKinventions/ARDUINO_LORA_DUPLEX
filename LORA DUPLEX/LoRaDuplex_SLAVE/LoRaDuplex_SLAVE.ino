/*
  LoRa Duplex communication

  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Uses readString() from Stream class to read payload. The Stream class'
  timeout may affect other functuons, like the radio's callback. For an

  created 28 April 2017
  by Tom Igoe
*/
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <ArduinoJson.h>
#include "DHT.h"


#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

 
#define LED1 15
#define LED2 22

const int csPin = 5;          // LoRa radio chip select
const int resetPin = 14;       // LoRa radio reset
const int irqPin = 2;         // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 1000;          // interval between sends




  int joyx_state;
  int joyy_state;
  int sw1_state;
  int sw2_state;
  float temp;
  float humi;
void setup() {
  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  dht.begin();

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
}

void loop() {
  //sending data to master node
  sendDataToMaster();
 

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}



void sendDataToMaster(){
String SendJSONData = "";
StaticJsonDocument<1024> doc;


  humi = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
//  float fah = dht.readTemperature(true);
  

  doc["sw1_state"] = sw1_state;
  doc["sw2_state"] = sw2_state;
  doc["joyx_state"] = joyx_state;
  doc["joyy_state"] = joyy_state;
  doc["temp"] = temp;
  doc["humi"] = humi;
  doc["distance"] = (int)random(5) + 30.50;
  doc["volt"] = (double)random(2)+ 3.30;
  // Add an array.
  //
  JsonArray data = doc.createNestedArray("cordinates");
  data.add(48.756080);
  data.add(2.302038);
  
serializeJson(doc, SendJSONData);
//serializeJsonPretty(doc, SendJSONData);

  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime > interval) { //sending every 1 second
    lastSendTime = currentMillis; 
  Serial.println("Sending " + SendJSONData);
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(SendJSONData.length());        // add payload length
  LoRa.print(SendJSONData);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
  }   


}






void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }



StaticJsonDocument<1024> doc2;
  DeserializationError error = deserializeJson(doc2, incoming);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
//  const char* sensor = doc2["sensor"];
//  long time = doc2["time"];
  int joyx = doc2["joyx"];
  int joyy = doc2["joyy"];
  int sw1 = doc2["sw1"];
  int sw2 = doc2["sw2"];
//  latitude = doc2["cordinates"][0];
//  longitude = doc2["cordinates"][1];


if(sw1 == 1){
  sw1_state = 1;
  digitalWrite(LED1, HIGH);
  Serial.println("led1 on");
}else{
  sw1_state = 0;
  digitalWrite(LED1, LOW);
  Serial.println("led1 off");
}

if(sw2 == 1){
  sw2_state = 1;
  digitalWrite(LED2, HIGH);
  Serial.println("led2 on");
}else{
  sw2_state = 0;
  digitalWrite(LED2, LOW);
  Serial.println("led2 off");
}

//sw1_state = (sw1_state == 1)?0:1
//sw2_state = (sw2_state == 1)?0:1
joyx_state = joyx;
joyy_state = joyy;


  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
//  Serial.println("Switch1: " + sw1);
//  Serial.println("Switch2: " + sw2);
//  Serial.println("Joystick_x: " + joyx);
//  Serial.println("Joystick_y: " + joyy);
  Serial.println();



  
}
