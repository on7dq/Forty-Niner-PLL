/******************************************************************
 *    Forty Niner PLL
 *
 *    Hardware      : Luc ON7DQ and Gil ONL12523
 *    Software      : Gil ONL12523
 *    Documentation : Luc ON7DQ
 *
 *    Version 1.0    23/01/2023
 ******************************************************************/

#include <si5351.h>
#include <Wire.h>
#include <rotary.h>
#include <LiquidCrystal_I2C.h>

// using a UC-146 I2C interface for the LCD display, change address or pins if needed
// the numbers are the port numbers of the PCF8574 chip
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol  (bl = backlight)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Create LCD object and set the LCD I2C address

// edit your messages here, add as many as you  like
// characters you may use : see manual, Appendix 1
const char* morseText[] = { "CQ CQ DE ON7DQ ON7DQ PSE K",
                            "TEST RBN DE ON7DQ ON7DQ TEST RBN",
                            "VVV DE ON7DQ/B ON7DQ/B JO11KF PWR 1 WATT = 73",
                            "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOGS BACK",
                            };

#define OFF   0
#define ON    1

#define CALIBRATION_ADDRESS 1000

// pin definitions
#define rotaryPin    4     // digital input pin for rotary encoder push switch
#define pttPin       7     // to transistor at D6 (mute audio)
#define exitPin      10    // ALT/EXIT pushbutton
#define menuPin      5     // MENU pushbutton
#define spotPin      6     // SPOT pushbutton
#define txrxPin     12     // from 49er (from zener diode), as in QST article
#define keyOutPin   13     // keying the 49er the classical way via transistor over the KEY connector in 49er
#define pwmPin      11     // audio tone output, needs a simple RC low pass filter + one transistor

#define mstart PORTD |=  _BV(5)
#define mstop  PORTD &=  ~_BV(5)

#define SPLIT  0          // mode bits
#define RIT    1

#define splitMode     (set.mode &   _BV(SPLIT))
#define setSplitMode  set.mode |=  _BV(SPLIT)
#define clrSplitMode  set.mode &= ~_BV(SPLIT)

#define ritMode       (set.mode &   _BV(RIT))
#define setRitMode    set.mode |=  _BV(RIT)
#define clrRitMode    set.mode &= ~_BV(RIT)

enum {                     // Rotary FSM states
ROTARY_TX,
ROTARY_RX,
ROTARY_RIT,
ROTARY_MENU,
ROTARY_BAND,
ROTARY_OFFSET,
ROTARY_FMINMAX,
//ROTARY_FMAX,
ROTARY_WPM,
ROTARY_DELAY,
ROTARY_BEACON,
ROTARY_ONOFF,
};

byte cursorCol = 5;
byte cursorRow = 0;
bool showCursor = false;

unsigned long vt1 ,t1;
unsigned long pttTimer;
unsigned long secTimer;

int beaconTimer = 0; // beacon or autorepeat time at startup
byte saveTimer = 5;  // after how much time is info stored in EEPROM (seconds)

long stepScale[] = {1000000,100000,10000,1000,500,100,10,1}; // possible frequency steps
byte stepCursor[] = {4,5,6,7,8,9,10,11};  // underline cursor positions, corresponding to above steps
byte step = 4;                            // which step to start with , 0 -- 7

byte charB[8] = {0x0, 0x0, 0x0, 0x6, 0x6, 0x0, 0x0, 0x0}; // custom beacon indicator, make your own 
/* e.g. a small square is made like this
00000 = 0x0
00000 = 0x0
00000 = 0x0
00110 = 0x6
00110 = 0x6
00000 = 0x0
00000 = 0x0
00000 = 0x0
*/

char buf1[50];
char lcdBuf[18];

unsigned char result;
char  rstate;

int tone_freq;
int Baud;              // = 1200/wpm;

long calibration;      // Si5351 calibration factor

