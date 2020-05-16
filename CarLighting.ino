#include "FastLED.h"
#include <Encoder.h>
#include <EEPROM.h>

#define EncoderClock 2
#define EncoderData 3
#define EncoderButton 4
#define EncoderPower 5
#define BuzzerMinus 11
#define BuzzerPlus 12
#define LEDData A5
#define EncoderButtonLogic LOW
#define LEDs 11

#define OffState 0
#define OnState 1
#define ColorState 2
#define BrightnessState 3

#define ShortPressTime 40
#define MaxShortPressTime 400
#define LongPressTime 1200
#define MaxLongPressTime 1500
#define ButtonTimeout 1500

#define EEPROMState 0
#define EEPROMHue 1
#define EEPROMBrightness 2

//#define StepsPerSecond 20 // every 50ms TODO: Utilize it
#define ClicksPerStep 1

CRGB leds[LEDs];
Encoder knob(EncoderData, EncoderClock);
uint8_t CurrentHue = 0;
uint8_t CurrentBrightness = 255;
uint8_t CurrentState = OnState;
int8_t EncoderPosition  = 0;
bool Clockwise = true;

void SetLEDs() {
	for (int i = 0; i < LEDs; i++) {
		leds[i] = CHSV(CurrentHue, 255, 255);
	}
	FastLED.show();
}

void Beep() {
	digitalWrite(BuzzerPlus, HIGH);
	delay(25);
	digitalWrite(BuzzerPlus, LOW);
}

void LongBeep() {
	digitalWrite(BuzzerPlus, HIGH);
	delay(250);
	digitalWrite(BuzzerPlus, LOW);
}

void setup() {
	pinMode(BuzzerMinus, OUTPUT);
	digitalWrite(BuzzerMinus, LOW);
	pinMode(BuzzerPlus, OUTPUT);
	digitalWrite(BuzzerPlus, LOW);
	pinMode(EncoderPower, OUTPUT);
	digitalWrite(EncoderPower, HIGH);
	pinMode(EncoderButton, INPUT_PULLUP);
	knob.write(0);
	FastLED.addLeds<NEOPIXEL, LEDData>(leds, LEDs);
	CurrentState = EEPROM.read(EEPROMState);
	CurrentHue = EEPROM.read(EEPROMHue);
	CurrentBrightness = EEPROM.read(EEPROMBrightness);
	FastLED.setBrightness(CurrentState == OffState ? 0 : CurrentBrightness);
	SetLEDs();
}

void ShortPress() {
	// Switch between states
	if (CurrentState == OffState) {
		CurrentState = OnState;
		FastLED.setBrightness(CurrentBrightness);
	FastLED.show();
		EEPROM.update(EEPROMState, CurrentState);
	} else if (CurrentState == OnState) {
		CurrentState = OffState;
		FastLED.setBrightness(0);
	FastLED.show();
		EEPROM.update(EEPROMState, CurrentState);
	} else if (CurrentState == ColorState) {
		CurrentState = BrightnessState;
	} else if (CurrentState == BrightnessState) {
		CurrentState = ColorState;
	} else {
		CurrentState = OnState;
		FastLED.setBrightness(CurrentBrightness);
	}
  Beep();
}

void LongPress() {
	// Activate or deactivate color editing pinMode
	if (CurrentState == OffState) {
		CurrentState = ColorState;
		FastLED.setBrightness(CurrentBrightness);
	} else if (CurrentState == OnState) {
		CurrentState = ColorState;
	} else if (CurrentState == ColorState || CurrentState == BrightnessState) {
		CurrentState = OnState;
		// Store colors to EEPROM TODO:
		EEPROM.update(EEPROMHue, CurrentHue);
		EEPROM.update(EEPROMBrightness, CurrentBrightness);
	}
  LongBeep();
}

void HandleButton() {
	// Reset button press timing
	static unsigned long LastButtonPressed = 0;
	static bool PreviousButtonState = false;
	// Consider button locked to avoid booting with button pressed
	static bool ButtonLocked = true;
	// If button is pressed
	bool ButtonState = digitalRead(EncoderButton) == EncoderButtonLogic;
	if (ButtonState) {
		// If button is not locked
		if (!ButtonLocked) {
			if (PreviousButtonState) {
				// If button is pressed continuously
				if (millis() > LastButtonPressed + ButtonTimeout) {
					// Lock button
					ButtonLocked = true;
					// Call continuous press function
					LongPress();
					LongBeep();
				}
			} else {
				// Store last time button was pressed
				LastButtonPressed = millis();
			}
		}
	} else {
		// If button is not locked
		if (!ButtonLocked) {
			// If button was just released
			if (PreviousButtonState) {
				// Check for long or short press. Order matters!
				float ElapsedTime = millis() - LastButtonPressed;
				if (ElapsedTime >= LongPressTime && ElapsedTime <= MaxLongPressTime) {
					LongPress();
				} else if (ElapsedTime > ShortPressTime && ElapsedTime <= MaxShortPressTime) {
					ShortPress();
				}
			}
		} else {
			// If button was locked and now released unlock it
			ButtonLocked = false;
		}
	}
	// Store last button state
	PreviousButtonState = ButtonState;
}

void ClockwiseStep() {
	if (CurrentState == ColorState) {
		if (CurrentHue < 255) {
			CurrentHue++;
		} else {
			CurrentHue = 0;
		}
		SetLEDs();
		//Beep();
	} else if (CurrentState == BrightnessState) {
		if (CurrentBrightness < 254) {
			CurrentBrightness++;
			CurrentBrightness++;
			FastLED.setBrightness(CurrentBrightness);
			FastLED.show();
			//Beep();
		} else {
			LongBeep();
		}
	}
}

void CounterClockwiseStep() {
	if (CurrentState == ColorState) {
		if (CurrentHue > 0) {
			CurrentHue--;
		} else {
			CurrentHue = 255;
		}
		SetLEDs();
		//Beep();
	} else if (CurrentState == BrightnessState) {
		if (CurrentBrightness > 1) {
			CurrentBrightness--;
			CurrentBrightness--;
			FastLED.setBrightness(CurrentBrightness);
			FastLED.show();
			//Beep();
		} else {
			LongBeep();
		}
	}
}

void HandleEncoder() {
	int8_t NewPosition = knob.read();
	if (NewPosition < EncoderPosition) {
		if (Clockwise) {
			NewPosition = 0;
			knob.write(0);
		}
		Clockwise = false;
	} else if (NewPosition > EncoderPosition) {
		if (!Clockwise) {
			NewPosition = 0;
			knob.write(0);
		}
		Clockwise = true;
	}
	EncoderPosition = NewPosition;
	if (abs(EncoderPosition) > ClicksPerStep) {
		EncoderPosition = 0;
		knob.write(0);
		// Step detected
		if (Clockwise) {
			ClockwiseStep();
		} else {
			CounterClockwiseStep();
		}
	}
}

void loop() {

	HandleButton();

	HandleEncoder();
	
}
