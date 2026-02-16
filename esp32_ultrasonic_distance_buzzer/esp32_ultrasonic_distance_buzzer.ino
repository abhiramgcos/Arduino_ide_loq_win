// ESP32 (side pins only) + HC-SR04 + Buzzer

#define TRIG_PIN   27   // side pin GPIO27
#define ECHO_PIN   26   // side pin GPIO26
#define BUZZER_PIN 25   // side pin GPIO25

const float MAX_DISTANCE = 150.0;
const float MIN_DISTANCE = 10.0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1.0;

  float distance = duration * 0.034 / 2.0;
  return distance;
}

void loop() {
  float d = getDistanceCm();
  Serial.print("Distance: ");
  Serial.print(d);
  Serial.println(" cm");

  // Invalid or out of max range -> OFF
  if (d < 0 || d > MAX_DISTANCE) {
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    return;
  }

  // 0 < d <= 10 cm -> continuous ON
  if (d > 0 && d <= 10.0) {
    digitalWrite(BUZZER_PIN, HIGH);
    // small delay to avoid hammering sensor
    delay(50);
    return;
  }

  // d > 10 cm -> distance-based beeps
  int minDelay = 50;
  int maxDelay = 600;

  float ratio = (d - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE);
  if (ratio < 0) ratio = 0;
  if (ratio > 1) ratio = 1;

  int beepDelay = minDelay + (int)((maxDelay - minDelay) * ratio);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);          // beep duration
  digitalWrite(BUZZER_PIN, LOW);
  delay(beepDelay);   // gap between beeps
}
