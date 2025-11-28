#include <Bluepad32.h>
#include <ESP32Servo.h>

// === Pin Definitions ===
const int IN1 = 27; //For directional motor control
const int IN2 = 26; //For directional motor control
const int STBY = 14;    // TB6612 standby pin
const int SERVO_PIN = 32; //requires pwm capabilities
const int A_BUTTON_LIGHT_PIN = 21;   // Change to any available pin
const int REVERSE_LIGHT_PIN = 22;    // Change to any available pin

//bool code
bool isALightOn = false;
bool wasAButtonPressed = false;

// === Objects ===
Servo myServo;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// === Bluepad32 Callbacks ===
void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      Serial.printf("Controller connected at index=%d\n", i);
      return;
    }
  }
  Serial.println("No available slot for new controller");
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      Serial.printf("Controller disconnected from index=%d\n", i);
      return;
    }
  }
}

// === Gamepad Input Handling ===
void processGamepad(ControllerPtr ctl) {
  // Toggle A button light
bool isAButtonPressed = ctl->a();   //might have to change the ctl->a to ctl->#, #=some letter/number phrase idk

if (isAButtonPressed && !wasAButtonPressed) {
  isALightOn = !isALightOn;
  digitalWrite(A_BUTTON_LIGHT_PIN, isALightOn ? HIGH : LOW);
}
wasAButtonPressed = isAButtonPressed;
  

  // === Servo control: Left joystick X (-511 to 512) ===
  int joyX = ctl->axisX();
  int angle = map(joyX, -511, 512, 0, 180);
  angle = constrain(angle, 70, 110);
  myServo.write(angle);

  // === Motor control ===
  int throttle = ctl->throttle(); // right trigger (0–1023)
  int brake = ctl->brake();       // left trigger (0–1023)
  const int threshold = 100;      // deadzone threshold

  if (throttle > threshold && brake > threshold) {
  // Both pressed: stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(REVERSE_LIGHT_PIN, LOW); // no reverse
} else if (throttle > threshold) {
  // Forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(REVERSE_LIGHT_PIN, LOW); // no reverse
} else if (brake > threshold) {
  // Reverse
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(REVERSE_LIGHT_PIN, HIGH); // reverse light ON
} else {
  // Nothing pressed: stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(REVERSE_LIGHT_PIN, LOW); // no reverse
}

  // Optional: Print debug info
  // Serial.printf("X: %d -> Angle: %d | Throttle: %d | Brake: %d\n", joyX, angle, throttle, brake);
}

void processControllers() {
  for (auto ctl : myControllers) {
    if (ctl && ctl->isConnected() && ctl->hasData()) {
      processGamepad(ctl);
    }
  }
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  myServo.attach(SERVO_PIN);

  pinMode(A_BUTTON_LIGHT_PIN, OUTPUT);
  pinMode(REVERSE_LIGHT_PIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH); // Enable motor driver

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys(); // Optional: only use if you want to unpair all devices
  BP32.enableVirtualDevice(false);

  Serial.println("Setup complete. Waiting for controller...");
}

// === Main loop ===
void loop() {
  bool updated = BP32.update();
  if (updated) {
    processControllers();
  }

  delay(10); // Keep CPU happy
}
