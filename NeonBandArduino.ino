// NeonBand - Arduino Mega 2560, two HC-SR04 sensors.
// Serial protocol: newline-terminated events at 9600 baud.
const byte DRUM_TRIG = 4, DRUM_ECHO = 3;
const byte BASS_TRIG = 6, BASS_ECHO = 5;
const unsigned long PING_INTERVAL = 30; // stagger sensors to reduce interference
const char* const drumEvents[] = {"DRUM:KICK", "DRUM:CLAP", "DRUM:HAT"};
const char* const bassEvents[] = {"BASS:C", "BASS:F", "BASS:G", "BASS:A"};
int lastZone[2] = {-1, -1};
int candidate[2] = {-1, -1};
byte samples[2] = {0, 0};
byte sensor = 0;
unsigned long lastPing = 0;

float readDistance(byte trig, byte echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long duration = pulseIn(echo, HIGH, 6000UL);
  return duration ? duration * 0.0343f / 2.0f : -1.0f;
}

int getZone(float cm, byte count) {
  if (cm < 5.0f || cm >= 5.0f + 10.0f * count) return -1;
  return (int)((cm - 5.0f) / 10.0f);
}

void setup() {
  Serial.begin(9600);
  pinMode(DRUM_TRIG, OUTPUT); pinMode(DRUM_ECHO, INPUT);
  pinMode(BASS_TRIG, OUTPUT); pinMode(BASS_ECHO, INPUT);
  digitalWrite(DRUM_TRIG, LOW); digitalWrite(BASS_TRIG, LOW);
}

void loop() {
  if (millis() - lastPing < PING_INTERVAL) return;
  lastPing = millis();
  byte i = sensor;
  sensor = 1 - sensor;
  float cm = i == 0 ? readDistance(DRUM_TRIG, DRUM_ECHO)
                    : readDistance(BASS_TRIG, BASS_ECHO);
  int zone = getZone(cm, i == 0 ? 3 : 4);
  // Require two consecutive readings before accepting a zone change.
  if (zone != candidate[i]) {
    candidate[i] = zone;
    samples[i] = 1;
    return;
  }
  if (samples[i] < 2) samples[i]++;
  if (samples[i] < 2 || zone == lastZone[i]) return;
  lastZone[i] = zone;
  // Leaving the active range rearms the instrument without sending a note.
  if (zone >= 0) Serial.println(i == 0 ? drumEvents[zone] : bassEvents[zone]);
}
