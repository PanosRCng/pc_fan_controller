#include <util/delay.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define F_CPU 8000000

#define MOTOR_PIN PB1
#define POT_PIN PB3
#define ONE_WIRE_BUS PB2
#define LED_FLOW_PIN PB0
#define LED_OVERRIDE_PIN PB4

#define STARTUP 0
#define AUTO_TEMP 1
#define MANUAL 2

#define DUTY_CYCLE_25 10
#define DUTY_CYCLE_50 20
#define DUTY_CYCLE_75 30
#define DUTY_CYCLE_100 40

#define STARTUP_DURATION_SECONDS 5

int state;
int duty_cycle;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


bool pot_changed()
{
  bool result;

  int pot_value = analogRead(POT_PIN);
  int duty_cycle = map(pot_value, 0, 1023, DUTY_CYCLE_25, DUTY_CYCLE_100);

  if(duty_cycle > DUTY_CYCLE_25)
  {
    result = true;
  }
  else
  {
    result = false;
  }

  return result;
}


void startup()
{
  OCR0B = DUTY_CYCLE_100;

  digitalWrite(LED_FLOW_PIN, HIGH);
  digitalWrite(LED_OVERRIDE_PIN, HIGH);

  _delay_ms(STARTUP_DURATION_SECONDS * 1000);

  digitalWrite(LED_FLOW_PIN, LOW);
  digitalWrite(LED_OVERRIDE_PIN, LOW);

  state = AUTO_TEMP;
}


void auto_temp()
{
  if(pot_changed())
  {
    state = MANUAL;
    return;
  }

  int temp;

  if(sensors.getDS18Count() != 0)
  {
    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
  }

  if(temp < 0)   // temp sensor problem
  {
    duty_cycle = DUTY_CYCLE_100;
  }
  else if(temp < 30)
  {
    duty_cycle = DUTY_CYCLE_25;
  }
  else if(temp < 35)
  {
    duty_cycle = DUTY_CYCLE_50;
  }
  else if(temp < 40)
  {
    duty_cycle = DUTY_CYCLE_75;
  }
  else
  {
    duty_cycle = DUTY_CYCLE_100;
  }

  OCR0B = duty_cycle;

  digitalWrite(LED_OVERRIDE_PIN, LOW);
}


void manual()
{
  int pot_value;

  pot_value = analogRead(POT_PIN);

  duty_cycle = map(pot_value, 0, 1023, DUTY_CYCLE_25, DUTY_CYCLE_100);

  if(duty_cycle == DUTY_CYCLE_25)
  {
    state = AUTO_TEMP;
    return;
  }

  OCR0B = duty_cycle;

  digitalWrite(LED_OVERRIDE_PIN, HIGH);
}



void indicate_duty_cycle()
{
  while(duty_cycle >= DUTY_CYCLE_25)
  {
    digitalWrite(LED_FLOW_PIN, HIGH);
    _delay_ms(100);
    digitalWrite(LED_FLOW_PIN, LOW);
    _delay_ms(100);

    duty_cycle = duty_cycle - DUTY_CYCLE_25;
  }

  _delay_ms(1000);
}


void setup_pwm()
{
  // CPU_FREQ / PRESCALE / (X + 1) = 25Khz
  // 8Mhz / 8 / (40 + 1) = 25Khz
 
  // Fast PWM Mode, Prescaler = /8
  // PWM on Pin 1(PB1)
  TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(WGM02) | _BV(CS01);

  OCR0A = DUTY_CYCLE_100;
  OCR0B = 0;
}


void setup_temp()
{
  sensors.begin();
  
  if(sensors.getDS18Count() != 0)
  {
    sensors.setResolution(12);
  }
}


void setup()
{
  pinMode(POT_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  pinMode(LED_FLOW_PIN, OUTPUT);
  pinMode(LED_OVERRIDE_PIN, OUTPUT);

  setup_pwm();

  setup_temp();

  state = STARTUP;
}



void loop()
{
  switch(state)
  {
    case STARTUP:
      startup();
      break;

    case AUTO_TEMP:
      auto_temp();
      break;

    case MANUAL:
      manual();
      break;
  }

  indicate_duty_cycle();
}
