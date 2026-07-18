// Version 1.3
// Refactored for international use, easy text customization,
// and the Waveshare LCD1602 I2C Module (AiP31068L controller).
//
// NOTE: This module is NOT a generic PCF8574 "backpack" board, so it uses
// the Waveshare_LCD1602 library instead of LiquidCrystal_I2C. Install it via:
// Arduino IDE > Sketch > Include Library > Add .ZIP Library, using the
// Waveshare_LCD1602 folder from Waveshare's LCD1602_I2C_Module_Demo.zip
// (Arduino/Waveshare_LCD1602).
//
// Long press (20 seconds) on Highscore screen = Clears the highscore
// Long press (2 seconds) on Dino-Run screen = Secret Mode (Defuse Bomb)
//
// Hardware Setup:
// Arduino Nano V3 Clone
// Waveshare LCD1602 I2C Module (AiP31068L controller)
// 1 Push Button (12x12mm)
//
// Pin Mapping:
// Button: Pin 8 (Other pin to GND, using INPUT_PULLUP)
// Buzzer: Pin 9 (Positive to Pin 9, Negative to GND)
// LCD VCC: 5V
// LCD GND: GND
// LCD SDA: A4
// LCD SCL: A5
// (No contrast potentiometer needed - this module sets contrast internally)

#include <Wire.h>
#include "Waveshare_LCD1602.h"
#include <EEPROM.h>

// =====================================================================
// --- CUSTOMIZABLE TEXTS (ENGLISH) ---
// Note: The LCD has 16 characters per line. Keep strings within this limit.
// Spaces are added intentionally to overwrite previous characters on the screen.
// =====================================================================

// Main Game & Menus
const char* txtTitle       = "   DINO RUN   ";
const char* txtNamesAlt1   = "Name1 & Name2 "; // Alternating name 1
const char* txtNamesAlt2   = "Name2 & Name1 "; // Alternating name 2
const char* txtLevelUp     = "!!! LEVEL ";
const char* txtLevelUpEnd  = " !!!";
const char* txtBestScore   = "BEST: ";
const char* txtPoints      = " Pts.   ";
const char* txtLevelStr    = "LEVEL: ";
const char* txtScoreDel    = "SCORE CLEARED!  ";
const char* txtNewRecord   = "NEW RECORD!     ";
const char* txtGameOver    = "GAME OVER!      ";
const char* txtScoreShort  = "S:";
const char* txtLevelShort  = " L:";

// Easter Egg (Bomb Defusal)
const char* eeInfo1L1      = "MISSION INFO    ";
const char* eeInfo1L2      = "SECRET FOUND!   ";
const char* eeInfo2L1      = "A bomb has been ";
const char* eeInfo2L2      = "discovered! ";    // Followed by bomb icon
const char* eeInfo3L1      = "Defuse it by    ";
const char* eeInfo3L2      = "perfect timing! ";
const char* eeInfo4L1      = "Press exactly at";
const char* eeInfo4L2      = "the right time! ";
const char* eePrepL1       = "Defuse in:      ";
const char* eePrepSec      = " seconds";
const char* eeRememberL1   = "Remember the    ";
const char* eeRememberL2   = "time!           ";
const char* eeCountdown3   = "3...            ";
const char* eeCountdown2   = "2...            ";
const char* eeCountdown1   = "1...            ";
const char* eeBeep         = "BEEP!           ";
const char* eeBoomL1       = "BOOM! ";          // Followed by bomb icon and " BOOM!"
const char* eeBoomL1End    = " BOOM!";
const char* eeTooSlow      = "Too slow!       ";
const char* eeDiff         = "Diff: ";
const char* eeMs           = " ms";
const char* eeTooEarly     = "-> Too early!   ";
const char* eeTooLate      = "-> Too late!    ";
const char* eePerfect      = "-> PERFECT!     ";
const char* eeDefusedL1    = "DEFUSED!        ";
const char* eeSavedL2      = "World saved :-) ";
const char* eeInaccurate   = "Too inaccurate! ";

