/*
  TimerInterrupt_test_Mega.ino
  Minimal test for khoih-prog TimerInterrupt (v1.8.x) on Arduino Mega 2560.

  Make sure you installed "TimerInterrupt" by khoih-prog via Library Manager,
  and selected Board = "Arduino Mega 2560".
*/

// Put these defines BEFORE including TimerInterrupt.h
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define USE_TIMER_1     true   // enable ITimer1
#define USE_TIMER_3     true   // enable ITimer3

#include <TimerInterrupt.h> // khoih-prog TimerInterrupt

// --- pins (using your pins)
const uint8_t A_IN_PIN  = 46; // input for "A" (reads B_OUT)
const uint8_t A_OUT_PIN = 48; // output A -> B_IN
const uint8_t B_IN_PIN  = 47; // input for "B" (reads A_OUT)
const uint8_t B_OUT_PIN = 49; // output B -> A_IN

// intervals in ms
const unsigned long A_INTERVAL_MS = 250;
const unsigned long B_INTERVAL_MS = 500;

// volatile flags set from ISR (tiny)
volatile bool flagA = false;
volatile bool flagB = false;
volatile unsigned long a_ticks = 0;
volatile unsigned long b_ticks = 0;

// ISR callbacks (these run in ISR context)
void onTimerA()
{
  flagA = true;
  a_ticks++;
}

void onTimerB()
{
  flagB = true;
  b_ticks++;
}

bool startTimers()
{
  // Initialize timer objects (per README usage)
  ITimer1.init();
  ITimer3.init();

  // attach intervals; returns true on success
  if (!ITimer1.attachInterruptInterval(A_INTERVAL_MS, onTimerA))
  {
    Serial.println(F("ITimer1.attachInterruptInterval() failed"));
    return false;
  }

  if (!ITimer3.attachInterruptInterval(B_INTERVAL_MS, onTimerB))
  {
    Serial.println(F("ITimer3.attachInterruptInterval() failed"));
    return false;
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) { } // wait for serial (optional)

  Serial.println(F("TimerInterrupt minimal test (Mega2560)"));

  pinMode(A_IN_PIN, INPUT_PULLUP);
  pinMode(A_OUT_PIN, OUTPUT);
  digitalWrite(A_OUT_PIN, LOW);

  pinMode(B_IN_PIN, INPUT_PULLUP);
  pinMode(B_OUT_PIN, OUTPUT);
  digitalWrite(B_OUT_PIN, LOW);

  if (startTimers())
    Serial.println(F("Timers started OK"));
  else
    Serial.println(F("Timers failed to start - check library / macros"));

  // Note: keep ISRs short (as above). Do heavy work in loop().
}

void loop()
{
  // process Timer A event
  if (flagA)
  {
    noInterrupts();
    flagA = false;
    unsigned long t = a_ticks;
    interrupts();

    // Toggle A_OUT and print a small line so you see activity
    static bool stateA = false;
    stateA = !stateA;
    digitalWrite(A_OUT_PIN, stateA);

    Serial.print(F("\t\t\t[A] tick# "));
    Serial.print(t);
    Serial.print(F("  A_OUT="));
    Serial.println(stateA);
  }

  // process Timer B event
  if (flagB)
  {
    noInterrupts();
    flagB = false;
    unsigned long t = b_ticks;
    interrupts();

    static bool stateB = false;
    stateB = !stateB;
    digitalWrite(B_OUT_PIN, stateB);

    Serial.print(F("[B] tick# "));
    Serial.print(t);
    Serial.print(F("  B_OUT="));
    Serial.println(stateB);
  }

  // small yield
  delay(1);
}
