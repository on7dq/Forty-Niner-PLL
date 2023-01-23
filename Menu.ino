/******************************************************************
 *               Forty Niner PLL
 *              
 *    ON7DQ   ONL12523          
 *    
 *    Version 1.0    6/01/2023
 ******************************************************************/

byte newByte;   
int  newInt;
long newLong;

struct menu_info {
const char* titel;
void (*handler)();
};

static menu_info menus[] = {
  { "BAND", menuBand},
  { "FREQ Minimum", menuFmin},
  { "FREQ Maximum", menuFmax},
  { "Rx Offset", menuOffset},
  { "PTT Delay", menuDelay},
  { "WPM", menuWpm},
  { "Beacon", menuBeacon},
};

byte mstate = 0;
byte fmstate = 0;
byte mindex = 0;

void menu() {
  
switch (mstate) {
  case 0: if ( menuButton ) {
            morse_stop();              // stop running Beacon message          
            releaseButton (menuPin);   // wait
            rstate = ROTARY_MENU;
            menuMode = true;
            mstate = 1;
            fmstate = 0;
            }            
          break;
          
  case 1: lcd.clear();
          showCursor = false;     
          sprintf ( buf1, "%d %s", mindex+1, menus[mindex].titel );                              
          printat ( 0, 0, buf1 );
          mstate = 2;
          break;     
  
                           
  case 2: (menus[mindex].handler)();      // menu function call
          if ( exitButton) {
            releaseButton (exitPin);   // wait
            exitButton = false;
            rstate = ritMode ? ROTARY_RIT : ROTARY_TX;
            fmstate = mstate = 0;
            menuMode = false;
            lcd.clear();
            printat (0, 0, "  "); 
            cursorRow = splitMode ? 1 : 0;
            showCursor = true;             
            update_freq(true);
            }
          break;
  }
}  


void rotaryMenu() {
  if ( result == DIR_CW ) mindex++;
  else if ( result == DIR_CCW ) mindex--;

  if ( mindex == 0xFF ) mindex = (sizeof(menus)/sizeof(menus[0]))-1; 
  else if ( mindex >= (sizeof(menus)/sizeof(menus[0]))) mindex = 0;

  fmstate = 0;
  mstate = 1;
}

void rotaryBand() {
  if ( result == DIR_CW ) newByte++;
  else if ( result == DIR_CCW ) newByte--;

  if ( newByte == 0xFF ) newByte = (sizeof(band)/sizeof(band[0]))-1; 
  else if ( newByte >= (sizeof(band)/sizeof(band[0]))) newByte = 0;

  printat ( 2, 1 , band[newByte].num );
}

void rotaryOffset(int rt1) { 
 
 byte localStep = ( rt1 < 50 ) ? 100 : 10;  // change offset

  if ( result == DIR_CW) set.offset += localStep;
  else if ( result == DIR_CCW ) set.offset -= localStep;
  set.offset = constrain(set.offset, -990, 990);
  sprintf ( buf1, "%5d Hz", set.offset) ;  
  DDS_init();                            
  printat ( 1, 1, buf1);
}


void rotaryFminMax() {
  if ( rotaryButton ) {
          rotateStep();                
          printFreq2(newLong); 
          rotaryButton = false;
          }
  else   {
    if ( result == DIR_CW) newLong += stepScale[step];
    else if ( result == DIR_CCW ) newLong -= stepScale[step];
    newLong = constrain(newLong, 10000, 99000000);  
    printFreq2(newLong);     
  }
}

void rotaryDelay(int rt1) {
 
  byte localStep = ( rt1 < 50 ) ? 100 : 10; 

  if ( result == DIR_CW ) newInt += localStep;
  else if ( result == DIR_CCW ) newInt -= localStep;
  newInt = constrain(newInt, 10, 10000); 
  sprintf ( buf1, "%5d mS", newInt) ;                              
  printat ( 1, 1, buf1);
}

void rotaryBeacon(int rt1) {
 
  byte localStep = ( rt1 < 50 ) ? 20 : 1; 

  if ( result == DIR_CW ) newInt += localStep;
  else if ( result == DIR_CCW ) newInt -= localStep;
  newInt = constrain(newInt, 0, 5400);
  if ( newInt > 0 )   printTimer(newInt, buf1);                             
  else                sprintf ( buf1, " OFF ");                              
  printat ( 2, 1, buf1);
}

