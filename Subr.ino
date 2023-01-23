/******************************************************************
 *                Forty Niner PLL
 *              
 *                  ON7DQ   ONL12523          
 *    
 *    Subroutines    6/01/2023
 ******************************************************************/  
#include <EEPROM.h>

void(* resetFunc) (void) = 0;    //declare reset function at address 0

char *freqtostr(long fl, char *bf) {
 char s[15];

  sprintf(s, "%8ld", fl);     // convert to string
  
  memcpy ( bf, s, 5 );     bf[5] = '.';  // split string
  memcpy ( bf+6, s+5, 3 ); 
  memcpy ( bf+9, " kHz", 5 );

  return bf;
 }

void printat ( char col, char row, const char*s) {
  lcd.noCursor();
  lcd.setCursor(col, row);
  lcd.print ( s );
 }

void printFreq() {
  showCursor = true;
  printat(1, 0, splitMode ? "Tx" : "Rx");
  printat(3, 0, freqtostr(set.freqTx, buf1));

  //if ( morseAutoRun ) {
    if (set.btime > 0) {
      lcd.setCursor(12, 0);
    lcd.write(byte(0));
  }
  
  if ( splitMode ) {
    printat(1, 1, "Rx");    
    printat(3, 1, freqtostr(set.freqRx, buf1));
    }
  else if ( ritMode ) {
            sprintf ( buf1, ">RIT %5d Hz", set.freqRit);
            printat (0, 1, buf1); printat(0 , 0, " ");
            }
       else {
            printat(0, 1, clearLine);
            }
    
  if ( showCursor && !(ritMode) ) {
    if ( cursorRow == 1 ) {printat(0 , 0, " "); printat(0, 1, ">");rstate = ROTARY_RX;}
    else { printat(0 , 1, " "); printat(0, 0, ">");rstate = ROTARY_TX;}
    lcd.setCursor(stepCursor[step], cursorRow);
    lcd.cursor();
  }    
}

void printFreq2( long f ) {
  printat(3, 1 , freqtostr( f, buf1 ) );
  if ( showCursor ) {
    lcd.setCursor(stepCursor[step], cursorRow);
    lcd.cursor();
  }
}


char *printTimer(int t, char*bf) {
  byte mm, ss;
  
  mm = t / 60;
  ss = t % 60;
  sprintf( bf, "%2d:%02d", mm, ss);
  return bf;
}

void printBeaconTimer () {
  printat(0, 1, " NEXT TX ");
  printat(9, 1, printTimer(beaconTimer, buf1) );
}

void save_settings( ) {
 char* s = (char*) &set;
 byte check = 0; 

  for ( byte i = 0 ; i < sizeof(settings)-1; i++ ) check ^= s[i];
  set.chk = check;
  EEPROM.put ( 0, set);
  
  Serial.println("Save settings");
}

void load_settings() {
  char* s = (char*) &set;
  byte check = 0; 

  EEPROM.get ( CALIBRATION_ADDRESS, calibration );
  EEPROM.get ( 0, set);

  for ( byte i = 0 ; i < sizeof(settings); i++ ) check ^= s[i]; 

  if ( check != 0 ){     // load default settings if EEPROM checksum wrong
    set.offset  = 600;      // RX TX offset, can be + or -990
    set.freqRit = 0;        // RIT frequency
    set.freqTx  = 7030000;  // start frequency CLK0 
    set.freqRx  = 7030000;  // RX frequency
    set.freqMin = 7000000;  // minimum freq 40M band
    set.freqMax = 7200000;  // maximum freq 40M band
    set.pttDelay = 800;    // PTT delay 1 sec
    set.btime = 0;          // Beacon time OFF 
    set.wpm = 16;           // morse WPM 
    set.band = 4;           // select band 40m
    set.mode = 0;           // 
    set.cqID = 0;           // CQ Message
    save_settings();
    }
}

void printset() {
  char* s = (char*) &set;
  byte c;
  
 for ( byte i = 0 ; i < sizeof(settings) ; i++) {
   c = s[i];
   Serial.print(c, HEX); Serial.print("  ");}

Serial.println();
}

void releaseButton ( byte button ) {
 int state = 0;

 do {
    state =  (state << 1) | digitalRead(button);
    delay(2);
    }
 while ( state != 0xFFFF );    
 }

void printStored() {
  printat(0, 1, "Stored in EEPROM" );
  delay(1500);
  printat(0, 1, "                ");
}

/************** SETUP *******************/

void printSetupText ( const char *s ) {
   printat(0, 0, s);
   printat(0, 1, "OK=Menu Exit=Alt");
}

void clearEEPROM() {

   printSetupText("Reset EEPROM ?  ");

   releaseButton(menuPin);

   do {
      read_buttons();
      } 
   while ( !menuButton && !exitButton );

   if ( menuButton ) {
       for ( int i = 0 ; i < sizeof(settings) ; i++) EEPROM.update ( i, 0xFF);
       EEPROM.write(0, 55);   // destroy checksum
      }
  
   pinMode(pwmPin, INPUT);
   resetFunc();
 }

void calibrate() {

   printSetupText( "Calibrate Si5351");

   releaseButton(exitPin);

   si5351.set_freq(1000000000, SI5351_CLK2);
   si5351.output_enable(SI5351_CLK2, ON);

   while ( !read_buttons() );     // wait answer

   if ( menuButton ) {
    showCursor = true; cursorRow = 1;
    printat ( 0, 1, clearLine);
    printCal();
    releaseButton(menuPin);

    do {
        read_buttons();
        if ( rotaryStep ) {
          rotaryStep = false;
          if ( rotaryButton ) {
                  rotateStep();                
                  printCal(); 
                  rotaryButton = false;
                  }
          else   {
            if ( result == DIR_CW) calibration += stepScale[step];
            else if ( result == DIR_CCW ) calibration -= stepScale[step];
            calibration = constrain(calibration, -5000000, 5000000);  
            si5351.set_correction(calibration, SI5351_PLL_INPUT_XO) ;  
            printCal();     
            }
          }
        } 
    while ( !menuButton && !exitButton );
   }
   if ( menuButton ) {
          EEPROM.put(CALIBRATION_ADDRESS, calibration);
          printStored();
          delay(1000);
      }
   pinMode(pwmPin, INPUT);
   resetFunc();
}

void printCal () {
    freqtostr(calibration, buf1); buf1[9] = '\0';
    printat ( 3, 1, buf1 );

    if ( showCursor ) {
        lcd.setCursor(stepCursor[step], cursorRow);
        lcd.cursor();
        }
}
