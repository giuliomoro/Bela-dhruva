/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\
http://bela.io
*/
/**
\example Trill/bar-led/render.cpp

Trill Bar Sound + LED
=============

This is example of how to communicate with the Trill Bar sensor using
the Trill library. It also visualises position of different touches in real time via
a series of LEDs connected to the digital outputs.

The Trill sensor is scanned on an auxiliary task running parallel to the audio thread
and the number of active touches, their position and size stored on global variables.

Twelve LEDs are used to represent positions on the Trill sensor. The length of the
Trill Bar sensor is divided into 12 different sections. When a touch
occurs on one of these sections, the corresponding LED turns on.
*/

#include <Bela.h>
#include <cmath>
#include <libraries/Trill/Trill.h>
#include <libraries/OnePole/OnePole.h>
#include <libraries/Oscillator/Oscillator.h>

#define NUM_TOUCH 5 // Number of touches on Trill sensor
#define NUM_LED 4 // Number of LEDs used for visualisation

// Trill object declaration
Trill touchSensor;

// Location of touches on Trill Bar
float gTouchLocation[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Size of touches on Trill bar
float gTouchSize[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Number of active touches
int gNumActiveTouches = 0;

// Sleep time for auxiliary task
unsigned int gTaskSleepTime = 12000; // microseconds
// Time period for printing
float gTimePeriod = 0.01; // seconds

// Digital pins assigned to LEDs used for visualisation
unsigned int gLedPins[NUM_LED] = { 0, 1, 2, 3 };
// Status of LEDs (1: ON, 0: OFF)
bool gLedStatus[NUM_LED] = { 0, 0, 0, 0 };

// audio part
// One Pole filters objects declaration
OnePole freqFilt[NUM_TOUCH], ampFilt[NUM_TOUCH];
// Frequency of one pole filters
float gCutOffFreq = 5, gCutOffAmp = 15;
// Oscillators objects declaration
Oscillator osc[NUM_TOUCH];
// Range for oscillator frequency mapping
float gFreqRange[2] = { 200.0, 1500.0 };
// Range for oscillator amplitude mapping
float gAmplitudeRange[2] = { 0.0, 1.0 } ;
// audio part end

/*
 * Function to be run on an auxiliary task that reads data from the Trill sensor.
 * Here, a loop is defined so that the task runs recurrently for as long as the
 * audio thread is running.
 */
void loop(void*)
{
	while(!Bela_stopRequested())
	{
		// Read locations from Trill sensor
		touchSensor.readI2C();
		gNumActiveTouches = touchSensor.getNumTouches();
		for(unsigned int i = 0; i < gNumActiveTouches; i++)
		{
			gTouchLocation[i] = touchSensor.touchLocation(i);
			gTouchSize[i] = touchSensor.touchSize(i);
		}
		// For all inactive touches, set location and size to 0
		for(unsigned int i = gNumActiveTouches; i <  NUM_TOUCH; i++)
		{
			gTouchLocation[i] = 0.0;
			gTouchSize[i] = 0.0;
		}
		usleep(gTaskSleepTime);
	}
}

bool setup(BelaContext *context, void *userData)
{
	// Setup a Trill Bar sensor on i2c bus 2 on BBAI, using the default mode and address
	if(touchSensor.setup(2, Trill::BAR) != 0) {
		fprintf(stderr, "Unable to initialise Trill Bar\n");
		return false;
	}

	touchSensor.printDetails();

	// Set and schedule auxiliary task for reading sensor data from the I2C bus
	Bela_runAuxiliaryTask(loop);
	// Set all digital pins corresponding to LEDs as outputs
	for(unsigned int l = 0; l < NUM_LED; l++)
		pinMode(context, 0, gLedPins[l], OUTPUT);
	// For each possible touch...
	for(unsigned int i = 0; i < NUM_TOUCH; i++) {
		// Setup corresponding oscillator
		osc[i].setup(context->audioSampleRate, Oscillator::sine);
		// Setup low pass filters for smoothing frequency and amplitude
		freqFilt[i].setup(gCutOffFreq, context->audioSampleRate);
		ampFilt[i].setup(gCutOffAmp, context->audioSampleRate);
	}
	return true;
}

void render(BelaContext *context, void *userData)
{
	// Active sections of the Trill BAR
	bool activeSections[NUM_LED] = { false };

	// Printing counter
	static unsigned int count = 0;

	// Each audio frame, check location of active touches and round to the the number
	// of sections on which the Trill bar has been devided.
	// Set LED status based on activations of corresponding sections.
	// Print LED status.
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		//audio part
		float out = 0.0;
/* For each touch:
		*
		* 	- Map touch location to frequency of the oscillator
		* 	and smooth value changes using a single pole LP filter
		* 	- Map touch size toa amplitude of the oscillator and
		* 	smooth value changes using a single pole LP filter
		* 	- Compute oscillator value and add to output.
		* 	- The overall output will be scaled by the number of touches.
		*/
		for(unsigned int i = 0; i < NUM_TOUCH; i++) {
			float frequency, amplitude;
			frequency = map(gTouchLocation[i], 0, 1, gFreqRange[0], gFreqRange[1]);
			// Uncomment the line below to apply a filter to the frequency of the oscillators
			// frequency = freqFilt[i].process(frequency);
			amplitude = map(gTouchSize[i], 0, 1, gAmplitudeRange[0], gAmplitudeRange[1]);
			amplitude = ampFilt[i].process(amplitude);

			out += (1.f/NUM_TOUCH) * amplitude * osc[i].process(frequency);
		}
		// Write computed output to audio channels
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}

		//audio part end
		for(unsigned int t = 0; t < gNumActiveTouches; t++) {
			int section = floor( NUM_LED * gTouchLocation[t] );
			activeSections[section] = activeSections[section] || 1;
		}

		for(unsigned int l = 0; l < NUM_LED; l++) {
			gLedStatus[l] = activeSections[l];
			digitalWrite(context, n, gLedPins[l], gLedStatus[l]);
		}

		if(count >= gTimePeriod*context->audioSampleRate)
		{
			for(unsigned int l = 0; l < NUM_LED; l++)
				rt_printf("%d ",gLedStatus[l]);
			rt_printf("\n");
			count = 0;
		}
		count++;
	}
}

void cleanup(BelaContext *context, void *userData)
{}
