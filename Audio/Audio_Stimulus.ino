#include <Arduino.h>
#include "AD9833.h"
#include <pt.h> //include protothreads library on arduino
#include <Wire.h>
#include "PT2258.h"

// --------------------- Pin Definitions ----------------------
#define FNC_PIN 2 //tone generation
#define LED_PIN 8 //actually the camera! not LED
#define BUTTON_PIN 9 //input for start/stop

// --------------------- Device Objects ------------------------
PT2258 pt2258(0x8C); // PT2258 Object
AD9833 waveGenerator(FNC_PIN);

// -------------------- Global Variables -----------------------
int frequencyList[] = {3000, 10000, 30000};
int volumeList[] = {20,12,10}; // Default volume / Starting Volume // lower number means louder
int blinkCount = 0; //keeps track of time for the tone generating part
int frequencyIndex = 0; //keeps track of which frequency we're using
bool isRunning = false; //keeps track of if the tone is actively being generated or not
unsigned long startTime = 0;

volatile unsigned long cycleStartTime = 0;
volatile bool newCycleReady = false;

// -------------------- Timing Customization --------------------
double delayUntilStimulus = 2.0; //keeps track of user input for time offset from cam start to tone start, default 2.0
double stimulusTime = 2.0; //keeps track of how long we play stimulus for, default 2.0
double delayAfterStimulus = 2.0; //keeps track of how long we wait after stimulus to turn cam off, default 2.0
double restTimeInSeconds = 30.0; //keeps track of how long we pause between blinking sequences, default 3.0
double blinksPerSecond = 20.0; //keeps track of how many times we blink per second, int for simplicity, default 20.0
double buttonToStartTime = 1.0; //keeps track of how long we wait after a button press to really start our system, default 1.0
int numberOfRampUpSteps = 150; //keeps track of how many steps we take in the ramp-up process, default 150
int numberOfRampDownSteps = 150; //keeps track of how many steps we take in the ramp-down process, default 150
double stepIntervalInSeconds = 0.0005; //keeps track of how long we wait between steps in the ramp process, default 0.0005


// Seconds to Blinks Conversions or Seconds to Millis/Micros Conversions
double blinkingDurationInSeconds = delayUntilStimulus + stimulusTime + delayAfterStimulus; //keeps track of how long we want to blink for in seconds
int delayUntilStimulusBlinks = (int)(delayUntilStimulus * blinksPerSecond); //1 second = blinksPerSecond blinks, casting to (int) rounds to nearest whole number for blinks
int delayAfterStimulusBlinks = (int)(delayAfterStimulus * blinksPerSecond); //1 second = blinksPerSecond blinks, casting to (int) rounds to nearest whole number for blinks
int blinkingDurationInBlinks = (int)((blinkingDurationInSeconds * blinksPerSecond) - 1); //1 second = blinksPerSecond blinks, minus 1 for zero-indexing
double restTimeInMicros = restTimeInSeconds*1000000; //rest time in microseconds
double secondsPerBlinkInMicros = (1/blinksPerSecond)*1000000; //for blinking math
double buttonToStartTimeInMillis = buttonToStartTime * 1000;
double stepIntervalInMicros = stepIntervalInSeconds*1000000; //step time in microseconds


// Protothread control blocks
static struct pt ptBlinkLED, ptGenerateTone;

// Protothread for blinking LED
static int blinkLED(struct pt *pt) { 
    static unsigned long lastTime = 0;
    PT_BEGIN(pt);
    while (1) { //always running
        digitalWrite(LED_PIN, HIGH);
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > (secondsPerBlinkInMicros*0.2));
        digitalWrite(LED_PIN, LOW);
        blinkCount++;
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > (secondsPerBlinkInMicros*0.8)); 
        if (blinkCount > blinkingDurationInBlinks) {  
            lastTime = micros();
            PT_WAIT_UNTIL(pt, micros() - lastTime > restTimeInMicros);
            cycleStartTime = micros();
            newCycleReady = true;
            blinkCount = 0;
        }
    } 
    PT_END(pt);
}

