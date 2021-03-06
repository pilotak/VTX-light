#include <Arduino.h>
#include <limits.h>
#include <avr/wdt.h>
#include "MovingAverage.h"

#define POT_MIN_V 1100 // volts*100
#define POT_MAX_V 1800 // volts*100
#define PWM_MIN   12
#define PWM_MAX   255
#define VCC_MAX   2032 // volts*100

// This is the COUNTERCLOCKWISE pin mapping

const int button1Pin = 4;  // PA6
const int button2Pin = 0;  // PB0

const int aVcc = A0;
const int aPot = A1;

const int mainLight = 5;  // PA5
const int hipwrLed  = 1;  // PB1

const int redLight   = 2;  // PB2
const int greenLight = 3;  // PA7
const int blueLight  = 6;  // PA4

const int swapBlinker = 8;  // PA2
const int flasher     = 7;  // PA3

enum modes {
    NORMAL,
    FADE
};

enum fade_modes {
    UP,
    DOWN
};

enum blinker_modes {
    OFF,
    BLINK
};

MovingAverage <unsigned short, 16> analogReading;

modes mode            = NORMAL;
modes mode_prev       = NORMAL;
fade_modes fade_mode  = UP;
blinker_modes blinker = OFF;

bool hipwr                = false;
byte current_pwm          = PWM_MAX;
unsigned short pot        = 0;
unsigned long debounce[2] = {0, 0};

void setup() {
    pinMode(button1Pin, INPUT);
    pinMode(button2Pin, INPUT);
    pinMode(mainLight, OUTPUT);
    pinMode(hipwrLed, OUTPUT);

    pinMode(redLight, OUTPUT);
    pinMode(greenLight, OUTPUT);
    pinMode(blueLight, OUTPUT);

    pinMode(swapBlinker, OUTPUT);
    pinMode(flasher, OUTPUT);

    delay(1);
    digitalWrite(mainLight, LOW);
    digitalWrite(hipwrLed, LOW);

    digitalWrite(redLight, LOW);
    digitalWrite(greenLight, LOW);
    digitalWrite(blueLight, LOW);

    digitalWrite(swapBlinker, LOW);
    digitalWrite(flasher, LOW);

    analogRead(aPot); // fake read
    analogRead(aVcc); // fake read

    delay(5);

    pot = map(analogRead(aPot), 0, 1023, POT_MIN_V, POT_MAX_V);

    wdt_enable(WDTO_250MS);
}

void loop() {
    static unsigned short counter = 1;
    static bool flasher_state = true;

    if (hipwr) {
        if (counter % 10 == 0) {  // 10ms
            debounce[0] <<= 1;

            if (digitalRead(button1Pin) == LOW) {  // ON
                debounce[0] |= 1;
            }

            if (debounce[0] == ULONG_MAX) {
                mode = FADE;

            } else if (debounce[0] == 0) {
                mode = NORMAL;
            }
        }

        if (mode != mode_prev) {
            if (mode == NORMAL) {  // switch is now off
                current_pwm = PWM_MAX;

            } else if (mode == FADE) {  // switch is now on
                fade_mode = DOWN;
                current_pwm = PWM_MAX;
            }

            mode_prev = mode;
        }

        analogWrite(mainLight, current_pwm);

        digitalWrite(redLight, HIGH);
        //analogWrite(redLight, 75);
        //digitalWrite(greenLight, HIGH);
        analogWrite(greenLight, 7); // 125
        //digitalWrite(blueLight, HIGH);

        if (mode == FADE) {
            if (fade_mode == UP) {
                if (current_pwm == PWM_MAX) {
                    fade_mode = DOWN;

                } else {
                    current_pwm++;
                }

            } else if (fade_mode == DOWN) {
                if (current_pwm == PWM_MIN) {
                    fade_mode = UP;

                } else {
                    current_pwm--;
                }
            }
        }

    } else if (counter % 50 == 0) {  // 50ms
        analogReading.add(analogRead(aVcc));

        if (counter % 10 == 0) {  // 100ms
            unsigned short voltage = map(analogReading.get(), 0, 1023, 0, VCC_MAX);

            if (voltage >= pot) {
                hipwr = true;
                digitalWrite(hipwrLed, HIGH);
            }
        }
    }

    if (counter % 10 == 0) {  // 10ms
        debounce[1] <<= 1;

        if (digitalRead(button2Pin) == LOW) {  // ON
            debounce[1] |= 1;
        }

        if (debounce[1] == ULONG_MAX) {
            blinker = BLINK;

        } else if (debounce[1] == 0) {
            blinker = OFF;
        }
    }

    if (counter % 100 == 0) {  // 100ms
        wdt_reset();
    }

    if (counter % 500 == 0) {  // 500ms
        counter = 1;

        if (blinker == BLINK) {
            digitalWrite(swapBlinker, HIGH);
            digitalWrite(flasher, flasher_state);
            flasher_state = !flasher_state;

        } else if (blinker == OFF) {
            digitalWrite(swapBlinker, LOW);
            digitalWrite(flasher, LOW);
            flasher_state = true;
        }

    } else {
        counter++;
    }

    delay(1);
}
