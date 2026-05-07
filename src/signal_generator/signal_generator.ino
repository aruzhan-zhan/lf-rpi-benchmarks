/*
 * STM32 Hardware External Signal Generator for RPi Timing Benchmarks
 * Target: STM32 Nucleo-64 (Arduino IDE)
 * Purpose: Generates periodic and sporadic pulses to trigger 
 * Raspberry Pi interrupts.
 */

// Pin Definitions
const int PERIODIC_PIN = D2;   // Connect to RPi wPi 16 (BCM 15)
const int SPORADIC_PIN = D3;   // Connect to RPi wPi 1 (BCM 18)

// Timing Constants
const unsigned long PERIODIC_INTERVAL = 10; // 10ms (100Hz)
unsigned long lastPeriodic = 0;
unsigned long nextSporadicBurst = 0;

void setup() {
  pinMode(PERIODIC_PIN, OUTPUT);
  pinMode(SPORADIC_PIN, OUTPUT);
  
  // Initialize pins LOW
  digitalWrite(PERIODIC_PIN, LOW);
  digitalWrite(SPORADIC_PIN, LOW);
  
  // Seed the random generator using an unconnected analog pin
  randomSeed(analogRead(A0));
  
  nextSporadicBurst = millis() + random(100, 500);
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. PERIODIC INTERRUPT GENERATION
  if (currentMillis - lastPeriodic >= PERIODIC_INTERVAL) {
    lastPeriodic = currentMillis;
    triggerPulse(PERIODIC_PIN);
  }

  // 2. SPORADIC INTERRUPT GENERATION (Bursts)
  if (currentMillis >= nextSporadicBurst) {
    // Generate a burst of 3-5 quick pulses
    int pulses = random(3, 6);
    for (int i = 0; i < pulses; i++) {
      triggerPulse(SPORADIC_PIN);
      delay(random(1, 5)); // Random tiny gap between pulses in burst
    }
    // Schedule the next random burst (between 200ms and 1 second later)
    nextSporadicBurst = currentMillis + random(200, 1000);
  }
}

// Helper function to create a clean pulse for edge detection
void triggerPulse(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(100); // 100us pulse width
  digitalWrite(pin, LOW);
}
