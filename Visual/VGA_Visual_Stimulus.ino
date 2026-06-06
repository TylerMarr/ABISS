/*
  Wiring:
  D3  : HSYNC (68 ohm series)      -> DB15 pin 13
  D4  : RED   (470 ohm series)     -> DB15 pin 1
  D5  : GREEN (470 ohm series)     -> DB15 pin 2
  D6  : BLUE  (470 ohm series)     -> DB15 pin 3
  D10 : VSYNC (68 ohm series)      -> DB15 pin 14
  D8  : Camera trigger (20 Hz, frame-synced)
  D9  : Button (to GND), INPUT_PULLUP
  GND : -> DB15 pins 5, 6, 7, 8, 10
*/

#include "TimerHelpers.h"
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <string.h>

// ---------------- Basic pins ----------------
const byte hSyncPin = 3;
const byte redPin   = 4;
const byte greenPin = 5;
const byte bluePin  = 6;
const byte vSyncPin = 10;

// ---------------- Camera trigger (20 Hz) ----------------
const byte camPin           = 8;
const int  FPS              = 60;
const int  CAM_PULSE_MODULO = 3;   // 60/3 = 20 Hz

bool camActive   = false;
int  camFrameMod = 0;

#define CAM_PIN_HIGH  (PORTB |=  (1 << 0))
#define CAM_PIN_LOW   (PORTB &= ~(1 << 0))

inline void camStart() { camActive = true; camFrameMod = 0; }
inline void camStop()  { camActive = false; CAM_PIN_LOW; }

// Called every VGA frame (60Hz). Produces 20Hz pulses via its own counter.
inline void updateCameraTriggerFrameTick() {
  if (!camActive) { CAM_PIN_LOW; return; }
  if (camFrameMod == 0) CAM_PIN_HIGH;
  else                  CAM_PIN_LOW;
  camFrameMod = (camFrameMod + 1) % CAM_PULSE_MODULO;
}

// Camera: start only when bars actually move
bool armCamOnMoveStart = false;   // true -> start camera at first movement step

// ---------------- Button (press-to-start) ----------------
const byte buttonPin       = 9;     // D9 with INPUT_PULLUP (button to GND)
const int  DEBOUNCE_FRAMES = 3;     // ~50 ms at 60 FPS

bool buttonPressedEdgeFrame() {
  static int stableState = HIGH;
  static int lastStable  = HIGH;
  static int cnt = 0;
  int raw = digitalRead(buttonPin);
  if (raw == stableState) {
    cnt = 0;
  } else {
    if (++cnt >= DEBOUNCE_FRAMES) {
      stableState = raw;
      cnt = 0;
      if (lastStable == HIGH && stableState == LOW) {
        lastStable = stableState;
        return true; // pressed
      }
      lastStable = stableState;
    }
  }
  return false;
}

// ---------------- Resolution and buffer ----------------
// Each byte corresponds to 8 horizontal pixels
const int horizontalBytes  = 55;      // 55 * 8 = 440 pixels wide
const int verticalPixels   = 480;     // visible pixel height
const int verticalLines    = verticalPixels / 16; // 1 buffer row per 16 scanlines
const int horizontalPixels = horizontalBytes * 8;

// ---------------- Sync parameters ----------------
const byte verticalBackPorchLines  = 37;
const int  verticalFrontPorchLines = 525 - verticalBackPorchLines;

// ---------------- State variables ----------------
volatile int  vLine;
volatile int  messageLine;
volatile int  backPorchLinesToGo;
volatile byte newFrame;

#define nop asm volatile ("nop\n\t")

// Framebuffer: verticalLines rows × 55 bytes per row
uint8_t message[verticalLines][horizontalBytes];

// ---------------- Colors and directions ----------------
#define COLOR_WHITE   (7 << 4) //red is 1, green is 2, blue is 4, pinkish purple is 5, cyan is 6, white is 7
#define LEFT_TO_RIGHT 1
#define RIGHT_TO_LEFT 2
#define TOP_TO_BOTTOM 3
#define BOTTOM_TO_TOP 4

// ---------------- Checkerboard ----------------
const int H_SHIFT = 1;
const int V_SHIFT = 1;