bool rotaryStep;
bool setChanged = false;
bool morseTone;
bool spotTone;
bool key;
bool spot;
bool ptt;
bool rotaryButton;
bool rotaryHold;
bool menuButton;
bool exitButton;
bool menuMode = false;
bool splitRx = false;
bool morseRunning;
bool morseDone;
bool morseAutoRun = false;


struct settings {       // automatically saved to EEPROM
  int  offset;          // RX TX offset, can be from +999 to -999
  int  freqRit;         // RIT offset freq
  long freqTx;          // TX frequency CLK0
  long freqRx;          // Rx frequency CLK1
  long freqClk3;        //    frequency CLK3, for calibration routine
  long freqMin;         // band limit, minimum freq
  long freqMax;         // band limit, maximum freq
  int  pttDelay;        // PTT delay in ms
  int  btime;           // Beacon delay 0 -- 90 minutes
  byte band;            // band selection
  byte wpm;             // WPM 5 - 49
  byte mode;            // mode settings
  byte cqID = 0;
  byte chk;             // XOR checksum
}set;

struct band_info {
  const char* num;
  long min;
  long max;
  long set;
};

// bands, see manual
static band_info band[] = {
  { "160m", 1810000, 2000000,  1836000 },
  { " 80m", 3500000, 3800000,  3560000 },
  { " 75m", 3800000, 4000000,  3881000 },     // +FAV
  { " 60m", 5351500, 5366500,  5354000 },
  { " 40m", 7000000, 7200000,  7030000 },
  { " 20m",14000000,14350000, 14060000 },
  { " ALL",   10000,99999999, 10000000 },
};

Si5351 si5351;
Rotary r = Rotary(2, 3);

const char clearLine[] = "                ";

void setup() {
  Serial.begin(115200);
  Serial.println("49PLL  Ver:1.0");

  lcd.begin(16, 2);         // 1602 = 2 rows of 16 characters
  lcd.createChar(0,charB);

  pinMode(rotaryPin, INPUT_PULLUP);
  pinMode(pttPin, OUTPUT);
  digitalWrite (pttPin, LOW);

  pinMode(menuPin, INPUT_PULLUP);
  pinMode(exitPin, INPUT_PULLUP);
  pinMode(spotPin, INPUT_PULLUP);
  pinMode(txrxPin, INPUT);

  pinMode(keyOutPin, OUTPUT);
  digitalWrite (keyOutPin, LOW);

  pinMode(pwmPin, OUTPUT) ;

  PCICR |= (1 << PCIE2);              // Rotary Interrupt service code
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);

  load_settings();

  if ( !si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0 , calibration))
             Serial.println("si5351 not found");

  tone_freq = constrain( abs(set.offset) , 400, 1000);
  DDS_init();

  morse_init();
  // splash screen, comment these two lines for no splash screen
  printat(0, 0, "Forty Niner PLL" );
  printat(0, 1, "Version 1.0"); // you could put your call/name here

  delay(2000);

  lcd.clear();
  // special EEPROM erase routine if MENU is pressed at startup or reset
  if ( !digitalRead(menuPin) ) clearEEPROM();
  // special CALIBRATION routine if ALT/EXIT is pressed at startup or reset
  if ( !digitalRead(exitPin) ) calibrate();

  rstate = ritMode ? ROTARY_RIT : ROTARY_TX;
  showCursor = true;
  update_freq(true);
  setPTT(OFF);
  secTimer = millis();
 }

/***********************************************************/

void loop() {

 key = !digitalRead(txrxPin);
   if ( key && !morseTone ) contact(ON);     // Morse tone
   if ( !key && morseTone ) contact(OFF);

 spot = !digitalRead(spotPin);
   if ( spot && !spotTone ) setSpotTone(ON); // Spot tone
   if ( !spot && spotTone ) setSpotTone(OFF);

 timers();

 morse();

 read_buttons();

 toggleMenuButton();

 if ( exitButton ) toggleExitButton();

 if ( rotaryButton ) toggleRitMode();

 if ( rotaryStep ) fsm_rotary();   // Rotary Finite State Machine
}

