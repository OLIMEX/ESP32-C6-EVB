// ======================================================
// ESP32-C6 EVB - Serial Control Console
// Open Serial Monitor and type help
// ======================================================

#define RELAY1 10
#define RELAY2 11
#define RELAY3 22
#define RELAY4 23

#define USER_LED 8
#define USER_BUTTON 9

#define OPTO1 1
#define OPTO2 2
#define OPTO3 3
#define OPTO4 15

#define RELAY_ON  HIGH
#define RELAY_OFF LOW

int relayPins[4] = {RELAY1, RELAY2, RELAY3, RELAY4};
int optoPins[4]  = {OPTO1, OPTO2, OPTO3, OPTO4};

bool demoMode = false;
int demoIndex = 0;
unsigned long lastDemoTime = 0;

String inputBuffer = "";

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\nESP32-C6 EVB Serial Console");
  Serial.println("Type 'help'");

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_OFF);
  }

  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, HIGH); // OFF (active LOW)

  pinMode(USER_BUTTON, INPUT_PULLUP);

  for (int i = 0; i < 4; i++) {
    pinMode(optoPins[i], INPUT);
  }
}

// ======================================================
// HELP MENU
// ======================================================
void printHelp() {
  Serial.println("\nCommands:");
  Serial.println(" help");
  Serial.println(" relay on <1-4>");
  Serial.println(" relay off <1-4>");
  Serial.println(" relay toggle <1-4>");
  Serial.println(" relay status");
  Serial.println(" opto status");
  Serial.println(" led on/off/toggle");
  Serial.println(" button");
  Serial.println(" demo start/stop");
  Serial.println(" status");
}

// ======================================================
// STATUS FUNCTIONS
// ======================================================
bool relayState(int i) {
  return digitalRead(relayPins[i]) == RELAY_ON;
}

void printRelayStatus() {
  Serial.println("Relays:");
  for (int i = 0; i < 4; i++) {
    Serial.printf("  R%d: %s\n", i + 1, relayState(i) ? "ON" : "OFF");
  }
}

void printOptoStatus() {
  Serial.println("Opto inputs:");
  for (int i = 0; i < 4; i++) {
    bool active = digitalRead(optoPins[i]) == LOW;
    Serial.printf("  IN%d: %s\n", i + 1, active ? "ACTIVE" : "IDLE");
  }
}

void printButton() {
  bool pressed = digitalRead(USER_BUTTON) == LOW;
  Serial.printf("Button: %s\n", pressed ? "PRESSED" : "RELEASED");
}

void printStatus() {
  printRelayStatus();
  printOptoStatus();
  printButton();
}

// ======================================================
// RELAY CONTROL
// ======================================================
void setRelay(int idx, bool state) {
  if (idx < 0 || idx > 3) return;
  digitalWrite(relayPins[idx], state ? RELAY_ON : RELAY_OFF);
}

// ======================================================
// COMMAND PARSER
// ======================================================
void processCommand(String cmd) {
  cmd.trim();

  if (cmd == "help") {
    printHelp();
  }

  else if (cmd.startsWith("relay")) {
    int num = cmd.substring(cmd.length() - 1).toInt() - 1;

    if (cmd.indexOf("on") > 0) setRelay(num, true);
    else if (cmd.indexOf("off") > 0) setRelay(num, false);
    else if (cmd.indexOf("toggle") > 0)
      setRelay(num, !relayState(num));
    else if (cmd.indexOf("status") > 0)
      printRelayStatus();
  }

  else if (cmd == "opto status") {
    printOptoStatus();
  }

  else if (cmd == "led on") {
    digitalWrite(USER_LED, LOW);
  }
  else if (cmd == "led off") {
    digitalWrite(USER_LED, HIGH);
  }
  else if (cmd == "led toggle") {
    digitalWrite(USER_LED, !digitalRead(USER_LED));
  }

  else if (cmd == "button") {
    printButton();
  }

  else if (cmd == "demo start") {
    demoMode = true;
    Serial.println("Demo started");
  }

  else if (cmd == "demo stop") {
    demoMode = false;
    Serial.println("Demo stopped");
  }

  else if (cmd == "status") {
    printStatus();
  }

  else {
    Serial.println("Unknown command");
  }
}

// ======================================================
// SERIAL INPUT HANDLER
// ======================================================
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      processCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

// ======================================================
// DEMO MODE
// ======================================================
void runDemo() {
  if (!demoMode) return;

  if (millis() - lastDemoTime > 2000) {
    lastDemoTime = millis();

    for (int i = 0; i < 4; i++)
      digitalWrite(relayPins[i], RELAY_OFF);

    digitalWrite(relayPins[demoIndex], RELAY_ON);

    digitalWrite(USER_LED, demoIndex % 2 ? LOW : HIGH);

    demoIndex = (demoIndex + 1) % 4;
  }
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  handleSerial();
  runDemo();
}
