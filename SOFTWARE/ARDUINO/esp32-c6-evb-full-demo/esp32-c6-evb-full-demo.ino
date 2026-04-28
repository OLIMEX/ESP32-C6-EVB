// ======================================================
// ESP32-C6-EVB (Olimex) - full demo
//
// Features:
// - 4 relays rotate every 2 seconds
// - User LED blinks every 2 seconds (active LOW)
// - Button toggles relay rotation ON/OFF
// - 4 optocoupler inputs monitored (event-based logging)
//
// Hardware specifics:
// - Native USB used for Serial
// - Relays: ACTIVE HIGH
// - LED: ACTIVE LOW
// - Button: ACTIVE LOW (internal pull-up)
// - Optocouplers: ACTIVE LOW (47k pull-up to 3.3V),
// provide between 5V DC and 30V DC!
// 
// After downloading the demo, open the serial monitor
// Ensure that "USB CDC On Boot" is Enabled
// Refer to "arduino-settings.jpg" for tested settings
// ======================================================


// ===== Relay pin definitions =====
#define RELAY1 10
#define RELAY2 11
#define RELAY3 22
#define RELAY4 23

// ===== User interface =====
#define USER_LED 8
#define USER_BUTTON 9

// ===== Optocoupler inputs =====
#define OPTO1 1
#define OPTO2 2
#define OPTO3 3
#define OPTO4 15


// ===== Timing (milliseconds) =====
const unsigned long RELAY_INTERVAL = 2000;
const unsigned long LED_INTERVAL   = 2000;


// ===== Relay logic =====
// Board is ACTIVE HIGH:
// HIGH = relay ON
// LOW  = relay OFF
#define RELAY_ON  HIGH
#define RELAY_OFF LOW


// ===== State variables =====
unsigned long lastRelayTime = 0;
unsigned long lastLedTime   = 0;

int currentRelay = 0;        // internal index (0–3)
bool relaysRunning = true;

bool ledState = false;
bool lastButtonState = HIGH;


// ===== Pin arrays for iteration =====
int relayPins[4] = {RELAY1, RELAY2, RELAY3, RELAY4};
int optoPins[4]  = {OPTO1, OPTO2, OPTO3, OPTO4};


// ===== Optocoupler state tracking =====
// Used to detect changes (edge detection)
bool lastOptoState[4] = {HIGH, HIGH, HIGH, HIGH};


// ======================================================
// SETUP
// ======================================================
void setup() {
  // Start native USB serial
  Serial.begin(115200);

  // Delay to allow terminal to connect (important for USB CDC)
  delay(2000);

  Serial.println("\nESP32-C6 EVB Full Demo Start");

  // ----- Initialize relays (all OFF) -----
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_OFF);
  }

  // ----- Initialize user LED (OFF) -----
  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, HIGH); // HIGH = OFF (active LOW LED)

  // ----- Initialize button -----
  // Internal pull-up → pressed = LOW
  pinMode(USER_BUTTON, INPUT_PULLUP);

  // ----- Initialize optocoupler inputs -----
  // External pull-ups already present on board (47k)
  for (int i = 0; i < 4; i++) {
    pinMode(optoPins[i], INPUT);
  }
}


// ======================================================
// HELPER: Check if relay is logically ON
// ======================================================
bool isRelayOn(int i) {
  return digitalRead(relayPins[i]) == RELAY_ON;
}


// ======================================================
// PRINT RELAY STATUS
// ======================================================
void printRelayStatus() {
  Serial.println("=== RELAY STATUS ===");

  for (int i = 0; i < 4; i++) {
    Serial.print("Relay ");
    Serial.print(i + 1);  // numbering
    Serial.print(": ");
    Serial.println(isRelayOn(i) ? "ON" : "OFF");
  }

  Serial.println("====================");
}


// ======================================================
// PRINT OPTOCOUPLER STATUS
// ======================================================
void printOptoStatus() {
  Serial.println("=== OPTO INPUT STATUS ===");

  for (int i = 0; i < 4; i++) {
    Serial.print("IN");
    Serial.print(i + 1);
    Serial.print(": ");

    // ACTIVE LOW → LOW means input is active
    bool active = (digitalRead(optoPins[i]) == LOW);
    Serial.println(active ? "ACTIVE" : "IDLE");
  }

  Serial.println("==========================");
}


// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  unsigned long now = millis();


  // ==================================================
  // BUTTON HANDLING (edge detection + debounce)
  // ==================================================
  bool currentButtonState = digitalRead(USER_BUTTON);

  // Detect button press (HIGH → LOW transition)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    delay(30); // simple debounce

    if (digitalRead(USER_BUTTON) == LOW) {
      relaysRunning = !relaysRunning;

      Serial.println("\n[EVENT] Button pressed");
      Serial.print("Relay rotation: ");
      Serial.println(relaysRunning ? "RUNNING" : "STOPPED");

      printRelayStatus();
      printOptoStatus();
    }
  }

  lastButtonState = currentButtonState;


  // ==================================================
  // RELAY SEQUENCER
  // ==================================================
  if (relaysRunning && (now - lastRelayTime >= RELAY_INTERVAL)) {
    lastRelayTime = now;

    // Turn all relays OFF
    for (int i = 0; i < 4; i++) {
      digitalWrite(relayPins[i], RELAY_OFF);
    }

    // Turn current relay ON
    digitalWrite(relayPins[currentRelay], RELAY_ON);

    Serial.print("Active relay -> ");
    Serial.println(currentRelay + 1);

    // Move to next relay (circular)
    currentRelay = (currentRelay + 1) % 4;
  }


  // ==================================================
  // LED BLINK (independent task)
  // ==================================================
  if (now - lastLedTime >= LED_INTERVAL) {
    lastLedTime = now;

    ledState = !ledState;

    // ACTIVE LOW LED:
    // LOW = ON, HIGH = OFF
    digitalWrite(USER_LED, ledState ? LOW : HIGH);

    Serial.print("LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  }


  // ==================================================
  // OPTOCOUPLER MONITORING (change detection)
  // ==================================================
  for (int i = 0; i < 4; i++) {
    bool currentState = digitalRead(optoPins[i]);

    // Detect change
    if (currentState != lastOptoState[i]) {
      lastOptoState[i] = currentState;

      Serial.print("[OPTO EVENT] IN");
      Serial.print(i + 1);
      Serial.print(" -> ");

      // ACTIVE LOW logic
      Serial.println(currentState == LOW ? "ACTIVE" : "IDLE");
    }
  }
}