/************************************************************/

bool read_buttons() {
bool b1, b2, b3, ba = false;

rotaryButton = false;
menuButton  = false;
exitButton  = false;

 if ( b1 = !digitalRead(rotaryPin)) ba = true;
 if ( b2 = !digitalRead(menuPin ) ) ba = true;
 if ( b3 = !digitalRead(exitPin ) ) ba = true;

 if ( ba ) {
    delay(20);
    if ( b1 && !digitalRead(rotaryPin)) rotaryButton = true;
    if ( b2 && !digitalRead(menuPin ) ) menuButton = true;
    if ( b3 && !digitalRead(exitPin ) ) exitButton = true;
    }
 if ( !rotaryButton ) rotaryHold = false;

 return ba;
}


void toggleExitButton() {
  long t1 = millis();

  if ( menuMode ) return;

  do {
    if ( rotaryStep ) {
      if ( result == DIR_CW ) set.cqID++;
      else if ( result == DIR_CCW ) set.cqID--;

      if  (set.cqID == 0xFF ) set.cqID = (sizeof(morseText)/sizeof(morseText[0]))-1; 
      else if ( set.cqID >= (sizeof(morseText)/sizeof(morseText[0]))) set.cqID = 0;

      sprintf(buf1, "%-16.16s", morseText[set.cqID]);
      printat(0, 1, buf1);
      rotaryStep = false;     
      }
  }
  while ( !digitalRead(exitPin) );

  if ( morseRunning ) {        // pause morse message
      morse_init();
      return;
     }

  if ( (millis() - t1) > 500) {
       morseAutoRun = !!set.btime;
       morse_start(morseText[set.cqID]);
       update_freq(true);
       return;
       }

  if ( splitMode ) {      // if SPLIT change Tx/Rx
    if ( splitRx ) {
        splitRx = false;
        cursorRow = 1;
        rstate = ROTARY_RX;
        }
      else
        {
        splitRx = true;
        clrRitMode;
        cursorRow = 0;
        rstate = ROTARY_TX;
        }

  }
  if ( ritMode ) set.freqRit = 0;

  update_freq(true);
}

void toggleMenuButton() {

  if ( menuMode ) { menu(); return; }
 
  if ( menuButton ) {
    long t1 = millis();
    releaseButton (menuPin);     // wait for release menubutton
   
    if ( (millis() - t1) > 500) {
        if ( splitMode ) {
            clrSplitMode;
            set.freqTx = set.freqRx;
            cursorRow = 0;
            rstate = ROTARY_TX;
            }
        else {
            clrRitMode;
            setSplitMode;
            cursorRow = 1;
            rstate = ROTARY_RX;
            splitRx = false;
            }
        update_freq(true);
        return;
      }
    else {
        if ( morseAutoRun ) {       // stop morse message
            morse_init();
            morseAutoRun = false;
            printFreq();
            return;
         }
      }
      
  menu();
  }
}

void toggleRitMode() {

 if ( rotaryHold || splitMode || menuMode ) return;

 delay(200);

 if ( !digitalRead(rotaryPin) ) { rotaryHold = true; return;}

 if ( ritMode ) {
     clrRitMode;
     rstate = ROTARY_TX;
     }
  else {
     setRitMode;
     rstate = ROTARY_RIT;
     printat(0, 1, clearLine);
     }

  update_freq(true);
  }

