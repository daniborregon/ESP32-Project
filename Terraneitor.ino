#include <BluetoothSerial.h>

#define ECHO 34
#define TRIG 18
#define IN1 26
#define IN2 25
#define IN3 33
#define IN4 32
#define LED_POS 22
#define LED_STOP 23
#define LED_IZQ 21
#define LED_DER 19

#define SOUND_SPEED 0.034

const int freq = 5000;
const int resolution = 8;
const int canal_IN1 = 1;
const int canal_IN2 = 2;
const int canal_IN3 = 3;
const int canal_IN4 = 4;

volatile int SPEED = 255;
volatile char comando_motor = 'S';

volatile bool DCHA = false;
volatile bool IZQ = false;
volatile bool ATRAS = false;

TaskHandle_t HandleMotor;
TaskHandle_t HandleBluetooth;
TaskHandle_t HandleSensor;
TaskHandle_t HandleParpadeo;

BluetoothSerial SerialBT;

void forward()
{
  ledcWrite(canal_IN1, SPEED);
  ledcWrite(canal_IN2, 0);
  ledcWrite(canal_IN3, 0);
  ledcWrite(canal_IN4, SPEED);
}
void back()
{
  ledcWrite(canal_IN1, 0);
  ledcWrite(canal_IN2, SPEED);
  ledcWrite(canal_IN3, SPEED);
  ledcWrite(canal_IN4, 0);
}
void right()
{
  ledcWrite(canal_IN1, SPEED);
  ledcWrite(canal_IN2, 0);
  ledcWrite(canal_IN3, 0);
  ledcWrite(canal_IN4, 0);
}
void left()
{
  ledcWrite(canal_IN1, 0);
  ledcWrite(canal_IN2, 0);
  ledcWrite(canal_IN3, 0);
  ledcWrite(canal_IN4, SPEED);
}
void stop()
{
  ledcWrite(canal_IN1, 0);
  ledcWrite(canal_IN2, 0);
  ledcWrite(canal_IN3, 0);
  ledcWrite(canal_IN4, 0);
}

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("ESP32_Robot");

  pinMode(LED_POS,OUTPUT);
  pinMode(LED_STOP,OUTPUT);
  pinMode(LED_IZQ,OUTPUT);
  pinMode(LED_DER,OUTPUT);

  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);

  ledcSetup(canal_IN1,freq,resolution);
  ledcSetup(canal_IN2,freq,resolution);
  ledcSetup(canal_IN3,freq,resolution);
  ledcSetup(canal_IN4,freq,resolution);

  ledcAttachPin(IN1,canal_IN1);
  ledcAttachPin(IN2,canal_IN2);
  ledcAttachPin(IN3,canal_IN3);
  ledcAttachPin(IN4,canal_IN4);

 xTaskCreatePinnedToCore(
    Task_Bluetooth,
    "Bluetooth",
    2048,
    NULL,
    1,
    &HandleBluetooth,
    0
    );

  xTaskCreatePinnedToCore(
    Task_Motor,
    "Control de motores",
    2048,
    NULL,
    0,
    &HandleMotor,
    0
  );

  xTaskCreatePinnedToCore(
    Task_Sensor,
    "Sensor de distancia",
    4096,
    NULL,
    1,
    &HandleSensor,
    1
  );

  xTaskCreatePinnedToCore(
    Task_Parpadeo,
    "Parpadeo",
    4096,
    NULL,
    1,
    &HandleParpadeo,
    1
  );
}

void loop()
{
}

void Task_Parpadeo(void *parameters)
{
  const TickType_t xPeriod = pdMS_TO_TICKS(400);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while(1)
  {
    if(DCHA)
    {
      digitalWrite(LED_DER, !digitalRead(LED_DER));
    }
    if(IZQ)
    {
      digitalWrite(LED_IZQ, !digitalRead(LED_IZQ));
    }
    if(ATRAS)
    {
      digitalWrite(LED_STOP, !digitalRead(LED_STOP));
    }
    vTaskDelayUntil(&xLastWakeTime,xPeriod);
  }
}

void Task_Sensor(void *parameters){

  long duration;
  float distanceCM;

  const TickType_t xPeriod = pdMS_TO_TICKS(100);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while(1)
  {
    digitalWrite(TRIG,HIGH);
    delay(10);
    digitalWrite(TRIG,LOW);

    duration = pulseIn(ECHO,HIGH,30000);

    distanceCM = duration*SOUND_SPEED /2;

    Serial.println(distanceCM);

    if(SerialBT.hasClient()) { // Solo enviar si hay alguien conectado
       SerialBT.println(distanceCM);
    }
    vTaskDelayUntil(&xLastWakeTime,xPeriod);
  }
}
  void Task_Bluetooth(void *parameters)
{
  const TickType_t xPeriod = pdMS_TO_TICKS(10);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(1)
  {
    if(SerialBT.available())
    {
      char c = SerialBT.read();
      if(c=='F' ||c=='B' ||c=='S' ||c=='L' ||c=='R')
      {
        comando_motor = c;
      }
    }
    vTaskDelayUntil(&xLastWakeTime,xPeriod);
  }
}

void Task_Motor(void *parameters)
{
  const TickType_t xPeriod = pdMS_TO_TICKS(10);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  while(1)
  {
    switch(comando_motor)
    {
      case 'F':
        forward();
        digitalWrite(LED_POS,HIGH);
        digitalWrite(LED_STOP,LOW);
        break;
      case 'B':
        back();
        digitalWrite(LED_STOP,LOW);
        ATRAS = true;
        digitalWrite(LED_POS,HIGH);
        break;
      case 'S':
        stop();
        IZQ = false;
        DCHA = false;
        ATRAS = false;
        digitalWrite(LED_POS,LOW);
        digitalWrite(LED_STOP,HIGH);
        break;
      case 'R':
        right();
        digitalWrite(LED_POS,HIGH);
        digitalWrite(LED_STOP,LOW);
        DCHA = true;
        break;
      case 'L':
        left();
        digitalWrite(LED_POS,HIGH);
        digitalWrite(LED_STOP,LOW);
        IZQ = true;
        break;
    }
    vTaskDelayUntil(&xLastWakeTime,xPeriod);
  }
}
