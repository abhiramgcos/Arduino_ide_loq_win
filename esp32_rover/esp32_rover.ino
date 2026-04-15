// esp32_rover.ino
// Firmware for ESP32 to control 2 DC motors via USB Serial (No PWM)

// Motor A (Left)
const int IN1 = 26; // Forward
const int IN2 = 27; // Backward

// Motor B (Right)
const int IN3 = 14; // Forward
const int IN4 = 12; // Backward

// Safety Timeout
unsigned long lastCommandTime = 0;
const unsigned long TIMEOUT_MS = 1000; // 1 second timeout for safety

void setup() {
  Serial.begin(115200);

  // Configure Motor Control Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initial Stop
  stopMotors();
  Serial.println("ESP32 Rover Init. Waiting for L<speed> R<speed> commands.");
  Serial.println("Note: Operating in 100% speed mode (No PWM).");
}

void loop() {
  // Read from Serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    parseAndSetSpeeds(command);
    lastCommandTime = millis();
  }

  // Safety cut-off if no command is received
  if (millis() - lastCommandTime > TIMEOUT_MS) {
    stopMotors();
  }
}

void parseAndSetSpeeds(String cmd) {
  // Expected format: L200 R150 or L-200 R-150
  int lIndex = cmd.indexOf('L');
  int rIndex = cmd.indexOf('R');
  
  if (lIndex != -1 && rIndex != -1) {
    int spaceIndex = cmd.indexOf(' ', lIndex);
    if (spaceIndex == -1) // Should have a space
      return;
      
    String lStr = cmd.substring(lIndex + 1, spaceIndex);
    String rStr = cmd.substring(rIndex + 1);
    
    int leftSpeed = lStr.toInt();
    int rightSpeed = rStr.toInt();
    
    setMotorSpeeds(leftSpeed, rightSpeed);
  }
}

void setMotorSpeeds(int leftSpeed, int rightSpeed) {
  // Since PWM is disabled, any speed > 0 is full forward.
  // Any speed < 0 is full backward. 0 is stop.

  // Set Left Motor
  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // Set Right Motor
  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
