#include <Servo.h>
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino_FreeRTOS.h>

// Motor left
// forward pin
const int pinA1 = 11;
// brake pin
const int pinA2 = 10;

// Motor right
// forward pin
const int pinB1 = 6;
// brake pin
const int pinB2 = 5;

//Ultrasonic Sensor
const int trigPin = 2;
const int echoPin = 3;
long front_distance;
long right_distance;
long left_distance;
long duration;

//Servo
Servo carservo;
Servo pickservo;
Servo pawservo;

//light
const int light = 7;
int pr;

//gamestate
#define idle 0
#define automatic 1
#define control 2
int state;

//IR-remote
const int RECV_PIN = 4;
char ch = 0;
IRrecv irrecv(RECV_PIN);
decode_results results;

//LCD
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

//moving state
#define notmoving 0
#define forwardmoving 1
#define leftmoving 2
#define rightmoving 3
#define backwardmoving 4
#define spinmoving 5
int moving = notmoving;

//task handler
TaskHandle_t Handle1, Handle2, Handle3, Handle4;

void setup() {
  //motor left setup
  pinMode(pinA1, OUTPUT);
  pinMode(pinA2, OUTPUT);
  //motor right setup
  pinMode(pinB1, OUTPUT);
  pinMode(pinB2, OUTPUT);

  //Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //car Servo
  carservo.attach(A0);
  //servo angle
  carservo.write(90);

  //pickservo
  pickservo.attach(A3);
  pickservo.write(180);

  //pawservo
  pawservo.attach(A2);
  pawservo.write(0);

  //light
  pinMode(A1, INPUT);
  pinMode(light, OUTPUT);
  digitalWrite(light, LOW);

  //state
  state = idle;

  //IRremote
  irrecv.enableIRIn();

  //LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);

  //timer interrupt
  cli();
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;
  TCCR0A |= (1 << WGM01);
  TCCR0B |= (1 << CS02) | (1 << CS00);
  OCR0A = 124;
  TIMSK0 |= (1 << OCIE0A);
  sei();

  //RTOS
  xTaskCreate(IRremote, (const portCHAR *)"IRremote", 80, NULL, 1, &Handle1);
  xTaskCreate(displaytask, (const portCHAR *)"displaytask", 80, NULL, 1, &Handle2);
  xTaskCreate(automoving, (const portCHAR *)"automoving", 80, NULL, 1, &Handle3);
  vTaskSuspend(Handle3);
}
void loop() {

}
ISR(TIMER0_COMPA_vect) {
  pr = analogRead(A1);
  if (pr > 800)digitalWrite(light, LOW);
  else digitalWrite(light, HIGH);
  TIFR0 &= ~(1 << OCF0A);
  TCNT0 = 0;
}
void IRremote(void *pvParameters) {
  (void) pvParameters;
  for ( ;; ) {
    if (irrecv.decode(&results)) {
      ch = translateIR();
      if (ch != 0) {
        if (ch == '0') {
          if (state == idle) {
            state = automatic;
            vTaskResume(Handle3);
          }
          else {
            state = idle;
            vTaskSuspend(Handle3);
            brake(1);
          }
        }
        else if (ch == '#' && state == automatic) {
          state = control;
          vTaskSuspend(Handle3);
          brake(1);
        }
        else if (ch == '*' && state == control) {
          vTaskResume(Handle3);
          state = automatic;
          brake(1);
        }
        else if (ch == '2' && state == control) {
          forward(2);
        }
        else if (ch == '4' && state == control) {
          turnLeft(1);
          forward(1);
        }
        else if (ch == '5') {
          brake(1);
          picksomething();
        }
        else if (ch == '6' && state == control) {
          turnRight(2);
          forward(1);
        }
        else if (ch == '8' && state == control) {
          brake(1);
          backward(2);
        }
        //vTaskResume(Handle2);
      }
      irrecv.resume();
    }
  }
}
void displaytask(void *pvParameters) {
  (void) pvParameters;
  for ( ;; ) {
    if (state == idle) {
      lcd.setCursor(0, 0);
      lcd.print("Off Mode        ");
      moving = notmoving;
    }
    else if (state == control) {
      lcd.setCursor(0, 0);
      lcd.print("Control Mode    ");
    }
    else if (state == automatic) {
      lcd.setCursor(0, 0);
      lcd.print("Automatic Mode  ");
    }

    if (moving == notmoving) {
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    else if (moving == forwardmoving) {
      lcd.setCursor(0, 1);
      lcd.print("forward         ");
    }
    else if (moving == backwardmoving) {
      lcd.setCursor(0, 1);
      lcd.print("backward        ");
    }
    else if (moving == leftmoving) {
      lcd.setCursor(0, 1);
      lcd.print("turn left       ");
    }
    else if (moving == rightmoving) {
      lcd.setCursor(0, 1);
      lcd.print("turn right      ");
    }
    else if (moving == spinmoving) {
      lcd.setCursor(0, 1);
      lcd.print("spinning        ");
    }
    //vTaskSuspend(Handle2);
  }
}
void automoving(void *pvParameters) {
  (void) pvParameters;
  for ( ;; ) {
    carservo.write(90);
    //vTaskDelay(500 / portTICK_PERIOD_MS);
    front_distance = detectDistance();
    if (front_distance < 20) {
      brake(1);
      backward(2);
      brake(1);
      carservo.write(120);
      right_distance = detectDistance();
      vTaskDelay(500 / portTICK_PERIOD_MS);
      carservo.write(30);
      left_distance = detectDistance();
      if (right_distance < 20 && left_distance < 20)spinRight(0.7);
      else if (right_distance > left_distance) {
        turnRight(2);
        brake(1);
      }
      else {
        turnLeft(2);
        brake(1);
      }
    }
    else {
      forward(0);
    }
  }
}
void picksomething() {
  pickservo.write(0);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  pawservo.write(60);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  pickservo.write(160);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  pawservo.write(0);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
int detectDistance() {
  digitalWrite(trigPin, LOW);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  return (duration * 0.034 / 2);
}
void forward(int time)
{
  moving = forwardmoving;
  motorAForward();
  motorBForward();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

void backward(int time)
{
  moving = backwardmoving;
  motorABackward();
  motorBBackward();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

void turnLeft(int time)
{
  moving = leftmoving;
  motorABackward();
  motorBForward();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

void turnRight(int time)
{
  moving = rightmoving;
  motorAForward();
  motorBBackward();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

void spinRight(int time)
{
  moving = spinmoving;
  motorAForward();
  motorBCoast();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}
void coast(int time)
{
  motorACoast();
  motorBCoast();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

void brake(int time)
{
  motorABrake();
  motorBBrake();
  vTaskDelay(time * 100 / portTICK_PERIOD_MS);
}

//motor A controls
void motorAForward()
{
  digitalWrite(pinA1, HIGH);
  digitalWrite(pinA2, LOW);
}

void motorABackward()
{
  digitalWrite(pinA1, LOW);
  digitalWrite(pinA2, HIGH);
}

//motor B controls
void motorBForward()
{
  digitalWrite(pinB1, HIGH);
  digitalWrite(pinB2, LOW);
}

void motorBBackward()
{
  digitalWrite(pinB1, LOW);
  digitalWrite(pinB2, HIGH);
}

//coasting and braking
void motorACoast()
{
  digitalWrite(pinA1, LOW);
  digitalWrite(pinA2, LOW);
}

void motorABrake()
{
  digitalWrite(pinA1, HIGH);
  digitalWrite(pinA2, HIGH);
}

void motorBCoast()
{
  digitalWrite(pinB1, LOW);
  digitalWrite(pinB2, LOW);
}

void motorBBrake()
{
  digitalWrite(pinB1, HIGH);
  digitalWrite(pinB2, HIGH);
}

char translateIR() {
  char ch = 0;
  switch (results.value) {
    case 0xFF6897:
      //Serial.println(" 0              ");
      ch = '0';
      break;
    case 0xC101E57B:
      //Serial.println(" 0              ");
      ch = '0';
      break;
    case 0xFF9867:
      //Serial.println(" 100+           ");
      ch = '*';
      break;
    case 0x97483BFB:
      //Serial.println(" 100+           ");
      ch = '*';
      break;
    case 0xFFB04F:
      //Serial.println(" 200+           ");
      ch = '#';
      break;
    case 0xF0C41643:
      //Serial.println(" 200+           ");
      ch = '#';
      break;
    case 0xFF30CF:
      //Serial.println(" 1              ");
      ch = '1';
      break;
    case 0x9716BE3F:
      //Serial.println(" 1              ");
      ch = '1';
      break;
    case 0xFF18E7:
      //Serial.println(" 2              ");
      ch = '2';
      break;
    case 0x3D9AE3F7:
      //Serial.println(" 2              ");
      ch = '2';
      break;
    case 0xFF7A85:
      //Serial.println(" 3              ");
      ch = '3';
      break;
    case 0x6182021B:
      //Serial.println(" 3              ");
      ch = '3';
      break;
    case 0xFF10EF:
      //Serial.println(" 4              ");
      ch = '4';
      break;
    case 0x8C22657B:
      //Serial.println(" 4              ");
      ch = '4';
      break;
    case 0xFF38C7:
      //Serial.println(" 5              ");
      ch = '5';
      break;
    case 0x488F3CBB:
      //Serial.println(" 5              ");
      ch = '5';
      break;
    case 0xFF5AA5:
      //Serial.println(" 6              ");
      ch = '6';
      break;
    case 0x449E79F:
      //Serial.println(" 6              ");
      ch = '6';
      break;
    case 0xFF42BD:
      //Serial.println(" 7              ");
      ch = '7';
      break;
    case 0x32C6FDF7:
      //Serial.println(" 7              ");
      ch = '7';
      break;
    case 0xFF4AB5:
      //Serial.println(" 8              ");
      ch = '8';
      break;
    case 0x1BC0157B:
      //Serial.println(" 8              ");
      ch = '8';
      break;
    case 0xFF52AD:
      //Serial.println(" 9              ");
      ch = '9';
      break;
    case 0x3EC3FC1B:
      //Serial.println(" 9              ");
      ch = '9';
      break;
  }
  if (ch == 0)    return 0;
  else            return ch;
  vTaskDelay(5 * 100 / portTICK_PERIOD_MS);
}


