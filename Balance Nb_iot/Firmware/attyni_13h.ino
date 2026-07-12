#include <Arduino.h>

// ================= CONFIG =================
#define HEARTBEAT_PIN   PIN_PA1
#define RESET_PIN       PIN_PA2

// Période inchangée : 31.25 ms (32 Hz)
#define RTC_PERIOD RTC_PERIOD_CYC1024_gc

// Calcul pour 14 heures :
// 14h × 3600s × 32Hz = 1,612,800 ticks
#define HEARTBEAT_TIMEOUT_TICKS 1612800UL // 'UL' pour unsigned long

#define RESET_PULSE_TICKS       16     // 500ms / 31.25ms = 16 ticks
#define DEBOUNCE_TICKS          2      // ~62.5ms

// ================= VARIABLES =================
volatile bool heartbeatDetected = false;
volatile uint32_t timeoutCounter = 0;    // CHANGÉ : 32 bits !
volatile uint16_t resetPulseCounter = 0;
volatile bool resetActive = false;

// ================= RTC PIT ISR =================
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;
  
  // Gestion timeout
  if (heartbeatDetected) {
    timeoutCounter = 0;
    heartbeatDetected = false;
  } else {
    if (timeoutCounter < HEARTBEAT_TIMEOUT_TICKS) {
      timeoutCounter++;
    }
  }
  
  // Déclenchement reset
  if (timeoutCounter >= HEARTBEAT_TIMEOUT_TICKS && !resetActive) {
    resetActive = true;
    resetPulseCounter = 0;
    PORTA.OUTSET = PIN2_bm;  // Activer RESET
  }
  
  // Gestion impulsion RESET
  if (resetActive) {
    if (resetPulseCounter >= RESET_PULSE_TICKS) {
      PORTA.OUTCLR = PIN2_bm;  // Désactiver RESET
      resetActive = false;
      timeoutCounter = 0;
      heartbeatDetected = true;  // Réarmer
    } else {
      resetPulseCounter++;
    }
  }
}

// ================= HEARTBEAT ISR =================
ISR(PORTA_PORT_vect) {
  if (PORTA.INTFLAGS & PIN1_bm) {
    heartbeatDetected = true;
    PORTA.INTFLAGS = PIN1_bm;
  }
}

// ================= SETUP =================
void setup() {
  // Configuration IO
  PORTA.DIRSET = PIN2_bm;    // PA2 en sortie
  PORTA.DIRCLR = PIN1_bm;    // PA1 en entrée
  PORTA.OUTCLR = PIN2_bm;    // PA2 à LOW
  
  // Pull-up + interruption sur front montant
  PORTA.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
  
  // Configuration RTC PIT
  while (RTC.STATUS > 0);  // Attendre synchronisation
  
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;     // Source: OSCULP32K interne
  RTC.PITCTRLA = RTC_PERIOD | RTC_PITEN_bm;
  RTC.PITINTCTRL = RTC_PI_bm;
  
  // Initialisation
  heartbeatDetected = true;
  timeoutCounter = 0;
  resetActive = false;
  
  sei();
}

// ================= LOOP =================
void loop() {
  // Tout est géré dans les ISR
}