// Invitation (Pages 1-7)
const char* invPage1L1     = "Invitation to   ";
const char* invPage1L2     = "Birthday Party  ";
const char* invPage2Name1  = "From: Name1 and "; // Adjust your names here
const char* invPage2Name2  = "From: Name2 and ";
const char* invPage2Name1B = "Name2           "; 
const char* invPage2Name2B = "Name1           ";
const char* invPage3Date   = "Oct 27 at 08:30 ";
const char* invPage3Scroll = "      We will pick you up!   "; // Scrolling text
const char* invPage4L1     = "We are going to ";
const char* invPage4L2     = "THE POOL        ";
const char* invPage5L1     = "Swimwear        ";
const char* invPage5L2Show = "DO NOT forget   "; // Blinking effect
const char* invPage5L2Hide = "       forget   ";
const char* invPage6L1     = "Ends at approx. ";
const char* invPage6L2     = "2:00 PM         ";
const char* invPage7L1     = "Please RSVP by  ";
const char* invPage7L2     = "Feb 20th!       ";


// =====================================================================
// --- PINS & HARDWARE SETUP ---
// =====================================================================
const int buttonPin = 8;
const int buzzerPin = 9;
Waveshare_LCD1602 lcd(16, 2);

// --- SPRITES (Custom Characters) ---
byte dinoSprite[8]    = { B00111, B00101, B00111, B10110, B11111, B01010, B01010, B00000 };
byte cactus1Sprite[8] = { B00000, B00100, B10100, B10101, B11101, B00111, B00100, B00100 };
byte cactus2Sprite[8] = { B00100, B01110, B00100, B00100, B01110, B00100, B00100, B00100 };
byte rocketSprite[8]  = { B00000, B00100, B01110, B01010, B01010, B11111, B10101, B00000 };
byte bombSprite[8]    = { B00010, B00100, B01110, B11111, B11111, B11111, B01110, B00000 };

// --- GLOBAL VARIABLES ---
int gameMode = 0;          // 0: Invite, 1: Start, 2: Play, 3: LevelUp, 4: GameOver, 5: Highscore, 6-9: EasterEgg
int invitationPage = 1;
int playerY = 1;           // Player Y position (0 = top, 1 = bottom)
bool canJump = true;       // Prevents holding the button to float endlessly

int score = 0;
int highscore = 0; 
int bestLevel = 1;
int currentLevel = 1;
int gameSpeed = 350;       // Delay between frames in milliseconds

unsigned long jumpStartTime = 0; 
unsigned long lastMoveTime = 0;

// --- LEVEL PRE-CALCULATION ARRAYS ---
// Max 25 enemies per level to prevent RAM overflow on the Arduino
const int MAX_OBSTACLES = 25; 
int obsX[MAX_OBSTACLES];
byte obsY[MAX_OBSTACLES];
byte obsType[MAX_OBSTACLES];
int numObstacles = 0;
int passedObstacles = 0;

// --- EASTER EGG VARIABLES ---
int eePage = 0;
unsigned long eePressStart = 0;
bool eeActive = false;
int targetTime = 0; 
unsigned long bombStartTime = 0;
unsigned long pressTime = 0;
bool bombStarted = false;


// =====================================================================
// --- HELPER: PRINT NUMBERS ---
// Waveshare_LCD1602 only accepts C-strings via send_string(), unlike
// LiquidCrystal's print(), which can take an int or String directly.
// This converts a number to text and sends it in one call.
// =====================================================================
void lcdPrintNum(long value) {
  char buf[12];
  ltoa(value, buf, 10);
  lcd.send_string(buf);
}


// =====================================================================
// --- SOUND EFFECTS ---
// =====================================================================
void playMarioIntro() {
  int melody[] = {660, 660, 0, 660, 0, 510, 660, 0, 770};
  int duration[] = {100, 100, 100, 100, 100, 100, 100, 100, 150};
  for (int i = 0; i < 9; i++) {
    if (melody[i] == 0) delay(duration[i]);
    else { tone(buzzerPin, melody[i], duration[i]); delay(duration[i] + 20); }
  }
}

void playPokemonHeal() {
  int notes[] = {1568, 1397, 1319, 1047, 1175, 1319}; 
  for (int i = 0; i < 6; i++) {
    tone(buzzerPin, notes[i], 150); delay(200);
  }
}

void playPokemonTriumph() {
  int notes[] = {1047, 1047, 1047, 1047, 1175, 1319, 1397, 1568};
  for (int i = 0; i < 8; i++) {
    tone(buzzerPin, notes[i], 150); delay(180);
  }
}

void playBeep() { tone(buzzerPin, 1500, 200); }

void playVictoryJingle() {
  int notes[] = {523, 659, 784, 1047};
  for (int i = 0; i < 4; i++) { tone(buzzerPin, notes[i], 150); delay(180); }
  delay(100); tone(buzzerPin, 1047, 400);
}

