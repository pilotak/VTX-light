#include <Arduino.h>
#include <limits.h>
#include <avr/wdt.h>
#include "MovingAverage.h"

#ifndef cbi
    #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
    #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define POT_MIN_V 1100 // volts*100
#define POT_MAX_V 1800 // volts*100
#define PWM_MIN   12
#define PWM_MAX   255
#define VCC_MAX   2032 // volts*100

MovingAverage <unsigned short, 16> analogReading;

enum modes {
    NORMAL,
    FADE,
    UP,
    DOWN
};

const int buttonPin = 1;
const int vccPin = A2;
const int mosfetPin = 0;
const int potPin = A1;
const int hipwrPin = 3;

modes mode = NORMAL;
modes mode_prev = NORMAL;
modes fade_mode = UP;

bool hipwr = false;
byte current_pwm = PWM_MAX;
unsigned short pot = 0;
unsigned long debounce = 0;

void setup() {
    pinMode(buttonPin, INPUT);
    pinMode(mosfetPin, OUTPUT);
    pinMode(hipwrPin, OUTPUT);
    delay(1);
    digitalWrite(mosfetPin, LOW);
    digitalWrite(hipwrPin, LOW);

    analogRead(potPin); // fake read
    analogRead(vccPin); // fake read
    delay(5);

    pot = map(analogRead(potPin), 0, 1023, POT_MIN_V, POT_MAX_V);

    wdt_enable(WDTO_250MS);
}

void loop() {
    static byte counter = 1;

    if (hipwr) {
        if (counter % 10 == 0) {  // 10ms
            debounce <<= 1;

            if (digitalRead(buttonPin) == LOW) { // ON
                debounce |= 1;
            }

            if (debounce == ULONG_MAX) {
                mode = FADE;

            } else if (debounce == 0) {
                mode = NORMAL;
            }
        }

        if (mode != mode_prev) {
            if (mode == NORMAL) { // switch is now off
                current_pwm = PWM_MAX;

            } else if (mode == FADE) { // switch is now on
                fade_mode = DOWN;
                current_pwm = PWM_MAX;
            }

            mode_prev = mode;
        }

        analogWrite(mosfetPin, current_pwm);

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
        analogReading.add(analogRead(vccPin));

        if (counter % 10 == 0) {  // 100ms
            unsigned short voltage = map(analogReading.get(), 0, 1023, 0, VCC_MAX);

            if (voltage >= pot) {
                hipwr = true;
                digitalWrite(hipwrPin, HIGH);
            }
        }
    }

    if (counter % 200 == 0) {  // 200ms
        counter = 1;
        wdt_reset();

    } else {
        counter++;
    }

    delay(1);
}
