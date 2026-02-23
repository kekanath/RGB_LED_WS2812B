/*
   ==========================================================
   ESP32 LED Controller
   - Animation 1 : Ring Pulse
   - Animation 2 : Cinematic Pulse (Smooth BPM Based)
   - Serial Controlled
   - Non Blocking
   ==========================================================
*/

#include <FastLED.h>

// ================= LED CONFIG =================
#define LED_PIN       3
#define NUM_LEDS      37
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB     // IMPORTANT: WS2812B = GRB
#define BRIGHTNESS    140

CRGB leds[NUM_LEDS];

// ================= RING LAYOUT =================
const uint8_t outerRing[]  = {15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
const uint8_t middleRing[] = {18,19,20,21,22,23,24,25,26,27,16,17};
const uint8_t innerRing[]  = {28,35,34,33,32,31,30,29};
const uint8_t centerLED[]  = {36};

#define OUTER_COUNT  16
#define MIDDLE_COUNT 12
#define INNER_COUNT  8

// ================= MODE =================
enum Mode { STOPPED, RING_PULSE, CINEMATIC };
Mode currentMode = STOPPED;

// ================= SPEED =================
uint8_t speedLevel = 3;
unsigned long lastUpdate = 0;
unsigned long intervalMs = 40;

void updateSpeed() {
  switch (speedLevel) {
    case 1: intervalMs = 120; break;
    case 2: intervalMs = 80;  break;
    case 3: intervalMs = 40;  break;
    case 4: intervalMs = 20;  break;
    case 5: intervalMs = 8;   break;
  }
}

// ================= SHARED STATE =================
uint8_t baseHue = 0;
uint8_t stepIndex = 0;
bool reverseDirection = false;

// ======================================================
void resetAnimationState() {
  stepIndex = 0;
  reverseDirection = false;
  baseHue = 0;
  FastLED.clear();
  FastLED.show();
}

// ======================================================
void setup() {

  Serial.begin(115200);
  delay(1000);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  updateSpeed();
  printMenu();
}

// ======================================================
void loop() {

  handleSerial();

  if (currentMode == STOPPED) return;

  // Ring Pulse uses interval timing
  if (currentMode == RING_PULSE) {
    if (millis() - lastUpdate >= intervalMs) {
      lastUpdate = millis();
      ringPulseAnimation();
    }
  }

  // Cinematic runs continuously (smooth)
  else if (currentMode == CINEMATIC) {
    cinematicPulseCycle();
  }
}

// ======================================================
void printMenu() {

  Serial.println("\n===== LED MENU =====");
  Serial.println("1 → Ring Pulse");
  Serial.println("2 → Cinematic Pulse");
  Serial.println("0 → Stop");
  Serial.println("S1–S5 → Speed Levels");
  Serial.println("+ / - → Speed Adjust");
  Serial.println("====================");
}

// ======================================================
void handleSerial() {

  while (Serial.available()) {

    char c = Serial.read();

    if (c == '\n' || c == '\r') continue;

    switch (c) {

      case '1':
        currentMode = RING_PULSE;
        resetAnimationState();
        Serial.println("Ring Pulse Selected");
        break;

      case '2':
        currentMode = CINEMATIC;
        resetAnimationState();
        Serial.println("Cinematic Selected");
        break;

      case '0':
        currentMode = STOPPED;
        FastLED.clear();
        FastLED.show();
        Serial.println("Stopped");
        break;

      case '+':
        if (speedLevel < 5) speedLevel++;
        updateSpeed();
        Serial.print("Speed Level: ");
        Serial.println(speedLevel);
        break;

      case '-':
        if (speedLevel > 1) speedLevel--;
        updateSpeed();
        Serial.print("Speed Level: ");
        Serial.println(speedLevel);
        break;

      case 'S':
        while (!Serial.available());
        char level = Serial.read();
        if (level >= '1' && level <= '5') {
          speedLevel = level - '0';
          updateSpeed();
          Serial.print("Speed Set To: ");
          Serial.println(speedLevel);
        }
        break;
    }
  }
}

// ======================================================
// ================= ANIMATION 1 =================
// ======================================================
void ringPulseAnimation() {

  FastLED.clear();
  CHSV color(baseHue, 255, 255);

  if (!reverseDirection) {

    if (stepIndex == 0)
      leds[centerLED[0]] = color;

    else if (stepIndex == 1)
      for (int i = 0; i < INNER_COUNT; i++)
        leds[innerRing[i]] = color;

    else if (stepIndex == 2)
      for (int i = 0; i < MIDDLE_COUNT; i++)
        leds[middleRing[i]] = color;

    else if (stepIndex == 3)
      for (int i = 0; i < OUTER_COUNT; i++)
        leds[outerRing[i]] = color;

  } else {

    if (stepIndex == 0)
      for (int i = 0; i < OUTER_COUNT; i++)
        leds[outerRing[i]] = color;

    else if (stepIndex == 1)
      for (int i = 0; i < MIDDLE_COUNT; i++)
        leds[middleRing[i]] = color;

    else if (stepIndex == 2)
      for (int i = 0; i < INNER_COUNT; i++)
        leds[innerRing[i]] = color;

    else if (stepIndex == 3)
      leds[centerLED[0]] = color;
  }

  FastLED.show();

  stepIndex++;

  if (stepIndex > 3) {
    stepIndex = 0;
    reverseDirection = !reverseDirection;
    baseHue += 20;
  }
}

// ======================================================
// ================= ANIMATION 2 =================
// ======================================================
void cinematicPulseCycle() {

  // Smooth fading background
  fadeToBlackBy(leds, NUM_LEDS, 20);

  // Speed-based breathing BPM
  uint8_t bpm;

  switch(speedLevel) {
    case 1: bpm = 6;  break;
    case 2: bpm = 10; break;
    case 3: bpm = 16; break;
    case 4: bpm = 25; break;
    case 5: bpm = 40; break;
  }

  uint8_t wave = beatsin8(bpm, 40, 255);

  // Layered cinematic glow
  for (int i = 0; i < OUTER_COUNT; i++)
    leds[outerRing[i]] += CHSV(baseHue, 180, wave * 0.6);

  for (int i = 0; i < MIDDLE_COUNT; i++)
    leds[middleRing[i]] += CHSV(baseHue + 20, 200, wave * 0.8);

  for (int i = 0; i < INNER_COUNT; i++)
    leds[innerRing[i]] += CHSV(baseHue + 40, 255, wave);

  leds[centerLED[0]] = CHSV(baseHue + 80, 255, wave);

  FastLED.show();

  baseHue++;
}