void playExplosion() {
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 100, 150); delay(150);
    tone(buzzerPin, 50, 150); delay(150);
  }
  tone(buzzerPin, 30, 500);
}


// =====================================================================
// --- SETUP FUNCTION ---
// =====================================================================
void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  // Initialize LCD and load custom characters
  lcd.init();
  lcd.display(); // Turn on the display (this library version has no brightness/backlight control)
  lcd.customSymbol(0, dinoSprite);
  lcd.customSymbol(1, cactus1Sprite);
  lcd.customSymbol(2, cactus2Sprite);
  lcd.customSymbol(3, rocketSprite);
  lcd.customSymbol(4, bombSprite);
  
  // Seed random generator with analog noise
  randomSeed(analogRead(0)); 
  
  // Read saved data from EEPROM
  highscore = EEPROM.read(0);
  if (highscore == 255) highscore = 0; // 255 is the default value of an empty EEPROM
  bestLevel = EEPROM.read(1);
  if (bestLevel == 255) bestLevel = 1;

  playMarioIntro();
}

// =====================================================================
// --- MAIN LOOP ---
// =====================================================================
void loop() {
  // State machine controlling the current screen/logic
  if (gameMode == 0) showInvitation();
  else if (gameMode == 1) showGameStart();
  else if (gameMode == 5) showHighscorePage();
  else if (gameMode == 2) gameLogic();
  else if (gameMode == 3) showLevelUp();
  else if (gameMode == 4) showGameOver();
  else if (gameMode == 6) eeIntro();
  else if (gameMode == 7) eePreparation();
  else if (gameMode == 8) eeGameLogic();
  else if (gameMode == 9) eeResult();
}

// =====================================================================
// --- GAME MECHANICS ---
// =====================================================================

// Calculates all enemies for the current level in advance
void generateLevel() {
  passedObstacles = 0;
  // Slowly increase enemy count, maxed out at MAX_OBSTACLES
  numObstacles = min(3 + (currentLevel * 2), MAX_OBSTACLES); 
  
  int currentX = 16; // Start placing enemies slightly off-screen to the right
  
  for(int i = 0; i < numObstacles; i++) {
    // 1. DETERMINE TYPE AND HEIGHT
    if (currentLevel >= 4 && random(0, 10) > 7) { 
      obsY[i] = 0; // Spawns at the top
      obsType[i] = 3; // Is a rocket
    } else { 
      obsY[i] = 1; // Spawns at the bottom
      obsType[i] = (random(0, 3) < 2) ? 1 : 2; // Randomly cactus 1 or 2
    }
    
    // 2. DETERMINE X POSITION
    if (i == 0) {
      obsX[i] = currentX;
    } else {
      int spacing = 0;
      bool isRocket = (obsY[i] == 0);
      bool prevWasRocket = (obsY[i-1] == 0);
      
      if (isRocket || prevWasRocket) {
        // Enforce safe distance if a rocket is involved
        spacing = random(6, 10); 
      } else {
        // Cactus logic: both obstacles are on the ground
        if (currentLevel >= 3 && random(0, 10) > 6) { 
          // 30% chance for a double-cactus (placed right next to each other)
          if (i == 1 || (obsX[i-1] - obsX[i-2] > 2)) {
            spacing = 1; // Create double-enemy
          } else {
            // Prevent impossible 3-blocks
            if (currentLevel >= 5) spacing = random(3, 6); // Gap 3 to 5
            else spacing = random(5, 9);                   // Gap 5 to 8
          }
        } else {
          // Normal gap without double-enemies
          if (currentLevel >= 5) spacing = random(3, 6); 
          else spacing = random(5, 9);                   
        }
      }
      currentX += spacing;
      obsX[i] = currentX;
    }
  }
}

