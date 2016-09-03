/**
 * A Mirf example to test the latency between two Ardunio.
 *
 * Pins:
 * Hardware SPI:
 * MISO -> 12
 * MOSI -> 11
 * SCK -> 13
 *
 * Configurable:
 * CE -> 8
 * CSN -> 7
 *
 * Note: To see best case latency comment out all Serial.println
 * statements not displaying the result and load 
 * 'ping_server_interupt' on the server.
 */

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define PAYLOAD_SIZE 16

typedef byte PayloadType[PAYLOAD_SIZE];

String serialInputString = "";         // a string to hold incoming data
boolean serialStringComplete = false;  // whether the string is complete
unsigned long timeoutTime = 0;
PayloadType payload;

void setup(){
  Serial.begin(9600);
  /*
  Mirf.cePin = 7;
  Mirf.csnPin = 8;
  */
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
   
  Mirf.setRADDR((byte *)"serv");
  Mirf.payload = PAYLOAD_SIZE; 
  Mirf.config();
  
  Serial.println("Ready"); 
}

void loop(){

  timeoutTime = millis();

  /* timeoutTimer == 0 - система не ожидает ответ от клиента */
  if (serialStringComplete) {
    if (serialInputString.startsWith("send")) {
          String addr = serialInputString.substring(4,8);
          String message = serialInputString.substring(8);
          /**/
          
          /**/
          Mirf.setTADDR((byte*)addr.c_str());
          Mirf.send((byte*)message.c_str());
          while(Mirf.isSending());
          payload[0] = 0;
          /*end*/
    } else {
      Serial.println("Unknown command: " + serialInputString);
    }
    serialStringComplete = false;
    serialInputString = "";
  }

  delay(10);
  while(!Mirf.dataReady()){
    if ( ( millis() - timeoutTime ) > 1000 ) {
      return;
    }
  }

  Mirf.getData((byte*)&payload);
  Serial.print("Received: ");
  Serial.println((char*)&payload);

}
  
/**
 * https://www.arduino.cc/en/Tutorial/SerialEvent
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the serialInputString:
    serialInputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\0' || inChar == '\n') {
      serialStringComplete = true;
    }
  }
}