// ---------------- Bar parameters (tweakable) ----------------
const int hThicknessBytes = 5;     // 5 bytes = 40 px wide (columns)
int       hSpacingBytes   = 61;    // distance between vertical bars in bytes //60 means the first bar must completely leave before second bar appears
const int vThicknessRows  = 3;     // 3 rows = 3*16 = 48 px tall (rows)
int       vSpacingRows    = 34;    // distance between horizontal bars in rows

float barSpeed;
float setBarSpeed  = 0.15625;  
int   barsPerBatch = 10;      // bars per batch
const int MAX_BARS = 10;      // do not set too large to avoid SRAM issues, max is around 30 (leaves 61 byte of memory for local variables
bool colorFlicker = true; // toggles whether the checkerboard flickers between black and white or not
bool checkerInvert = false; // DO NOT TOUCH, holds the current checkerboard pattern state (flipped or not frame to frame)
int flickerFrameCount = 0;
const int FLICKER_TOGGLE_FRAMES = 5;  // 60Hz / 5 = 12 toggles/sec = 6Hz full cycle

// ---- Phase & pair control ----
int  sweepsPerPhase = 1;   // how many sweeps per direction
int  numPairs       = 10;   // how many pairs; 0 = loop forever
bool stopAtEnd      = true;// clear & stop after finishing all pairs

// ---- Runtime state ----
int  sweepsDoneInPhase = 0;
int  pairsDone         = 0;
bool running           = false;    // start idle, wait for button

inline int iround(float x) { return (int)(x + (x >= 0 ? 0.5f : -0.5f)); }

// iround(60*10/3) = 200 logic ticks. 200/20Hz = 10 seconds. Same wall time.
int  interPairPauseFrames     = iround(FPS * 10.0f / 3.0f);
bool interPairHold            = false;
int  interPairFramesRemaining = 0;
bool needInitAfterPairPause   = false;

// ----- Inter-phase pause between forward and reverse (inside one pair) -----
int  interPhasePauseFrames     = iround(FPS * 10.0f / 3.0f);
bool interPhaseHold            = false;           // when true, we're in the inter-phase pause
int  interPhaseFramesRemaining = 0;
bool needInitAfterPause        = false;           // delay initBatch() until pause ends

// ---------------- Direction ----------------
int direction = LEFT_TO_RIGHT;  // starting "forward" direction
//int direction = TOP_TO_BOTTOM;

inline int oppositeDir(int d) {
  if (d == LEFT_TO_RIGHT) return RIGHT_TO_LEFT;
  if (d == RIGHT_TO_LEFT) return LEFT_TO_RIGHT;
  if (d == TOP_TO_BOTTOM) return BOTTOM_TO_TOP;
  return TOP_TO_BOTTOM;
}
inline bool isForwardDir(int d) { return d == LEFT_TO_RIGHT || d == TOP_TO_BOTTOM; }
inline bool isReverseDir(int d) { return d == RIGHT_TO_LEFT || d == BOTTOM_TO_TOP; }
inline bool isHorizontal()      { return direction == LEFT_TO_RIGHT || direction == RIGHT_TO_LEFT; }
inline bool isVertical()        { return direction == TOP_TO_BOTTOM || direction == BOTTOM_TO_TOP; }

// Track the head position of each bar, in bytes (H) or rows (V)
float barPositions[MAX_BARS];

// ---------------- Prototypes ----------------
void    initBatch();
void    drawBarsToBuffer();
bool    isBatchFinished();
boolean doOneScanLine();
void    advanceFrame();

// ---------------- Helpers ----------------
inline void clearFrameBuffer() {
  for (int y = 0; y < verticalLines; y++) memset(message[y], 0, horizontalBytes);
}

// ---------------- ISRs ----------------
ISR (TIMER1_OVF_vect) { // Vertical sync
  vLine = 0;
  messageLine = 0;
  backPorchLinesToGo = verticalBackPorchLines;
  newFrame = true;
}
ISR (TIMER2_OVF_vect) { // Horizontal sync
  backPorchLinesToGo--;
}

