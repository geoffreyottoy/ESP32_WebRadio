#include <VUMeter.h>

#define TIMER0_INTERVAL_MS 25

VUMeter vuMeter;

int fix_fftr(short f[], int m);

VUMeter::VUMeter(uint8_t leftChannel, uint8_t rightChannel){
    this->lPin = leftChannel;
    this->rPin = rightChannel;

    this->FFT = new arduinoFFT(this->vReal, this->vImag, SAMPLES, SAMPLING_FREQ);
}

void VUMeter::loop(void){
    if(micros()-this->prevSampleTime > TIMER0_INTERVAL_MS){
        this->prevSampleTime = micros();
        this->vReal[sampleCtr] = analogRead(this->lPin)*1.0; // A conversion takes about 9.7uS on an ESP32
        this->vImag[sampleCtr++] = 0;
    }

    if(this->sampleCtr>SAMPLES){
        Serial.println("fft");
        this->sampleCtr = 0;

        this->FFT->DCRemoval();
        this->FFT->Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        this->FFT->Compute(FFT_FORWARD);
        this->FFT->ComplexToMagnitude();

        // Analyse FFT results
        for (int i = 2; i < (SAMPLES/2); i++){       // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
            if (this->vReal[i] > NOISE) {                    // Add a crude noise filter
                // 7 bands, 12kHz top band
                if (i<=3)           this->bandValues[0]  += (int)this->vReal[i];
                if (i>3  && i<=7) this->bandValues[1]  += (int)this->vReal[i];
                if (i>7 && i<=15) this->bandValues[2]  += (int)this->vReal[i];
                if (i>15 && i<=30) this->bandValues[3]  += (int)this->vReal[i];
                if (i>30 && i<=60) this->bandValues[4]  += (int)this->vReal[i];
                if (i>60 && i<=235) this->bandValues[5]  += (int)this->vReal[i];
                if (i>235)           this->bandValues[6]  += (int)this->vReal[i];
            }
        }

        Serial.print("bv: ");
        for(int i=0; i<NUM_BANDS; i++){
            Serial.print(this->bandValues[i]);
            this->bandValues[i] = 0;
            Serial.print(" - ");
        }
        Serial.println();

    }
}
