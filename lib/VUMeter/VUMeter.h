#ifndef __VU_METER__
#define __VU_METER__

#include <Arduino.h>
#include <ESP32TimerInterrupt.h>
#include <arduinoFFT.h>

#define DEFAULT_L_PIN 32
#define DEFAULT_R_PIN 33

#define SAMPLES         1024          // Must be a power of 2
#define SAMPLING_FREQ   40000         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE       1000          // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define COLOR_ORDER     GRB           // If colours look wrong, play with this
#define CHIPSET         WS2812B       // LED strip type
#define NUM_BANDS       7             // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE           500           // Used as a crude noise filter, values below this are ignored

class VUMeter {
private:
    double vReal[SAMPLES];
    double vImag[SAMPLES];
    int bandValues[NUM_BANDS];
    
    uint8_t lPin;
    uint8_t rPin;

    unsigned long prevSampleTime=0;
    int sampleCtr = 0;
    int infoCtr = 0;

    arduinoFFT * FFT;
public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    VUMeter(uint8_t leftChannel=DEFAULT_L_PIN, uint8_t rightChannel=DEFAULT_R_PIN);

    void loop(void);
    void sample( void );
};

extern VUMeter vuMeter;

#endif /* __VU_METER__ */