// ---------------- Setup ----------------
void setup() {
  // Disable Timer0 (keep VGA timing clean)
  TIMSK0 = 0; OCR0A = 0; OCR0B = 0;

  // VSYNC with Timer1
  pinMode(vSyncPin, OUTPUT);
  Timer1::setMode(15, Timer1::PRESCALE_1024, Timer1::CLEAR_B_ON_COMPARE);
  OCR1A = 260; OCR1B = 0;
  TIFR1 = bit(TOV1);
  TIMSK1 = bit(TOIE1);

  // HSYNC with Timer2
  pinMode(hSyncPin, OUTPUT);
  Timer2::setMode(7, Timer2::PRESCALE_8, Timer2::CLEAR_B_ON_COMPARE);
  OCR2A = 63; OCR2B = 7;
  TIFR2 = bit(TOV2);
  TIMSK2 = bit(TOIE2);

  set_sleep_mode(SLEEP_MODE_IDLE);

  pinMode(redPin,   OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin,  OUTPUT);

  // Camera trigger & Button
  pinMode(camPin,   OUTPUT);
  digitalWrite(camPin, LOW);
  pinMode(buttonPin, INPUT_PULLUP);

  // Clear screen
  clearFrameBuffer();

  // Init batch geometry
  if (barsPerBatch > MAX_BARS) barsPerBatch = MAX_BARS;
  initBatch();

  // Start in IDLE; wait for button
  running = false;
  camStop();
}

// ---------------- Render one visible scanline ----------------
boolean doOneScanLine() {
  if (backPorchLinesToGo > 0) return false;       // wait for back porch
  if (vLine == verticalPixels)  return newFrame;  // end of visible -> frame done

  int row = vLine >> 4;                            // vLine / 16
  if (row >= verticalLines) row = verticalLines - 1;
  char* messagePtr = &message[row][0];

  delayMicroseconds(1);
  for (byte i = 0; i < horizontalBytes; i++) {
    PORTD = (*messagePtr++);
  }
  nop; nop; nop;
  PORTD = 0;

  vLine++;
  return false;
}

// ---------------- Initialize a batch of bars ----------------
void initBatch() {
  barSpeed = setBarSpeed;

  int head, spacing;
  if (isHorizontal()) {
    head    = (direction == LEFT_TO_RIGHT) ? -hThicknessBytes : (horizontalBytes - 1 + hThicknessBytes);
    spacing = hSpacingBytes;
  } else {
    head    = (direction == TOP_TO_BOTTOM) ? -vThicknessRows : (verticalLines - 1 + vThicknessRows);
    spacing = vSpacingRows;
  }
  for (int i = 0; i < barsPerBatch; i++) {
    if (isHorizontal()) {
      barPositions[i] = (direction == LEFT_TO_RIGHT) ? (float)(head - i * spacing)
                                                     : (float)(head + i * spacing);
    } else {
      barPositions[i] = (direction == TOP_TO_BOTTOM) ? (float)(head - i * spacing)
                                                     : (float)(head + i * spacing);
    }
  }
}

// ---------------- Draw bars into framebuffer ----------------
void drawBarsToBuffer() {
  clearFrameBuffer();

  for (int b = 0; b < barsPerBatch; b++) {
    int pos = (int)(barPositions[b]);

    if (isHorizontal()) {
      for (int t = 0; t < hThicknessBytes; t++) {
        int x = (direction == LEFT_TO_RIGHT) ? (pos + t) : (pos - t);
        if (x >= 0 && x < horizontalBytes) {
          for (int y = 0; y < verticalLines; y++) {
            if (((((y >> V_SHIFT) + (x >> H_SHIFT)) & 1) ^ checkerInvert) == 0) {
              message[y][x] = COLOR_WHITE;
            } else {
              message[y][x] = 0x00;
            }
          }
        }
      }
    } else {
      for (int t = 0; t < vThicknessRows; t++) {
        int y = (direction == TOP_TO_BOTTOM) ? (pos + t) : (pos - t);
        if (y >= 0 && y < verticalLines) {
          for (int x = 0; x < horizontalBytes; x++) {
            if (((((y >> V_SHIFT) + (x >> H_SHIFT)) & 1) ^ checkerInvert) == 0) {
              message[y][x] = COLOR_WHITE;
            } else {
              message[y][x] = 0x00;
            }
          }
        }
      }
    }
  }
}

// ---------------- Check if the whole batch has left the screen ----------------
bool isBatchFinished() {
  if (isHorizontal()) {
    if (direction == LEFT_TO_RIGHT) {
      for (int i = 0; i < barsPerBatch; i++) if (barPositions[i] < horizontalBytes) return false;
      return true;
    } else {
      for (int i = 0; i < barsPerBatch; i++) if (barPositions[i] >= 0) return false;
      return true;
    }
  } else {
    if (direction == TOP_TO_BOTTOM) {
      for (int i = 0; i < barsPerBatch; i++) if (barPositions[i] < verticalLines) return false;
      return true;
    } else {
      for (int i = 0; i < barsPerBatch; i++) if (barPositions[i] >= -vThicknessRows) return false;
      return true;
    }
  }
}