// -----------------------------------------------------------------------------
// Protothread: Tone Generator
// -----------------------------------------------------------------------------
static int generateTone(struct pt *pt) {
    static unsigned long lastTime = 0;
    static unsigned long target = 0; //NEW
    static int i = 0;
    static double targetVolume = 0;
    static double volumeRange = 0.0;
    static double stepSize = 0.0;
    static double stepper = 0.0;
    static int currentFrequency = 0;
    static int currentVolume = 0;
    
    PT_BEGIN(pt);
    
    while (1) {
        if(cycleStartTime == 0) {
          startTime = micros();
          cycleStartTime = 1;
        }
        else {
          PT_WAIT_UNTIL(pt, newCycleReady);
          startTime = cycleStartTime;
          newCycleReady = false;
        }
        
        //pt2258.attenuation(1, min(volumeList[frequencyIndex % 3]+50,79)); //min volume
        currentFrequency = frequencyList[frequencyIndex % 3];
        currentVolume = volumeList[frequencyIndex % 3];
        pt2258.attenuation(1, 79); //start muted
        waveGenerator.ApplySignal(SINE_WAVE, REG0, currentFrequency);
        //PT_WAIT_UNTIL(pt, blinkCount > delayUntilStimulusBlinks); //wait 30 blinks + offset before enabling tone generation, 1.5 seconds
        target = startTime + (delayUntilStimulus * 1000000);
        PT_WAIT_UNTIL(pt, micros() > target);
        waveGenerator.EnableOutput(true); //turns on tone generation
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros); //this waits stepIntervalInMicros seconds for waveGenerator to turn on

        //ramp up
        targetVolume = (double)currentVolume;
        volumeRange = 79.0 - targetVolume;
        stepSize = volumeRange / numberOfRampUpSteps;
        for(i = 0; i < numberOfRampUpSteps; i++) {
          stepper = 79.0 - (stepSize * i);
          pt2258.attenuation(1, max((int)stepper, (int)targetVolume));
          lastTime = micros();
          PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros);
        }
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros);
        pt2258.attenuation(1, (int)targetVolume);
        //PT_WAIT_UNTIL(pt, blinkCount > (blinkingDurationInBlinks - delayAfterStimulusBlinks)); //plays at max volume here
        target = startTime + ((blinkingDurationInSeconds-delayAfterStimulus) * 1000000);
        PT_WAIT_UNTIL(pt, micros() > target);
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros);

        //ramp down
        stepSize = volumeRange / numberOfRampDownSteps;
        for(i = 0; i < numberOfRampDownSteps; i++) {
          stepper = targetVolume + (stepSize * i);
          pt2258.attenuation(1, min((int)stepper, 79));
          lastTime = micros();
          PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros);
        }

        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > stepIntervalInMicros);
        pt2258.attenuation(1, 79);
        waveGenerator.EnableOutput(false); //turns off tone generation
        
        //PT_WAIT_UNTIL(pt, blinkCount > blinkingDurationInBlinks); //wait the rest of the duration before stopping
        target = startTime + (blinkingDurationInSeconds * 1000000);
        PT_WAIT_UNTIL(pt, micros() > target);
        lastTime = micros();
        PT_WAIT_UNTIL(pt, micros() - lastTime > restTimeInMicros); //chill for user input seconds
        if (blinkCount >= (blinkingDurationInBlinks + 1)) {
            blinkCount = 0;
        }
        frequencyIndex++; //move to next frequency to generate
    }
    PT_END(pt);
}

void setup() {
    waveGenerator.Begin();
    pinMode(LED_PIN, OUTPUT);
    pinMode(FNC_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    waveGenerator.EnableOutput(false);
    digitalWrite(LED_PIN, LOW);
    PT_INIT(&ptBlinkLED);
    PT_INIT(&ptGenerateTone);
    Wire.setClock(400000); // setting the I2C clock to 400KHz, Fast Mode
    /* Initiating PT with default volume and Pin*/
    pt2258.mute(false);
    pt2258.attenuation(1, 79);
}

void loop() {
    int currentButtonState = digitalRead(BUTTON_PIN);
    static int previousButtonState = HIGH;
    if (frequencyIndex == 30) { //30 cycles of our program
        waveGenerator.EnableOutput(false);
        digitalWrite(LED_PIN, LOW);
        previousButtonState = HIGH;
        isRunning = !isRunning;
        blinkCount = 0;
        frequencyIndex = 0;
        return;
    }

    if (currentButtonState == LOW && previousButtonState == HIGH) {
        delay(50); // Debounce delay
        if (digitalRead(BUTTON_PIN) == LOW) { 
            delay(buttonToStartTimeInMillis); //waits user input seconds after button press to actually start
            isRunning = !isRunning;
        }
    }

    if (isRunning) {
        //waveGenerator.EnableOutput(true); //uncomment this when we want to actually output noise!
        blinkLED(&ptBlinkLED); 
        generateTone(&ptGenerateTone);
    }

    previousButtonState = currentButtonState;
}
