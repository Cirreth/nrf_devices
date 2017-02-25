/**
 * Периферия:
 *   1. Serial
 *   2. Сенсорная кнопка дверного звонка 
 *   3. Лампа системы проверки наличия содержимого
 *   4. Приемник системы проверки наличия содержимого
 *   5. Сервопривод замка
 *   6. Индикация состояния звонка
 * ----------
 * Запросы и ответы doorbell
 *   1. Прерывание при нажатии на кнопку звонка
 *      * отправка сообщения dbt в serial
 *      * ожидание ответа ok из serial
 *      * повторная отправка с интервалом 0.5с. до тех пор, пока не будет получен ok
 *   2. 
 */
#include <Servo.h>
#include <CapacitiveSensor.h>

#define MAILBOX_PIN 2
#define DOORBELL_PIN_1 4
#define DOORBELL_PIN_2 2
#define CONTENT_SENSOR 15
#define CONTENT_SENSOR_LED 8
#define DOORBELL_LED1 11
#define ONBOARD_LED 13
#define DOORBELL_LOCK_TIMEOUT 15000

#define DOORBELL_BLINK_COUNT 16
#define DOORBELL_AMBIENT_LEVEL 30

// sensor button
// 10M resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and foil
CapacitiveSensor doorbellBtnCapacitySensor = CapacitiveSensor(DOORBELL_PIN_1, DOORBELL_PIN_2);
unsigned long csSum;
// -- sensor button end --

Servo doorlockServo;
const int DOORLOCK_SERVO_LOCKED_POS = 80;
const int DOORLOCK_SERVO_UNLOCKED_POS = 180;

void setup(){
  Serial.begin(9600);
  Serial.setTimeout(50);
  
  doorlockServo.attach(9);
  pinMode(MAILBOX_PIN, INPUT_PULLUP);
  /**/
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(CONTENT_SENSOR_LED, OUTPUT);
  pinMode(CONTENT_SENSOR, INPUT);
  
  digitalWrite(CONTENT_SENSOR_LED, HIGH);
  analogWrite(DOORBELL_LED1, DOORBELL_AMBIENT_LEVEL);

  lockMailBox();
}

volatile bool doorbellTriggered = false;
unsigned long doorbellTimeout = 0;
int contentSensorValue = 0;

void loop(){
  contentSensorValue = analogRead(CONTENT_SENSOR);
  
  if (doorbellTriggered == true) {
    analogWrite(DOORBELL_LED1, 255);
    doorbellTimeout = millis();
    doorbellTriggered = false;
    sendDoorbell();
  }

  if (doorbellTimeout > 0) {
    if (millis() - doorbellTimeout > DOORBELL_LOCK_TIMEOUT) {
      doorbellTimeout = 0;
      analogWrite(DOORBELL_LED1, DOORBELL_AMBIENT_LEVEL); 
    } else {
      int doorbell_led_level = abs(-15 + DOORBELL_BLINK_COUNT * 30 * ((millis() - doorbellTimeout) % (DOORBELL_LOCK_TIMEOUT/DOORBELL_BLINK_COUNT)) / DOORBELL_LOCK_TIMEOUT);
      doorbell_led_level = 30 + doorbell_led_level * doorbell_led_level;
      Serial.print(doorbell_led_level);
      Serial.print('\t');
      Serial.println(abs(doorbell_led_level));
      analogWrite(DOORBELL_LED1, doorbell_led_level);
    }
  }

  if (doorbellTimeout == 0) {
      long cs =  doorbellBtnCapacitySensor.capacitiveSensor(80);
      if (cs > 100) {
        csSum += cs;
        if (csSum >= 1000) //c: This value is the threshold, a High value means it takes longer to trigger
        {
          doorbellTriggered = true;
          if (csSum > 0) { csSum = 0; } //Reset
          doorbellBtnCapacitySensor.reset_CS_AutoCal(); //Stops readings
        }
      } else {
        csSum = 0; //Timeout caused by bad readings
      }
  }

  if (!Serial.available()) return;
  
  String command = Serial.readStringUntil('\n');

  trimString(command);
  // странности с обработкой в функции
  if (command.equals("")) return;
  if (command.equals("ready")) {
    serialFlush();
    sendComplete();
  } else if (command.equals("lock")) {
    lockMailBox();
  } else if (command.equals("unlock")) {
    unlockMailBox();
  } else if (command.equals("test")) {
    test();
  } else if (command.equals("content")) {
    sendHasContent();
  } else if (command.equals("contentval")) {
    sendContentSensorValue();
  } else {
    sendFailed("unknown command: " + command);
  }
  //
}

void sendHasContent() {
  Serial.print("ok:");
  Serial.println((analogRead(CONTENT_SENSOR) < 60) ? "1" : "0");
}

void sendContentSensorValue() {
  Serial.print("ok:");
  Serial.println(contentSensorValue);
}

void sendDoorbell() {
  Serial.print("db");
}

bool test() {
  digitalWrite(ONBOARD_LED, HIGH);
  delay(500);
  digitalWrite(ONBOARD_LED, LOW);
  sendComplete();
}

bool readMailboxState() {
  return digitalRead(MAILBOX_PIN)== HIGH; 
}

void sendMailboxState(bool state) {
  Serial.println("mb:" + state ? "1" : 0);
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
  Serial.println("ok");
}

void sendFailed(String reason) {
  Serial.println("failed:" + reason);
}

void trimString(String &request) {
  request.replace("\r", "");
  request.trim();
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}
