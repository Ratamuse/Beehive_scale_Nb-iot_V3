
/*
  ============================================================
  Ratamuse 2026
  ATtiny202 - Watchdog externe (Heartbeat + Reset automatique)
  ============================================================

  Ce programme est destiné à être téléversé sur un ATtiny202
  via la broche UPDI (programmation UPDI obligatoire).

  Objectif :
  - Surveiller un signal "heartbeat" sur PA1
  - Si aucun heartbeat n’est détecté pendant ~14 heures,
    l’ATtiny202 déclenche une impulsion de reset sur PA2
    afin de redémarrer un autre microcontrôleur / système.

  Configuration actuelle :
  - RTC PIT à ~32 Hz (tick ≈ 31,25 ms)
  - Timeout : 1 612 800 ticks ≈ 14 heures
  - Impulsion RESET : 16 ticks ≈ 500 ms
  - Délai de 10 s au démarrage pour laisser une fenêtre UPDI
    avant de passer en mode sleep (standby)

  Connexions :
  - PA1 : entrée heartbeat (pull-up activée, front montant)
  - PA2 : sortie reset (niveau actif selon câblage)

  Remarque :
  - Le microcontrôleur reste en mode basse consommation
    (SLEEP_MODE_STANDBY) entre les interruptions RTC/heartbeat.


*/


#include <Arduino.h>
#include <avr/sleep.h>

// ================= CONFIG =================
#define HEARTBEAT_PIN   PIN_PA1
#define RESET_PIN       PIN_PA2


// RTC PIT à 32 Hz (31.25 ms)
#define RTC_PERIOD RTC_PERIOD_CYC1024_gc
// 14h × 3600 × 32 = 1 612 800 ticks (≈ 14 h)
#define HEARTBEAT_TIMEOUT_TICKS 1612800UL   // Ajuste selon besoin (~120 s)
#define RESET_PULSE_TICKS 16             // ~500 ms
#define INITIAL_NO_SLEEP_MS 10000UL      // Fenêtre UPDI

// ================= VARIABLES =================
volatile bool heartbeatEvent = false;
volatile uint32_t timeoutCounter = 0;
volatile uint16_t resetPulseCounter = 0;
volatile bool resetActive = false;
volatile bool allowSleep = false;

// ================= RTC PIT ISR =================
ISR(RTC_PIT_vect) {
  RTC.PITINTFLAGS = RTC_PI_bm;

  // Timeout : incrémenté uniquement dans l'ISR
  if (heartbeatEvent) {
    timeoutCounter = 0;
    heartbeatEvent = false;
  } else {
    if (timeoutCounter < HEARTBEAT_TIMEOUT_TICKS) {
      timeoutCounter++;
    }
  }

  // Déclenchement reset
  if (timeoutCounter >= HEARTBEAT_TIMEOUT_TICKS && !resetActive) {
    resetActive = true;
    resetPulseCounter = 0;
    PORTA.OUTSET = PIN2_bm;  // RESET actif
  }

  // Gestion impulsion reset
  if (resetActive) {
    if (resetPulseCounter >= RESET_PULSE_TICKS) {
      PORTA.OUTCLR = PIN2_bm;  // Fin reset
      resetActive = false;
      timeoutCounter = 0;
      heartbeatEvent = false;
    } else {
      resetPulseCounter++;
    }
  }
}

// ================= HEARTBEAT ISR =================
ISR(PORTA_PORT_vect) {
  if (PORTA.INTFLAGS & PIN1_bm) {
    if (!resetActive) {
      heartbeatEvent = true;  // resetCounter remis à 0 dans ISR PIT
    }
    PORTA.INTFLAGS = PIN1_bm;
  }
}

// ================= SLEEP =================
void configureSleep() {
  set_sleep_mode(SLEEP_MODE_STANDBY);

  // Désactiver broches inutilisées
  PORTA.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN4CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PORTA.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
}

void enterSleep() {
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

// ================= SETUP =================
void setup() {
  // IO
  PORTA.DIRSET = PIN2_bm;     // PA2 sortie reset
  PORTA.DIRCLR = PIN1_bm;     // PA1 entrée heartbeat
  PORTA.OUTCLR = PIN2_bm;     // reset inactif

  // Heartbeat : pull-up + front montant
  PORTA.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;

  // RTC PIT
  while (RTC.STATUS > 0);

  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
  RTC.PITCTRLA = RTC_PERIOD | RTC_PITEN_bm;
  RTC.PITINTCTRL = RTC_PI_bm;

  #ifdef RTC_PITRUNSTDBY_bm
  RTC.PITCTRLA |= RTC_PITRUNSTDBY_bm;  // RTC actif en standby
  #endif

  configureSleep();

  // Init
  heartbeatEvent = false;
  timeoutCounter = 0;
  resetActive = false;
  allowSleep = false;

  // Fenêtre UPDI
  delay(INITIAL_NO_SLEEP_MS);

  allowSleep = true;

  sei();
}

// ================= LOOP =================
void loop() {
  if (allowSleep) {
    enterSleep();  // MCU reste en deep sleep jusqu'à ISR
  }
}