void fsm_rotary()
{
  int rt1;
  static int rt[4];
  static char rtl = 0;

  rotaryStep = false;

  rt[rtl++] = t1; rtl &= 0x01;    // average rotary time
  rt1 = (rt[0] + rt[1]) / 2;

  switch ( rstate) {
   case ROTARY_TX:
            if ( rotaryButton ) {
                rotateStep();
                printFreq();
                rotaryButton = false;
                }
            else   {
              if ( result == DIR_CW) set.freqTx += stepScale[step];
              else if ( result == DIR_CCW ) set.freqTx -= stepScale[step];
                  set.freqTx = constrain(set.freqTx, set.freqMin, set.freqMax);
                  update_freq(true);
            }
            break;

   case ROTARY_RX:
            if ( rotaryButton ) {
                rotateStep();
                printFreq();
                rotaryButton = false;
                }
            else   {
              if ( result == DIR_CW) set.freqRx += stepScale[step];
              else if ( result == DIR_CCW ) set.freqRx -= stepScale[step];
              set.freqRx = constrain(set.freqRx, set.freqMin, set.freqMax);
              update_freq(true);
            }
            break;

   case ROTARY_RIT:
              if ( result == DIR_CW) set.freqRit += 10;
              else if ( result == DIR_CCW ) set.freqRit -= 10;
              set.freqRit = constrain(set.freqRit, -5000, 5000);
              update_freq(true);
            break;

    case ROTARY_MENU: rotaryMenu();
            break;
    case ROTARY_BAND: rotaryBand();
            break;
    case ROTARY_OFFSET: rotaryOffset(rt1);
            break;
    case ROTARY_FMINMAX: rotaryFminMax();
            break;
    case ROTARY_DELAY: rotaryDelay(rt1);
            break;
    case ROTARY_BEACON: rotaryBeacon(rt1);
            break;
    case ROTARY_WPM: rotaryWpm();
            break;
    case ROTARY_ONOFF: rotaryOnOff();
            break;
    }
 }

void update_freq(bool printLCD) {
  long rxFreq;

  if ( !splitMode ) set.freqRx = set.freqTx;

  if ( printLCD ) printFreq();

  si5351.set_freq(set.freqTx * 100ULL, SI5351_CLK0);
  rxFreq = set.freqRx + set.offset;
  if ( ritMode ) rxFreq += set.freqRit;
  si5351.set_freq(rxFreq * 100ULL, SI5351_CLK1);

  setChanged = true;
  saveTimer = 5;
}

ISR(PCINT2_vect) {

  result = r.process();     // Rotary encoder interrupt

  if ( result ) {
    t1 = millis() - vt1;
    vt1 = millis();
    rotaryStep = true;
    }
}

void contact ( bool state) {

  if ( state == ON ) {
    if ( !menuMode ) setPTT ( ON );
    set_tone(ON);
    }
  else {
    set_tone(OFF);
    }
}

void setPTT ( bool mode ) {

  if ( mode == ON  ) {
    pttTimer = millis() + set.pttDelay;
    if ( !ptt ) {
        digitalWrite(pttPin, ON);
        delayMicroseconds(500);
        si5351.output_enable(SI5351_CLK1, OFF);
        si5351.output_enable(SI5351_CLK0, ON);
        ptt = ON;
        printat (1, 0, "TX");
        if ( splitMode ) printat (1, 1, "  ");
      }
    }
  else {
    digitalWrite(pttPin, OFF);
    si5351.output_enable(SI5351_CLK0, OFF);
    si5351.output_enable(SI5351_CLK1, ON);
    ptt = OFF;
    if ( !menuMode ) printFreq();
    }
 }

void timers() {

  if ( !key && ptt && pttTimer < millis()) { setPTT ( OFF );}

/************** 1 sec *********************/

  if ( secTimer < millis() ) {
    secTimer += 1000;
    
    if ( morseAutoRun && beaconTimer > 0 ) {
      beaconTimer--;
      if ( !morseRunning && !splitMode && !ritMode ) {
        printBeaconTimer();
      }
      if ( !menuMode && !morseRunning && beaconTimer == 0) 
        morse_start(morseText[set.cqID]);
    }
   
    if ( setChanged && saveTimer > 0) {
      saveTimer--;
      if (saveTimer == 0 ) {
         setChanged = false;
         save_settings(); 
      }
    }
  }
}