void gameLogic() {
  static int oldScore = -1; 
  int oldPlayerY = playerY;

  // --- 1. DEBOUNCE BUTTON ---
  bool rawButton = (digitalRead(buttonPin) == LOW);
  static bool buttonPressed = false;
  static unsigned long lastButtonPress = 0;

  if (rawButton) {
    buttonPressed = true;
    lastButtonPress = millis();
  } else {
    // Release button only after 50ms to debounce
    if (millis() - lastButtonPress > 50) buttonPressed = false;
  }

  // --- 2. DYNAMIC JUMP LOGIC ---
  unsigned long jumpDuration = millis() - jumpStartTime;
  unsigned long minJumpTime = 2 * gameSpeed; 
  unsigned long maxJumpTime = 4 * gameSpeed; 

  if (playerY == 1) { // Player is on the ground
    if (buttonPressed && canJump) {
      playerY = 0; // Move up
      jumpStartTime = millis();
      canJump = false; // Prevent holding jump
      tone(buzzerPin, 800, 30);
    }
    if (!buttonPressed) canJump = true; // Reset jump ability
  } else { // Player is in the air
    if (buttonPressed) {
      if (jumpDuration >= maxJumpTime) playerY = 1; // Fall down after max time
    } else {
      if (jumpDuration >= minJumpTime) playerY = 1; // Fall down early if button released
    }
  }

  // --- 3. MOVE GAME WORLD ---
  if (millis() - lastMoveTime > gameSpeed) {
    lastMoveTime = millis();
    
    // Clear old positions of visible obstacles
    for (int i = 0; i < numObstacles; i++) {
      if (obsX[i] >= 0 && obsX[i] < 16) {
        // Prevent clearing the score text in the top right corner
        if (!(obsY[i] == 0 && obsX[i] >= 13)) {
          lcd.setCursor(obsX[i], obsY[i]); 
          lcd.send_string(" ");
        }
      }
    }
    
    // Move all obstacles to the left by 1
    for (int i = 0; i < numObstacles; i++) {
      obsX[i]--; 
      
      // Score point when an obstacle passes the player (X = -1)
      if (obsX[i] == -1) {
        score++;
        passedObstacles++;
        tone(buzzerPin, 1200, 15);
      }
      
      // Draw new position (only if within screen bounds)
      if (obsX[i] >= 0 && obsX[i] < 16) {
        // Don't draw rockets over the score (X>=13)
        if (!(obsY[i] == 0 && obsX[i] >= 13)) {
          lcd.setCursor(obsX[i], obsY[i]); 
          lcd.write_char(obsType[i]);
        }
      }
    }
    
    // Check if the level is completed
    if (passedObstacles >= numObstacles) {
      currentLevel++;
      gameMode = 3; 
      return;
    }
  }

  // --- 4. COLLISION DETECTION ---
  for (int i = 0; i < numObstacles; i++) {
    // Collision happens if player and obstacle share the same coordinates
    if (obsX[i] == 0 && playerY == obsY[i]) {
      gameMode = 4; // Trigger Game Over
      return;
    }
  }
  
  // Update player sprite on screen
  if (playerY != oldPlayerY) { 
    lcd.setCursor(0, oldPlayerY); lcd.send_string(" "); 
  }
  lcd.setCursor(0, playerY); 
  lcd.write_char(0);
  
  // Update score display only if changed
  if (score != oldScore) {
    lcd.setCursor(13, 0); 
    if (score < 10) lcd.send_string(" "); 
    if (score < 100) lcd.send_string(" "); 
    lcd.setCursor(13, 0); 
    lcdPrintNum(score);
    oldScore = score;
  }
}

// =====================================================================
// --- UI SCREENS ---
// =====================================================================

void showLevelUp() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.send_string(txtLevelUp); lcdPrintNum(currentLevel); lcd.send_string(txtLevelUpEnd);
  
  // Play sound depending on milestone
  if (currentLevel == 5 || currentLevel == 10) playPokemonTriumph();
  else playPokemonHeal(); 
  
  // Increase speed
  if (currentLevel == 2) gameSpeed = 320;
  else if (currentLevel == 3) gameSpeed = 280; 
  else { if(gameSpeed > 80) gameSpeed -= 10; } 
  
  // Pre-calculate the next level while the screen is shown
  generateLevel(); 
  
  delay(1000); lcd.clear(); gameMode = 2;
}

