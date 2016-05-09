/**
 * An Mirf example which copies back the data it recives.
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
 */

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define PAYLOAD_SIZE 16
#define MASTER_ADDR "m1"

typedef byte PayloadType[PAYLOAD_SIZE];

void setup(){
  Serial.begin(9600);
  
  /* Set the SPI Driver. */
  Mirf.spi = &MirfHardwareSpi;
  
  /* Setup pins / SPI. */
  Mirf.init();
  
  /* Configure reciving address. */
  Mirf.setRADDR((byte *)"cli1");
  Mirf.payload = PAYLOAD_SIZE;
  
  /* Write channel and payload config then power up reciver. */
  Mirf.config();
  
  Serial.println("Listening..."); 
  /*
  PayloadType payload;
  payload[0] = 'a';
  payload[1] = 'b';
  payload[2] = 0;
  Mirf.send((byte*)&payload);
  while(Mirf.isSending());
  */
}

void loop(){
   
  byte data[Mirf.payload];
  
  if(!Mirf.isSending() && Mirf.dataReady()){
    Serial.println("Got packet");
    
    /* Get load the packet into the buffer */
    Mirf.getData(data);
    
    /* Set the send address. */ 
    Mirf.setTADDR((byte *)MASTER_ADDR);
    
    /* Send the data to the server */
    Mirf.send(data);
    
    /*
     * Wait untill sending has finished
     * NB: isSending returns the chip to receving after returning true.
    */  
    Serial.println("Reply sent.");
  }
}
