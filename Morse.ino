/******************************************************************
* Morse routines for Arduino
* Revision 2.0 
* Copyright ONL12523 Gilbert Ghyselbrecht 2022  
*
* Morse Timing:
*
* WPM = words per minute. Standard word = PARIS = 50 dots
* the DOT is a unit of time, at 16 WPM   dot = 60 / (wpm * 50) = 75 ms
*
* dot = dot
* dash = dot * 3
* character_interval = dot * 3
* space = dot * 4 + character_interval (= 7 * dot space between words)
*
*******************************************************************/

#define INTERBAUD_DURATION     Baud*1
#define INTERLETTER_DURATION   Baud*2
#define DIT_DURATION           Baud
#define DAH_DURATION           Baud*3

byte morsestate;   // state machine for sending morse characters

int teller;
char mkar;
char mc;
byte mp;
byte char_interval = 3;
const char *morse_tekst; 

const unsigned char morsewoord[] PROGMEM = { 
          0x00,0x00,0x92,0xCA,0xD1,0x85,0xC2,0x9E,  // Prosigns
          0xD6,0xAD,0x00,0xCA,0xB3,0xA1,0x95,0xD2,
          0xDF,0xCF,0xC7,0xC3,0xC1,0xC0,0xD0,0xD8,  // 0
          0xDC,0xDE,0xB8,0xC8,0x00,0xD1,0x00,0x8C,
          0xC4,0xF9,0xE8,0xEA,0xF4,0xFC,0xE2,0xF6,  // A
          0xE0,0xF8,0xE7,0xF5,0xE4,0xFB,0xFA,0xF7,
          0xE6,0xED,0xF2,0xF0,0xFD,0xF1,0xE1,0xF3,
          0xE9,0xEB,0xEC,0xD5,0x00,0x85,0x00,0x00,
         };

void morse_init() {
  morsestate = 5;
  morseRunning = 0;
  printat(0, 1, clearLine);
  set_tone (OFF);
}

void morse_stop() {
 morsestate = 5;
 morseRunning = 0;
 morseAutoRun = false; 
}

void morse_start( const char *m ) { // start morse engine
  morse_tekst = m;
  mp = 0;
  Baud = 1200/set.wpm;
  memset(lcdBuf,0x20,16);     // clear lcd text
  lcdBuf[16] = 0x00;
  showCursor = false;
  morsestate = 0;
  morseRunning = 1;
  morseDone = 0;
  if ( morseAutoRun && set.btime > 0 ) 
      beaconTimer = set.btime;   
}

void morse() {
  
  switch ( morsestate )
  {
    case 0: mc = toupper(morse_tekst[mp++]);
            if ( mc == 0 ) {          // end of text
              Serial.println();
              delay(1000);
              morse_init();              
              break;
              }
            Serial.print ( mc );
            morse_print( mc );
            if ( mc == ' ' ) {
              start_pause ( Baud * 4 );
              morsestate = 4;
              break;
              }
            if ( (mkar = pgm_read_byte(morsewoord+ mc-32)) == 0 ) break;
            teller = 8;
            while ( teller-- && (mkar & 0x80)) mkar <<=1;
            morsestate = 1;
            break;

    case 1: digitalWrite(keyOutPin , ON);
            contact(ON);
            busy = true;
            tms = TMS_INIT;
            timer1 = ((mkar <<=1) & 0x80) ? DAH_DURATION: DIT_DURATION;
            timer2 = timer1 + INTERBAUD_DURATION;
            morsestate = 2;
            break;
    
    case 2: if ( !busy ) morsestate = 3; 
            break;
            
    case 3: teller--;
            if ( teller == 0 ) {
                start_pause ( Baud * (char_interval-1));
                morsestate = 4;
            }
            else {
               morsestate = 1; 
            }
            break;
              
    case 4: if ( !busy ) morsestate = 0; 
              break;

    case 5: break;
  }
}

void start_pause ( int t ) {
  busy = true;
  tms = TMS_INIT;
  timer2 = t;
 }
 
void morse_print ( char c ) {
  memmove(lcdBuf, lcdBuf+1, 15);
  lcdBuf[15] = c;
  printat(0, 1, lcdBuf);
}