void showHighscorePage() {
  static unsigned long hsPressStart = 0; // Tracks button hold duration

  lcd.setCursor(0, 0); lcd.send_string(txtBestScore); lcdPrintNum(highscore); lcd.send_string(txtPoints);
  lcd.setCursor(0, 1); lcd.send_string(txtLevelStr); lcdPrintNum(bestLevel); lcd.send_string("      ");

  if (digitalRead(buttonPin) == LOW) {
    if (hsPressStart == 0) {
      hsPressStart = millis(); // Button just pressed
    } else if (millis() - hsPressStart >= 20000) {
      // Button held for 20 seconds -> CLEAR EEPROM
      highscore = 0;
      bestLevel = 1;
      EEPROM.write(0, 0);
      EEPROM.write(1, 1);
      
      tone(buzzerPin, 500, 500); 
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.send_string(txtScoreDel);
      delay(1500);
      lcd.clear();
      hsPressStart = 0; // Reset timer
    }
  } else {
    // Button released
    if (hsPressStart > 0 && millis() - hsPressStart < 2000) {
      // Short press -> start game normally
      lcd.clear(); 
      score = 0; 
      currentLevel = 1; 
      gameSpeed = 350;
      
      generateLevel(); 
      
      gameMode = 2; 
      delay(500); 
    }
    hsPressStart = 0; // Safely reset timer
  }
}

void showGameOver() {
  lcd.clear();
  if (score > highscore) {
    lcd.send_string(txtNewRecord); 
    highscore = score; 
    bestLevel = currentLevel;
    EEPROM.write(0, highscore); 
    EEPROM.write(1, bestLevel);
  } else { 
    lcd.send_string(txtGameOver); 
  }
  
  lcd.setCursor(0, 1); 
  lcd.send_string(txtScoreShort); lcdPrintNum(score); 
  lcd.send_string(txtLevelShort); lcdPrintNum(currentLevel);
  
  tone(buzzerPin, 150, 600); 
  delay(1500); 
  invitationPage = 0; 
  gameMode = 1; 
}

void showGameStart() {
  lcd.setCursor(0, 0); lcd.send_string(txtTitle); lcd.write_char(0);
  
  // Blinking names every 2 seconds
  if ((millis() / 2000) % 2 == 0) {
    lcd.setCursor(0, 1); lcd.send_string(txtNamesAlt1);
  } else {
    lcd.setCursor(0, 1); lcd.send_string(txtNamesAlt2);
  }

  // Activate Easter Egg if button held for 2 seconds
  if (digitalRead(buttonPin) == LOW) {
    if (eePressStart == 0) {
      eePressStart = millis();
    } else if (millis() - eePressStart >= 2000 && !eeActive) {
      eeActive = true;
      tone(buzzerPin, 2000, 300);
      lcd.clear();
      gameMode = 6;
      eePressStart = 0;
      delay(500);
      return;
    }
  } else {
    if (eePressStart > 0 && millis() - eePressStart < 2000) {
      tone(buzzerPin, 1000, 100); 
      lcd.clear(); 
      gameMode = 5; 
      delay(500);
    }
    eePressStart = 0;
  }
}

// =====================================================================
// --- EASTER EGG (BOMB DEFUSAL) ---
// =====================================================================
void eeIntro() {
  if (eePage == 0) {
    lcd.setCursor(0, 0); lcd.send_string(eeInfo1L1); 
    lcd.setCursor(0, 1); lcd.send_string(eeInfo1L2);
  } else if (eePage == 1) {
    lcd.setCursor(0, 0); lcd.send_string(eeInfo2L1); 
    lcd.setCursor(0, 1); lcd.send_string(eeInfo2L2); lcd.write_char(4); // Bomb Icon
  } else if (eePage == 2) {
    lcd.setCursor(0, 0); lcd.send_string(eeInfo3L1); 
    lcd.setCursor(0, 1); lcd.send_string(eeInfo3L2);
  } else if (eePage == 3) {
    lcd.setCursor(0, 0); lcd.send_string(eeInfo4L1); 
    lcd.setCursor(0, 1); lcd.send_string(eeInfo4L2);
  }
  
  if (digitalRead(buttonPin) == LOW) {
    tone(buzzerPin, 800, 50); 
    eePage++; 
    delay(300);
    if (eePage > 3) { eePage = 0; gameMode = 7; }
    lcd.clear();
  }
}

void eePreparation() {
  int seconds = random(3, 7); 
  targetTime = seconds * 1000; 
  
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eePrepL1); lcd.setCursor(0, 1); lcdPrintNum(seconds); lcd.send_string(eePrepSec); delay(2500);
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eeRememberL1); lcd.setCursor(0, 1); lcd.send_string(eeRememberL2); delay(1500);
  
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eeCountdown3); tone(buzzerPin, 800, 200); delay(1000);
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eeCountdown2); tone(buzzerPin, 800, 200); delay(1000);
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eeCountdown1); tone(buzzerPin, 800, 200); delay(1000);
  lcd.clear(); lcd.setCursor(0, 0); lcd.send_string(eeBeep); playBeep(); delay(500);
  
  lcd.clear(); 
  bombStartTime = millis(); 
  bombStarted = true; 
  gameMode = 8;
}

