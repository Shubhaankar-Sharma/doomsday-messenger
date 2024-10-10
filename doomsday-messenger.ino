#include <SPI.h>
#include <LoRa.h>
#include <BluetoothSerial.h>

#define ss 5
#define rst 14
#define dio0 2

BluetoothSerial SerialBT;

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 200;          // interval between sends
byte address = 0x01;

String readBtMsg() {
  String text = SerialBT.readStringUntil('\n');
  text.remove(text.length()-1,1);
  return text;
}

void sendBtMsg(String msg) {
  uint8_t buf[msg.length()];
  memcpy(buf,msg.c_str(),msg.length());
  SerialBT.write(buf, msg.length());
  SerialBT.println();
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //868E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  SerialBT.begin("Doomsday-messenger-1");
}

void loop() {
  if (SerialBT.available()) {
    String inputStr = readBtMsg();
    inputStr.trim();
    if (inputStr.length() > 0) {
      if (millis() - lastSendTime > interval) {
        Serial.println(inputStr.length());    
        sendMessage(inputStr);
        Serial.println("Sending: " + inputStr);
        lastSendTime = millis();
      } else {
        Serial.println("rate limited");
      }
    }
  }
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(address);                  // add message sender
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket(true);                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize){
  if (packetSize == 0) return;          // if there's no packet, return
  byte senderAddr = LoRa.read();
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    Serial.println(incoming);
    return;                             // skip rest of function
  }

  Serial.println("Recieved Message");
  Serial.print("Recieved from: ");
  Serial.print(senderAddr, HEX);
  Serial.print("\n");
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  sendBtMsg("Message Recieved: \n Sender: " + String(senderAddr) + "\n" + incoming);
}
