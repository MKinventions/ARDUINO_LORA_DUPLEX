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


/***********OLED DISPLAY****************/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);





//lora esp32
#define NSS 5
#define RST 14
#define DIO0 2

////arduino
//#define NSS 10
//#define RST 9
//#define DIO0 2

#define joyX 34
#define joyY 35
#define sw1 15
#define sw2 4

String outgoing;              // outgoing message



byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xBB;      // destination to send to
int lastSwitch1State = 0;        // last send time
int lastSwitch2State = 0;        // last send time
unsigned long lastSendTime = 0;
int interval = 1000;          // interval between sends

/******SENDING DATA*****/
int JOYSTICK_X;
int JOYSTICK_Y;
int SWITCH1;
int SWITCH2;


/*****RECEIVED DATA********/
  int joyx_state;
  int joyy_state;
  int sw1_state;
  int sw2_state;
  double temp;
  double humi;
  double distance;
  double voltage;
  double latitude;
  double longitude;



void setup() {
  Serial.begin(115200);                   // initialize serial

   /********OLED DISPLAY************/
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  
  while (!Serial);
  pinMode(joyX, INPUT);
  pinMode(joyY, INPUT);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);


  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
LoRa.setPins(NSS, RST, DIO0);

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");

}

void loop() {
  //sending data to slave node
  sendDataToSlave();


//RECEIVED DATA FROM SLAVE NODE
  onReceive(LoRa.parsePacket());


  
}


void sendDataToSlave(){

String SendJSONData = "";
StaticJsonDocument<1024> doc;

JOYSTICK_X = map(analogRead(joyX), 4095, 0, 1023, 0);
JOYSTICK_Y = map(analogRead(joyY), 4095, 0, 1023, 0);


/***********SWITCH1*******************/
int currentSwitch1State = digitalRead(sw1);
if(currentSwitch1State != lastSwitch1State){
  lastSwitch1State = currentSwitch1State;
  if(lastSwitch1State == 0){
//    SWITCH1 = (SWITCH1 == 0)?1:0;
      SWITCH1 = !SWITCH1;

  }
}

/***********SWITCH2*******************/
int currentSwitch2State = digitalRead(sw2);
if(currentSwitch2State != lastSwitch2State){
  lastSwitch2State = currentSwitch2State;
  if(lastSwitch2State == 0){
//    SWITCH2 = (SWITCH2 == 0)?1:0;
      SWITCH2 = !SWITCH2;
  }
}




  doc["joyx"] = JOYSTICK_X;
  doc["joyy"] = JOYSTICK_Y;
  doc["sw1"] = SWITCH1;
  doc["sw2"] = SWITCH2;
  // Add an array.
  //
  JsonArray data = doc.createNestedArray("data");
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
  if (recipient != localAddress && recipient != 0xBB) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }


  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();


  StaticJsonDocument<1024> doc2;
  DeserializationError error = deserializeJson(doc2, incoming);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* sensor = doc2["sensor"];
  long time = doc2["time"];
  joyx_state = doc2["joyx_state"];
  joyy_state = doc2["joyy_state"];
  sw1_state = doc2["sw1_state"];
  sw2_state = doc2["sw2_state"];
  temp = doc2["temp"];
  humi = doc2["humi"];
  distance = doc2["distance"];
  voltage = doc2["volt"];
  latitude = doc2["cordinates"][0];
  longitude = doc2["cordinates"][1]; 


    display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // Display static text
  display.setCursor(0, 0);//(COL, ROW)
  display.print("RSSI:"); display.println(String(LoRa.packetRssi()));
  display.setCursor(0, 8);//(COL, ROW)
  display.print("TEMP:"); display.println(temp);
  display.setCursor(0, 16);//(COL, ROW)
  display.print("DIST:"); display.println(distance);
  display.setCursor(0, 24);//(COL, ROW)
  display.print("LATI:"); display.println(latitude);
  
  display.setCursor(62, 8);//(COL, ROW)
  display.print("|HUMI:"); display.println(humi);
  display.setCursor(62, 16);//(COL, ROW)
  display.print("|V0LT: "); display.println(voltage);
  display.setCursor(62, 24);//(COL, ROW)
  display.print("|LONG: "); display.println(longitude);
  display.display();

  display.setCursor(0, 32);//(COL, ROW)
  display.print("---------------------");
  display.setCursor(0, 40);//(COL, ROW)
  display.print("JOY-X:"); display.println(joyx_state);
  display.setCursor(0, 48);//(COL, ROW)
  display.print("JOY-Y:"); display.println(joyy_state);
  
  display.setCursor(65, 40);//(COL, ROW)
  display.print("|SW1:"); display.println(sw1_state);
  display.setCursor(65, 48);//(COL, ROW)
  display.print("|SW2:"); display.println(sw2_state);
  display.display();

}
