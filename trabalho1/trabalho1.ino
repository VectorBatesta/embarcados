/*
  TimerInterrupt_debug_visualize.ino
  Single-file test for TimerInterrupt (khoih-prog).
  Prints timestamped CSV + events so you can see the relation between A_OUT and B_IN.

  Make sure TimerInterrupt by khoih-prog is installed and Board = Arduino Mega 2560.
*/

// Enable timers BEFORE including library
#define USE_TIMER_1  true
#define USE_TIMER_3  true
#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

#include <TimerInterrupt.h>
#include <Arduino.h>

// Pin mapping (your pins)
const uint8_t A_IN_PIN  = 46; // A reads this (connected to B_OUT)
const uint8_t A_OUT_PIN = 48; // A drives this (connect to B_IN)
const uint8_t B_IN_PIN  = 47; // B reads this (connected to A_OUT)
const uint8_t B_OUT_PIN = 49; // B drives this (connect to A_IN)

// Intervals & toggle rates (ms)
const unsigned long A_INTERVAL_MS = 200;
const unsigned long B_INTERVAL_MS = 200;
const uint8_t A_toggle_every_ticks = 3; // every N A-ticks toggle A_out
const uint8_t B_toggle_every_ticks = 5; // B toggles its own out (for activity)

// Volatile flags set in ISRs
volatile bool flagA_tick = false;
volatile bool flagB_tick = false;
volatile unsigned long A_tick_count = 0;
volatile unsigned long B_tick_count = 0;

// States
bool A_out_state = LOW;
bool B_out_state = LOW;

// For correlation measurement
unsigned long lastAOutChangeTime = 0; // millis() when A last toggled its output
unsigned long lastAOutTick = 0;
int lastBInValue = HIGH; // because we use INPUT_PULLUP initially

// ISR callbacks (very short)
void onTimerA() {
  flagA_tick = true;
  A_tick_count++;
}
void onTimerB() {
  flagB_tick = true;
  B_tick_count++;
}

// start timers via library
bool startTimers() {
  ITimer1.init();
  ITimer3.init();

  if (!ITimer1.attachInterruptInterval(A_INTERVAL_MS, onTimerA)) {
    return false;
  }
  if (!ITimer3.attachInterruptInterval(B_INTERVAL_MS, onTimerB)) {
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // wait for Serial to attach (optional)

  Serial.println(F("TimerInterrupt debug visualize starting"));
  pinMode(A_IN_PIN, INPUT_PULLUP);
  pinMode(A_OUT_PIN, OUTPUT);
  digitalWrite(A_OUT_PIN, LOW);

  pinMode(B_IN_PIN, INPUT_PULLUP);
  pinMode(B_OUT_PIN, OUTPUT);
  digitalWrite(B_OUT_PIN, LOW);

  lastBInValue = digitalRead(B_IN_PIN);

  if (!startTimers()) {
    Serial.println(F("ERROR: timers did not start. Check TimerInterrupt install and macros."));
  } else {
    Serial.println(F("Timers started OK"));
  }

  // Header for CSV-style heartbeat lines (makes it easy to import)
  Serial.println(F("time_ms,src,tick,A_out,B_in"));
}

// Called from main loop when A tick fired
void processArduinoA() {
  // Toggle logic
  static unsigned int localA = 0;
  localA++;
  bool prevOut = A_out_state;
  if ((localA % A_toggle_every_ticks) == 0) {
    A_out_state = !A_out_state;
  }

  // If B is pulled low (connected), create a short pulse to demonstrate interaction
  int readB_via_Ain = digitalRead(A_IN_PIN);
  if (readB_via_Ain == LOW) {
    // force a short HIGH pulse on A_out so B sees activity
    A_out_state = HIGH;
  }

  // Commit output
  digitalWrite(A_OUT_PIN, A_out_state);

  // If A actually changed its output, record time and print event
  if (A_out_state != prevOut) {
    lastAOutChangeTime = millis();
    lastAOutTick = A_tick_count;
    Serial.print(F("EVENT,A_OUT_CHANGE,"));
    Serial.print(lastAOutChangeTime);
    Serial.print(F(",tick#"));
    Serial.print(lastAOutTick);
    Serial.print(F(",val="));
    Serial.println(A_out_state);
  }

  // Heartbeat CSV line
  int b_in_now = digitalRead(B_IN_PIN);
  Serial.print(millis());
  Serial.print(',');
  Serial.print('A');
  Serial.print(',');
  Serial.print(A_tick_count);
  Serial.print(',');
  Serial.print((int)A_out_state);
  Serial.print(',');
  Serial.println(b_in_now);
}

// Called from main loop when B tick fired
void processArduinoB() {
  // Toggle own out occasionally (so B also generates output)
  static unsigned int localB = 0;
  localB++;
  if ((localB % B_toggle_every_ticks) == 0) {
    B_out_state = !B_out_state;
    digitalWrite(B_OUT_PIN, B_out_state);

    // Report B's own output change as event (for clarity)
    Serial.print(F("EVENT,B_OUT_CHANGE,"));
    Serial.print(millis());
    Serial.print(F(",tick#"));
    Serial.print(B_tick_count);
    Serial.print(F(",val="));
    Serial.println(B_out_state);
  }

  // Read A_out through B_IN
  int readA_via_Bin = digitalRead(B_IN_PIN);

  // If input changed (edge detected), compute delay from last A change
  if (readA_via_Bin != lastBInValue) {
    unsigned long now = millis();
    long delayFromA = -1;
    if (lastAOutChangeTime != 0) {
      delayFromA = (long)(now - lastAOutChangeTime); // may be negative if clocks wrap (very unlikely here)
    }

    Serial.print(F("EVENT,B_IN_CHANGE,"));
    Serial.print(now);
    Serial.print(F(",tick#"));
    Serial.print(B_tick_count);
    Serial.print(F(",val="));
    Serial.print(readA_via_Bin);
    Serial.print(F(",delay_from_last_A_ms="));
    Serial.println(delayFromA);

    lastBInValue = readA_via_Bin;
  }

  // Heartbeat CSV line
  int a_out_now = digitalRead(A_OUT_PIN);
  Serial.print(millis());
  Serial.print(',');
  Serial.print('B');
  Serial.print(',');
  Serial.print(B_tick_count);
  Serial.print(',');
  Serial.print(a_out_now);
  Serial.print(',');
  Serial.println((int)B_out_state);
}

void loop() {
  // handle A
  if (flagA_tick) {
    noInterrupts();
    flagA_tick = false;
    interrupts();
    processArduinoA();
  }

  // handle B
  if (flagB_tick) {
    noInterrupts();
    flagB_tick = false;
    interrupts();
    processArduinoB();
  }

  // tiny yield
  delay(1);
}
