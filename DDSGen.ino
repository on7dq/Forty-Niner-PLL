/*****************************************************************
* Arduino table-based digital sinus oscillator
* using "DDS" tables by Adrian Freed. Copyright 2009.
* Copyright ONL12523 Gilbert Ghyselbrecht 2022.  
* All Rights Reserved.
******************************************************************/

#define MORSE_VOLUME 240
#define SPOT_VOLUME   80
#define PWM_VALUE_DESTINATION     OCR2A
#define PWM_INTERRUPT   TIMER2_OVF_vect
#define TMS_INIT 32

const unsigned int LUTsize = 1<<8; // Look Up Table size: has to be power of 2 so that the modulo LUTsize
                                   // can be done by picking bits from the phase avoiding arithmetic
                                   
const unsigned char sintable[LUTsize] PROGMEM = { // already biased with +127
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,
  176,179,182,184,187,190,193,195,198,200,203,205,208,210,213,215,
  217,219,221,224,226,228,229,231,233,235,236,238,239,241,242,244,
  245,246,247,248,249,250,251,251,252,253,253,254,254,254,254,254,
  255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,
  245,244,242,241,239,238,236,235,233,231,229,228,226,224,221,219,
  217,215,213,210,208,205,203,200,198,195,193,190,187,184,182,179,
  176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,
  127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,
  78,75,72,70,67,64,61,59,56,54,51,49,46,44,41,39,
  37,35,33,30,28,26,25,23,21,19,18,16,15,13,12,10,
  9,8,7,6,5,4,3,3,2,1,1,0,0,0,0,0,
  0,0,0,0,0,0,1,1,2,3,3,4,5,6,7,8,
  9,10,12,13,15,16,18,19,21,23,25,26,28,30,33,35,
  37,39,41,44,46,49,51,54,56,59,61,64,67,70,72,75,
  78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124};

uint32_t phase;
int32_t  phase_increment;
byte     amplitude;
byte     amplitude_increment;
bool     increase_volume;
byte     outputvalue  = 127;
bool     busy = 0;
byte     prevStep;

int16_t  timer1 = 0;
int16_t  timer2 = 0;
byte     tms = 32;
byte     maxVolume = 240;

unsigned long phaseinc_from_frequency(unsigned long frequency_in_Hz)
{
    return ( 256 * ((131072 * frequency_in_Hz)/62500L) );
}

void DDS_init() {    // Setup PWM sinus generator
  tone_freq = constrain( abs(set.offset) , 400, 1000);
  phase = 0;
  phase_increment = phaseinc_from_frequency(tone_freq); 
  amplitude_increment = 2;
  amplitude = 0; 
   
  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);
}

void set_tone( bool seton )
 {
   maxVolume = MORSE_VOLUME;
   increase_volume = seton;      // increase/decrease volume
   morseTone = seton;
 }
 

void setSpotTone( bool seton ) {

  if ( menuMode ) return;

  maxVolume = SPOT_VOLUME;
  increase_volume = spotTone = seton;  // increase/decrease volume

   if ( seton ) {
    prevStep = step;
    step = 6;
    }
   else {
    step = prevStep;  
    }

   printFreq();
}

// this is the heart of the wavetable synthesis. A phasor looks up a sine table

SIGNAL(PWM_INTERRUPT)
{
  int16_t op;
 
   PWM_VALUE_DESTINATION = outputvalue; //output first to minimize jitter
   
   op = pgm_read_byte(sintable+((phase>>16)%LUTsize)) - 127;
   outputvalue =  (( op * amplitude )>>8) + 127;  //set amplitude
   phase += (uint32_t)phase_increment;

   // ramp amplitude
  
   if ( increase_volume && amplitude < maxVolume) amplitude += amplitude_increment;
   else if ( amplitude >= amplitude_increment )    amplitude -= amplitude_increment;

   // mS timers

   if ( --tms == 0 ) {
      tms = TMS_INIT;
      if ( timer1 ) {
        timer1--;
        if ( timer1 == 4 ) increase_volume = 0;
        if ( timer1 == 1 ) digitalWrite(keyOutPin, LOW);
        }
      if ( timer2 ) {
        timer2--;
        if ( timer2 == 1 ) busy = 0;
        }
      }  
  }
