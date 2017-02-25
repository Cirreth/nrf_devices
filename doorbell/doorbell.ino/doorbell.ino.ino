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

/**
 * Периферия:
 *   1. nrf24l01
 *   2. Сенсорная кнопка дверного звонка 
 *   3. Лампа системы проверки наличия содержимого
 *   4. Приемник системы проверки наличия содержимого
 *   5. Сервопривод замка
 *   6. Индикация состояния звонка
 * ----------
 * Коммуникация:
 *   1. Отправка сообщения при нажатии на кнопку звонка
 *   2. Ответ на за
 */

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <Servo.h>

#define MAILBOX_PIN 2
#define DOORBELL_BTN 3
#define DOORBELL_LED1 10
#define DOORBELL_LOCK_TIMEOUT 7500


#define PAYLOAD_SIZE 16
#define MASTER_ADDR "serv"

typedef byte PayloadType[PAYLOAD_SIZE];

Servo doorlockServo;
const int DOORLOCK_SERVO_LOCKED_POS = 40;
const int DOORLOCK_SERVO_UNLOCKED_POS = 90;

void setup(){
  Serial.begin(9600);

  Serial.println("Test serial");
  
  /* Set the SPI Driver. */
  Mirf.spi = &MirfHardwareSpi;
  
  /* Setup pins / SPI. */
  Mirf.init();
  
  /* Configure reciving address. */
  Mirf.setRADDR((byte *)"mbox");
  Mirf.setTADDR((byte*)MASTER_ADDR);
  Mirf.payload = PAYLOAD_SIZE;
  
  /* Write channel and payload config then power up reciver. */
  Mirf.config();

  pinMode(MAILBOX_PIN, INPUT_PULLUP);
  /**/
  pinMode(DOORBELL_BTN, INPUT_PULLUP);
  pinMode(DOORBELL_LED1, OUTPUT);
  digitalWrite(DOORBELL_LED1, LOW);
  attachInterrupt(digitalPinToInterrupt(DOORBELL_BTN), doorbellInterrupt, FALLING );

  lockMailBox();
  Serial.println("V0.5 Listening..."); 
}

volatile bool doorbellTriggered = false;
unsigned long doorbellTimeout = 0;
PayloadType payload;

void loop(){
  
  if (doorbellTriggered == true) {
    digitalWrite(DOORBELL_LED1, HIGH);
    doorbellTimeout = millis();
    doorbellTriggered = false;
    Serial.println("Interrupt");
    sendDoorbell();
  }

  if (doorbellTimeout > 0 and millis() - doorbellTimeout > DOORBELL_LOCK_TIMEOUT) {
    doorbellTimeout = 0;
    digitalWrite(DOORBELL_LED1, LOW); 
  }
  
  if(!Mirf.isSending() && Mirf.dataReady()){
    Serial.println("Got packet");
    
    /* Get load the packet into the buffer */
    Mirf.getData((byte*)&payload);
    Serial.println((char*)&payload);
    Mirf.setTADDR((byte*)MASTER_ADDR);

    performCommand((char*)&payload);

  }

}

void performCommand(char* command) {
  if (strcmp(command, "lock") == 0) {
    Serial.println("Lock recognized");
    lockMailBox();
    sendComplete();
  } else if (strcmp(command, "unlock") == 0) {
    Serial.println("Unlock recognized");
    unlockMailBox();
    sendComplete();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
    Mirf.setTADDR((byte*)MASTER_ADDR);
    Mirf.send((byte*)&payload);
  }
}

bool readMailboxState() {
  return digitalRead(MAILBOX_PIN)== HIGH; 
}

void sendMailboxState(bool state) {
  payload[0] = 'm';
  payload[1] = 'b';
  payload[2] = state; 
  payload[3] = 0;
  Mirf.send((byte*)&payload);
}

void sendDoorbell() {
  payload[0] = 'd'; payload[1] = 'b'; payload[2] = 0;
  Mirf.send((byte*)&payload);
  while(Mirf.isSending());
}

void doorbellInterrupt() {
  if (doorbellTimeout == 0) {
    doorbellTriggered = true;
  }
}

void unlockMailBox() {
 doorlockServo.write(DOORLOCK_SERVO_UNLOCKED_POS);
 sendComplete();
 digitalWrite(10, HIGH);
}

void lockMailBox() {
 doorlockServo.write(DOORLOCK_SERVO_LOCKED_POS);
 sendComplete();
 digitalWrite(10, LOW);
}

void sendComplete() {
  //byte[] data = "ok";
  payload[0] = 'o';
  payload[1] = 'k';
  payload[2] = 0;
  Mirf.send((byte*)&payload);
}