void eeGameLogic() {
  unsigned long elapsedTime = millis() - bombStartTime;
  
  // Timeout: Explode if 2 seconds passed the target time
  if (elapsedTime > targetTime + 2000) { 
    pressTime = 999999; 
    gameMode = 9; 
    return; 
  }
  
  // Register button press
  if (digitalRead(buttonPin) == LOW) { 
    pressTime = elapsedTime; 
    gameMode = 9; 
    delay(300); 
  }
}

void eeResult() {
  lcd.clear();
  if (pressTime == 999999) {
    lcd.setCursor(0, 0); lcd.send_string(eeBoomL1); lcd.write_char(4); lcd.send_string(eeBoomL1End);
    lcd.setCursor(0, 1); lcd.send_string(eeTooSlow); 
    playExplosion();
  } else {
    long diff = (long)pressTime - (long)targetTime;
    unsigned long absDiff = abs(diff);
    
    lcd.setCursor(0, 0); lcd.send_string(eeDiff); lcdPrintNum(absDiff); lcd.send_string(eeMs);
    lcd.setCursor(0, 1);
    
    if (diff < 0) lcd.send_string(eeTooEarly); 
    else if (diff > 0) lcd.send_string(eeTooLate); 
    else lcd.send_string(eePerfect); 
    
    delay(2500); lcd.clear();
    
    if (absDiff <= 500) {
      lcd.setCursor(0, 0); lcd.send_string(eeDefusedL1); 
      lcd.setCursor(0, 1); lcd.send_string(eeSavedL2); 
      playVictoryJingle();
    } else {
      lcd.setCursor(0, 0); lcd.send_string(eeBoomL1); lcd.write_char(4); lcd.send_string(eeBoomL1End);
      lcd.setCursor(0, 1); lcd.send_string(eeInaccurate); 
      playExplosion();
    }
  }
  delay(3000); lcd.clear(); bombStarted = false; eeActive = false; gameMode = 1;
}

// =====================================================================
// --- INVITATION LOGIC ---
// =====================================================================
void showInvitation() {
  if (invitationPage == 1) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage1L1); 
    lcd.setCursor(0, 1); lcd.send_string(invPage1L2); 
  }
  else if (invitationPage == 2) {
    if ((millis() / 2000) % 2 == 0) { 
      lcd.setCursor(0, 0); lcd.send_string(invPage2Name1); 
      lcd.setCursor(0, 1); lcd.send_string(invPage2Name1B); 
    } else { 
      lcd.setCursor(0, 0); lcd.send_string(invPage2Name2); 
      lcd.setCursor(0, 1); lcd.send_string(invPage2Name2B); 
    }
  }
  else if (invitationPage == 3) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage3Date);      
    // Create scrolling effect using the String object
    String lText = invPage3Scroll; 
    int pos = (millis() / 1000) % lText.length();
    String displayStr = lText.substring(pos) + lText.substring(0, pos); 
    lcd.setCursor(0, 1); lcd.send_string(displayStr.substring(0, 16).c_str());
  }
  else if (invitationPage == 4) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage4L1); 
    lcd.setCursor(0, 1); lcd.send_string(invPage4L2); 
  }
  else if (invitationPage == 5) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage5L1); 
    // Blinking logic
    if ((millis() / 500) % 2 == 0) { 
      lcd.setCursor(0, 1); lcd.send_string(invPage5L2Show); 
    } else { 
      lcd.setCursor(0, 1); lcd.send_string(invPage5L2Hide); 
    } 
  }
  else if (invitationPage == 6) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage6L1); 
    lcd.setCursor(0, 1); lcd.send_string(invPage6L2); 
  }
  else if (invitationPage == 7) { 
    lcd.setCursor(0, 0); lcd.send_string(invPage7L1); 
    lcd.setCursor(0, 1); lcd.send_string(invPage7L2); 
  }
  
  // Navigation
  if (digitalRead(buttonPin) == LOW) { 
    tone(buzzerPin, 1000, 50); 
    invitationPage++; 
    lcd.clear(); 
    if (invitationPage > 7) gameMode = 1; 
    delay(300); 
  }
}