void rotaryWpm() {
  if ( result == DIR_CW ) newByte += 1;
  else if ( result == DIR_CCW ) newByte -= 1;
  newByte = constrain(newByte, 5, 49); 
  sprintf ( buf1, "%3d", newByte) ;                              
  printat ( 1, 1, buf1);
}

void rotateStep() {
      if ( result == DIR_CW ) { 
          step++;
          if ( step > (sizeof(stepScale)/sizeof(stepScale[0]))-1 ) step--; 
          }
      else {
          step--;
          if ( step == 0xFF ) step++; 
          }
}

void rotaryOnOff() {
       if ( splitMode ) clrSplitMode;
       else             { clrRitMode; setSplitMode; } 
       printat ( 5, 1, ( splitMode ? "ON " : "OFF") );   
}

void menuBand() {
  switch (fmstate) {
    case 0:  newByte = set.band;
             printat (2, 1, band[newByte].num);
             fmstate   = 1; 
             break;
    
     case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait  
              menuButton = false;
              rstate = ROTARY_BAND;
              printat(0, 1, ">");
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              set.band = newByte;
              set.freqMin = band[set.band].min;
              set.freqMax = band[set.band].max;
              set.freqTx =  band[set.band].set;
              set.freqRx = set.freqTx;
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                    
              fmstate = 0;             
              }
            break;
  }

}

void menuBeacon() {
  switch (fmstate) {
    case 0: newInt = set.btime;
            if ( newInt > 0 )   printTimer(newInt, buf1);                            
            else                sprintf ( buf1, " OFF ");                              
            printat ( 2, 1, buf1); 
            fmstate = 1; 
            break;
    
     case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_BEACON;
              printat(0, 1, ">");
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              set.btime = newInt;
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();    
              fmstate = 0;             
              }
            break;
  }

}

void menuOffset() {
  switch (fmstate) {
    case 0: newInt = set.offset;
            sprintf ( buf1, "%5d Hz", set.offset) ;                              
            printat ( 1, 1, buf1);   
            fmstate = 1; 
            break;
    
     case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_OFFSET;
              printat(0, 1, ">");
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                    
              fmstate = 0;             
              }
            if ( exitButton ) {
              set.offset = newInt;  // no change
              DDS_init();
            }              
            break;
  }
}

void menuFmin() {
    switch (fmstate) {
    case 0: newLong = set.freqMin;
            printat(3, 1, freqtostr(newLong, buf1));
            fmstate   = 1; 
            break;
    
    case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_FMINMAX;
              cursorRow = 1;          
              showCursor = true;
              printat(0, 1, ">");
              printFreq2(newLong);
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              set.freqMin = newLong;
              if ( set.freqMin > set.freqMax) set.freqMin = set.freqMax;
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                   
              fmstate = 0;             
              }
            break;
  }
}

void menuFmax() {
    switch (fmstate) {
    case 0: newLong = set.freqMax;
            printat(3, 1, freqtostr(newLong, buf1));
            fmstate   = 1; 
            break;
    
    case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_FMINMAX;
              showCursor = true;
              cursorCol = 1;  
              printat(0, 1, ">");
              printFreq2(newLong);
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              set.freqMax = newLong;
              if ( set.freqMax < set.freqMin) set.freqMax = set.freqMin;
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                      
              fmstate = 0;             
              }
            break;
  }
}

void menuWpm() {
  switch (fmstate) {
    case 0: newByte = set.wpm;
            sprintf ( buf1, "%3d", newByte);                              
            printat ( 1, 1, buf1);   
            fmstate = 1; 
            break;
    
     case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_WPM;
              printat(0, 1, ">");
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              releaseButton (menuPin);   // wait
              set.wpm = newByte;
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                    
              fmstate = 0;             
              }
            break;
  }
}

void menuDelay() {
    switch (fmstate) {
    case 0: newInt = set.pttDelay;
            sprintf ( buf1, "%5d mS", newInt) ;                              
            printat ( 1, 1, buf1);
            fmstate   = 1; 
             break;
    
     case 1: if ( menuButton ){
              releaseButton (menuPin);   // wait
              menuButton = false;
              rstate = ROTARY_DELAY;
              printat(0, 1, ">");
              fmstate = 2;             
              }
            break;

    case 2: if ( menuButton ){
              set.pttDelay = newInt;
              releaseButton (menuPin);   // wait
              save_settings();
              rstate = ROTARY_MENU;      
              printStored();                      
              fmstate = 0;             
              }
            break;
  }
}