// ---------------- Per-logic-tick update ----------------
void advanceFrame() {
  if (!running) return;  // <-- removed "newFrame = false" here

  // 1) Inter-phase pause (between forward and reverse within a pair)
  if (interPhaseHold) {
    if (interPhaseFramesRemaining > 0) {
      interPhaseFramesRemaining--;
      return;  // <-- removed "newFrame = false" here; keep holding; camera stays off
    } else {
      interPhaseHold = false;
      if (needInitAfterPause) {
        needInitAfterPause = false;
        armCamOnMoveStart  = true;  // first movement frame starts camera
        initBatch();
      }
    }
  }

  // 1b) Inter-pair pause (between completed pairs)
  if (interPairHold) {
    if (interPairFramesRemaining > 0) {
      interPairFramesRemaining--;
      return; 
    } else {
      interPairHold = false;
      if (needInitAfterPairPause) {
        needInitAfterPairPause = false;
        armCamOnMoveStart      = true; // start camera exactly on first movement frame
        initBatch();
      }
    }
  }

  // If armed, begin camera trigger exactly when movement resumes
  if (armCamOnMoveStart) { camStart(); armCamOnMoveStart = false; }

  float delta = isForwardDir(direction) ? barSpeed : -barSpeed;
  for (int i = 0; i < barsPerBatch; i++) barPositions[i] += delta;
  drawBarsToBuffer();

  // 3) End-of-batch handling
  if (isBatchFinished()) {
    sweepsDoneInPhase++;

    if (sweepsDoneInPhase < sweepsPerPhase) {
      initBatch();
    } else {
      // Phase done -> flip direction
      sweepsDoneInPhase = 0;
      int  newDir        = oppositeDir(direction);
      bool completedPair = ((direction == RIGHT_TO_LEFT && newDir == LEFT_TO_RIGHT) ||
                            (direction == BOTTOM_TO_TOP && newDir == TOP_TO_BOTTOM));

      camStop();     // stop camera at end of any phase
      direction = newDir;

      if (completedPair) {
        // End of pair
        pairsDone++;
        if (numPairs > 0 && pairsDone >= numPairs) {
          running = false;
          if (stopAtEnd) clearFrameBuffer();
          return; 
        }

        if (interPairPauseFrames > 0) {
          interPairHold            = true;
          interPairFramesRemaining = interPairPauseFrames;
          needInitAfterPairPause   = true;
          clearFrameBuffer();   // blank screen during inter-pair gap
          camStop();            // camera stays off during the gap
        } else {
          // No gap: start next pair immediately
          armCamOnMoveStart = true;
          initBatch();
        }
      } else {
        // Forward finished → (optional) gap → Reverse
        if (interPhasePauseFrames > 0) {
          interPhaseHold            = true;
          interPhaseFramesRemaining = interPhasePauseFrames;
          needInitAfterPause        = true;
          clearFrameBuffer();
        } else {
          armCamOnMoveStart = true;
          initBatch();
        }
      }
    }
  }
}

// ---------------- Main loop ----------------
void loop() {
  static byte logicTick = 0;

  while (true) {
    sleep_mode();                 // Wait for ISR wakeup
    if (doOneScanLine()) {
      newFrame = false;

      if (colorFlicker && running) {
        flickerFrameCount++;
        if (flickerFrameCount >= FLICKER_TOGGLE_FRAMES) {
          checkerInvert = !checkerInvert;
          flickerFrameCount = 0;
          drawBarsToBuffer();   // redraw immediately with new polarity
        }
      }

      updateCameraTriggerFrameTick();   // runs every VGA frame -> 20Hz output

      // Start on button press (only when idle)
      if (!running && buttonPressedEdgeFrame()) {
        sweepsDoneInPhase = 0;
        pairsDone         = 0;
        armCamOnMoveStart = true;   // start camera on first movement frame
        initBatch();
        running = true;
      }

      // Heavy logic every 3rd VGA frame = 20Hz
      logicTick++;
      if (logicTick >= CAM_PULSE_MODULO) {
        logicTick = 0;
        advanceFrame();
      }
    }
  }
}
