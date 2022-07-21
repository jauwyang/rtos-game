#include "gameLogic.h"

#include <stdbool.h>
#include <math.h>

// Define PI Constants
# ifndef M_PI
# define M_PI 3.14159265358979323846
# endif

extern osThreadId_t hitBallID;
extern osThreadId_t writeScoreID;

// Power Mutex and Data
const uint32_t MAX_POWER = 8;
const uint32_t MIN_POWER = 2;

// Physics constants
const double ACCEL = 0.1;

// Note: the direction is kept in degrees and is not converted in accordance to the map. It must be updated immediately before using.
uint32_t prevPlayerDirection = 0;  // value to add hysteresis to prevent jittering in the pot output

const int MAP_CONVERSION_ANGLE = 30; // Note: 90 degrees means straight up.

// Position Struct, Actor Mutex and Data
extern osMutexId_t ballMutex;

Ball *golfBall;
Environment *hole;
Environment *teleporter;

extern osMutexId_t isHitMutex;

// Golf Score Mutex and Data
const int MAX_GOLF_SCORE = 20;
int golfScore;  
extern osMutexId_t scoreMutex;


// GLCD
// Mapping Data Values
// -> Note: - The GLCD values are set for portrait view
//          - The Bottom Left will be x = y = 0??? (CHECK)<<<<<< HERE
const uint32_t LCD_HEIGHT = 240;  // y-axis
const uint32_t LCD_WIDTH = 320;  // x-axis

// Sizes and bitmaps
const uint32_t BALL_SIZE = 5;
const uint32_t ENV_SIZE = 5;

char ballBitmap[] = {0x38, 0x38, 0x38};		// 3 x 8 
char holeBitmap[] = {0x38, 0x38, 0x38};		// 3 x 8
char teleporterBitmap[] = {0x38, 0x38, 0x38};  // 3 x 8
const uint32_t BALL_GLCD_WIDTH = 3;
const uint32_t ENVIRONMENT_GLCD_WIDTH = 3;

extern osThreadId_t arrowBitMapMutex;

// ================================
// ====== GLCD + Draw on GLCD =====
// ================================


// Initializes in-game elements. Called before threads are created and the kernel is initialized.
// (1) Golf Score
// (2) Golf golfBall State
// (3) Hole State
void setupGame(void) {
  golfScore = 0;
  
  golfBall = malloc(sizeof(golfBall));
  golfBall->pos.x = rand() % ((LCD_WIDTH - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->pos.y = rand() % ((LCD_HEIGHT - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;

  golfBall->bitmap = ballBitmap;
  golfBall->power = MIN_POWER;

  hole = malloc(sizeof(Environment));
  hole->bitmap = holeBitmap;

  // Hole location cannot be same as golfBall location when spawned
  do {
    hole->pos.x = rand() % ((LCD_WIDTH - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;
    hole->pos.y = rand() % ((LCD_HEIGHT - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;
  } while (hole->pos.x == golfBall->pos.x && hole->pos.y == golfBall->pos.y);

  teleporter = malloc(sizeof(Environment));
  teleporter->bitmap = teleporterBitmap;

  // Teleporter location cannot be same as golfBall or hole location when spawned
  do {
    teleporter->pos.x = rand() % ((LCD_WIDTH - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;
    teleporter->pos.y = rand() % ((LCD_HEIGHT - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;	
  } while ((teleporter->pos.x == golfBall->pos.x && teleporter->pos.y == golfBall->pos.y) && 
    (teleporter->pos.x == hole->pos.x && teleporter->pos.y == hole->pos.y));
	
	// Draw the still ball
	GLCD_SetTextColor(White);
	drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ballBitmap, BALL_GLCD_WIDTH);
	
	// Draw the hole
	GLCD_SetTextColor(Black);
	drawSpriteAt(hole->pos.x, hole->pos.y, holeBitmap, ENVIRONMENT_GLCD_WIDTH);
  
	// Draw teleporter
  	GLCD_SetTextColor(Red);
	drawSpriteAt(teleporter->pos.x, teleporter->pos.y, teleporterBitmap, ENVIRONMENT_GLCD_WIDTH);
  
}


void checkEndGame(void *args) {
	while (1) {
		osMutexAcquire(scoreMutex, osWaitForever);
		int score = golfScore;
		osMutexRelease(scoreMutex);

		osMutexAcquire(ballMutex, osWaitForever);
		
		// Terminate all threads responsible for drawing on the LCD
		// And erase the ball from the game
		
		// Can maybe write to the LCD if golf score is too high
		if (inHole(BALL_GLCD_WIDTH, ENVIRONMENT_GLCD_WIDTH) || score > MAX_GOLF_SCORE) {

			unsigned char loseText[6] = "LOSE!";
			unsigned char winText[5] = "WIN!";
			
			// Erase the golf ball
			GLCD_SetTextColor(Green);
			drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ballBitmap, 3);
			
			osThreadTerminate(hitBallID);
			osThreadTerminate(writeScoreID);

			printf("%s", (score > MAX_GOLF_SCORE) ? loseText : winText);
		}
		
		osMutexRelease(ballMutex);
	}
}

void teleportBall(void *args) {
  while (1) {
    osMutexAcquire(ballMutex, osWaitForever);
		
	// Randomly teleport ball to a different location. It is possible for the ball to return to the same spot or in the hole itself.
	if (inTeleporter(BALL_GLCD_WIDTH, ENVIRONMENT_GLCD_WIDTH)) {
      golfBall->pos.x = rand() % ((LCD_WIDTH - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;
      golfBall->pos.y = rand() % ((LCD_HEIGHT - ENV_SIZE) - ENV_SIZE + 1) + ENV_SIZE;
    } 

    osMutexRelease(ballMutex);
  }
}


void drawPixelsAt(int x, int y, int limit) {
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < limit; ++j) {
			GLCD_PutPixel(x + i, y + j);
		}
	}
}

void drawSpriteAt(int x, int y, char *bitmap, int bitmap_size) {
	// Print the sprite left to right, with x being the vertical component, y the horizontal
	// Given the orientation of the screen, joystick, and button
	
	int spriteIndex = bitmap_size - 1;
	int spriteShift = 0;
	
	for (int i = (bitmap_size - 1) * SPRITE_SCALE; i >= 0; i -= SPRITE_SCALE) {
		spriteShift = 0;
		for (int j = 0; j < SPRITE_COLS*SPRITE_SCALE; j+=SPRITE_SCALE) {
			if ((bitmap[spriteIndex] >> spriteShift) & 1) {
				
				drawPixelsAt(x + ((bitmap_size - 1) * SPRITE_SCALE - i), y + j, SPRITE_SCALE);
			}
			
			spriteShift++; // Bitmap column coordinate
		}
		
		spriteIndex--; // Bitmap row coordinate
	}
}

// =============================
// ======= Serial Output =======
// =============================

// -->> SERIAL OUTPUT <<--
// MUTEX: scoreMutex
// PROTECTED DATA: golfScore
void writeGolfScore(void *args) {
	while (1) {
		osMutexAcquire(scoreMutex, osWaitForever);
		printf("Golf Score: %d\n", golfScore);
		osMutexRelease(scoreMutex);
		osDelay(1000U);
	}
}


// =============================
// =========== LEDs ============
// =============================

// Initialize the LED pin directions (set as outputs)
void initLEDs(void) { 
  LPC_GPIO1->FIODIR |= 1<<28; 
  LPC_GPIO1->FIODIR |= 1<<29; 
  LPC_GPIO1->FIODIR |= 1<<31; 
  
  LPC_GPIO2->FIODIR |= 1<<2; 
  LPC_GPIO2->FIODIR |= 1<<3; 
  LPC_GPIO2->FIODIR |= 1<<4; 
  LPC_GPIO2->FIODIR |= 1<<5; 
  LPC_GPIO2->FIODIR |= 1<<6;
}


// -->> LEDs <<-- 
// MUTEX: ballMutex
// PROTECTED DATA: golfball->power
// Updates LEDs depending on Power Level
void updateLEDs(void *args) {
  while (1) {
      // Initialize array of ints to store
      bool ledsOn[MAX_POWER] = {false, false, false, false, false, false, false, false};
      
      osMutexAcquire(ballMutex, osWaitForever);
      for (int i = 0; i < golfBall->power; ++i) {
        ledsOn[i] = true;
      }
      osMutexRelease(ballMutex);

      // LEDs are ordered from P.28 on the left to P.6 on the right

      //--- GPIO 1 LEDs ---//
      // First LED
      if (ledsOn[0]) {
        LPC_GPIO1->FIOSET |= (1 << 28);
      } else {
        LPC_GPIO1->FIOCLR |= (1 << 28);
      }

      // Second LED
      if (ledsOn[1]) {
        LPC_GPIO1->FIOSET |= (1 << 29);
      } else {
        LPC_GPIO1->FIOCLR |= (1 << 29);
      }

      // Third LED
      if (ledsOn[2]) {
        LPC_GPIO1->FIOSET |= (1 << 31);
      } else {
        LPC_GPIO1->FIOCLR |= (1 << 31);
      }

      //--- GPIO2 LEDs --//
      if (ledsOn[3]) {
        LPC_GPIO2->FIOSET |= (1 << 2);
      } else {
        LPC_GPIO2->FIOCLR |= (1 << 2);
      }

      if (ledsOn[4]) {
        LPC_GPIO2->FIOSET |= (1 << 3);
      } else {
        LPC_GPIO2->FIOCLR |= (1 << 3);
      }

      if (ledsOn[5]) {
        LPC_GPIO2->FIOSET |= (1 << 4);
      } else {
        LPC_GPIO2->FIOCLR |= (1 << 4);
      }

      if (ledsOn[6]) {
        LPC_GPIO2->FIOSET |= (1 << 5);
      } else {
        LPC_GPIO2->FIOCLR |= (1 << 5);
      }

      if (ledsOn[7]) {
        LPC_GPIO2->FIOSET |= (1 << 6);
      } else {
        LPC_GPIO2->FIOCLR |= (1 << 6);
      }
	  }
}



// ===========================================
// ============= READ USER INPUT =============
// ===========================================


//  -->> JOYSTICK <<--
// MUTEX: ballMutex
// PROTECTED DATA: golfBall->power
void readPowerInput(void *args) {
  unsigned int lastStateP26;
  unsigned int currStateP26;

  unsigned int lastStateP24;
  unsigned int currStateP24;

  while (1) {		
      // Read both P26 and P24		
      currStateP26 = !(LPC_GPIO1->FIOPIN & (1 << 26));
      currStateP24 = !(LPC_GPIO1->FIOPIN & (1 << 24));

      // Joystick was pulled down, toward P.26 label and current state is not the same as previous state
      // State check is done to 
      if (currStateP26 && currStateP26 != lastStateP26) {
        osMutexAcquire(ballMutex, osWaitForever);
        if (golfBall->power > MIN_POWER) {
					osDelay(150);		// Delay changes power level so it is slow enough for user control. 150 is a nice value.
          golfBall->power--;
        }
        osMutexRelease(ballMutex);
      }

      // Joytick was pushed up, toward P.24 label and current state is not the same as previous state
      else if (currStateP24 && currStateP24 != lastStateP24) {
        osMutexAcquire(ballMutex, osWaitForever);
        if (golfBall->power < MAX_POWER) {
					osDelay(150);
          golfBall->power++;
        }
        osMutexRelease(ballMutex);
      }
  }
}


//  -->> POTENTIOMETER <<--
// MUTEX: ballMutex
// PROTECTED DATA: golfBall->direction
void readDirectionInput(void *args) {
  while (1) {
    // Read current angle of potentiometer
    int32_t currAngle;

    LPC_ADC -> ADCR |= 1<<24;
    while (!(LPC_ADC->ADGDR & (1<<31)));

    currAngle = LPC_ADC -> ADGDR & 0XFFFF;
    currAngle = currAngle >> 4;

    currAngle = currAngle / 12;  // used to convert value to 340 degrees of rotation
	

    // Update golfBall direction if potentiometer direction changes
    if (currAngle - prevPlayerDirection > 5) {  // add a hysteresis to prevent unwanted jitter 
			osMutexAcquire(ballMutex, osWaitForever);

      golfBall -> direction = currAngle;
      prevPlayerDirection = currAngle;

			printf("%d\n", currAngle - MAP_CONVERSION_ANGLE);
			osMutexRelease(ballMutex);
    } 
  }
}

// -->> PUSH BUTTON << --
void hitBall(void *args) {
	unsigned int lastButtonState;
	unsigned int currButtonState;
	
	while (1) {
		currButtonState = !(LPC_GPIO2->FIOPIN & (1<<10));
		
		if (currButtonState && currButtonState != lastButtonState) {
			// Increment the golf score every time the ball is hit
			osMutexAcquire(scoreMutex, osWaitForever);
			golfScore++;
			osMutexRelease(scoreMutex);
			
			// Ball draw and logic ops
			osMutexAcquire(ballMutex, osWaitForever);
			// Erase the still ball by simply writing to all '1' locations in the bitmap with bg colour
			GLCD_SetTextColor(Green);
			drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ballBitmap, BALL_GLCD_WIDTH);
			
			// Move the ball bitmap, i.e. animate the ball moving 
			launchBall();
			
			// Once the ball has stopped, launchBall() will return. At this point, it is now safe to redraw the still ball.
			// Draw the still ball.
			GLCD_SetTextColor(White);
			drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ballBitmap, BALL_GLCD_WIDTH);
			
			osMutexRelease(ballMutex);
		}
		
		lastButtonState = currButtonState;
	}
}

// ===========================================
// ============== GAME  PHYSICS ==============
// ===========================================

void launchBall(void) {	
	// Get angle, power, current position
	double xPrevPos = golfBall->pos.x;
	double yPrevPos = golfBall->pos.y;
	
	//printf("Before: %lf\n", golfBall->direction - MAP_CONVERSION_ANGLE);
	double angle = convertAngle(golfBall->direction);
	//printf("After: %lf\n", angle);
	
	uint32_t power = golfBall->power;
	
	// Set initial ball velocity
	golfBall->xVelocity = 2* power * cos(angle*2);
	golfBall->yVelocity = 2* power * sin(angle*2);
	
			
	printf("Vx: %d\n", golfBall->xVelocity);
	printf("Vy: %d\n", golfBall->yVelocity);
	
	while (abs(golfBall->xVelocity) != 0  && abs(golfBall->yVelocity) != 0) {
		
		//*** Erase the ball at the previous position ***//
		GLCD_SetTextColor(Green);
		drawSpriteAt(xPrevPos, yPrevPos, ballBitmap, 3);
		
		// Determine if the ball is out of bounds based on the min and max width and height of the LCD 
		int xTemp = golfBall->pos.x + golfBall->xVelocity; 
		int yTemp = golfBall->pos.y + golfBall->yVelocity;
		
		bool xLessMin = xTemp <= 0;
		bool xMoreMax = xTemp >= LCD_WIDTH;
		bool yLessMin = yTemp <= 0;
		bool yMoreMax = yTemp >= LCD_HEIGHT;
		
		// x out of bounds
		if (xLessMin || xMoreMax) {
			golfBall->pos.x = (xLessMin) ? 0: LCD_WIDTH;
		} else {
			golfBall->pos.x = xTemp;
		}
		
	 // printf("Vx: %d\n", golfBall->xVelocity);
	  //printf("Vy: %d\n", golfBall->yVelocity);

		// y out of bounds
		if (yLessMin || yMoreMax) {
			golfBall->pos.y = (yLessMin) ? 0 : LCD_HEIGHT;
		} else {
			golfBall->pos.y = yTemp;
		}
						
		//*** Draw the ball at the current position ***//
		GLCD_SetTextColor(White);
		drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ballBitmap, 3);
		
		
		// Update the velocity at the new position
		golfBall->xVelocity = (golfBall->xVelocity < 0) ? golfBall->xVelocity + ACCEL : golfBall->xVelocity - ACCEL;
		golfBall->yVelocity = (golfBall->yVelocity < 0) ? golfBall->yVelocity + ACCEL : golfBall->yVelocity - ACCEL;
				
		// Change the direction of the current velocity based on the position of the ball
		// Run bounce algorithm
		if (xLessMin || xMoreMax) {
			golfBall->xVelocity = golfBall->xVelocity * -1;
		}
		
		if (yLessMin || yMoreMax) {
			golfBall->yVelocity = golfBall->yVelocity * -1;
		}
		
		// Set previous position as the current position for the next cycle
		xPrevPos = golfBall->pos.x;
		yPrevPos = golfBall->pos.y;
	}
}

bool inHole(int sizeBall, int sizeHole) {
	// Extract coordinates of the golfBall
	int xTopBall = golfBall->pos.x;
	int yTopBall = golfBall->pos.y;
	
	int xBotBall = golfBall->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotBall = golfBall->pos.y + sizeBall * SPRITE_SCALE;

	// Extract coordinates of the hole
	int xTopHole = hole->pos.x;
	int yTopHole = hole->pos.y;
	
	int xBotHole = hole->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotHole = hole->pos.y + sizeHole * SPRITE_SCALE;
	
  // If this returns true, then the golfBall is in the hole
	return (xTopBall < xBotHole && 
          xBotBall > xTopHole &&
          yTopBall < yBotHole &&
          yBotBall > yTopHole);
}


bool inTeleporter(int sizeBall, int sizeTeleporter) {
  	// Extract coordinates of the golfBall
	int xTopBall = golfBall->pos.x;
	int yTopBall = golfBall->pos.y;

	int xBotBall = golfBall->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotBall = golfBall->pos.y + sizeBall * SPRITE_SCALE;

	// Extract coordinates of the teleporter
	int xTopTeleporter = teleporter->pos.x;
	int yTopTeleporter = teleporter->pos.y;

	int xBotTeleporter = teleporter->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotTeleporter = teleporter->pos.y + sizeTeleporter * SPRITE_SCALE;
	
  	// If this returns true, then the golfBall is in the teleporter
	return (xTopBall < xBotTeleporter && 
          xBotBall > xTopTeleporter &&
          yTopBall < yBotTeleporter &&
          yBotBall > yTopTeleporter);
}





// ===========================================
// ============== MISCELLANEOUS ==============
// ===========================================


// Converts the angle (degrees) obtained from potentiometer into radians and positions it correctly w.r.t. map orientation
double convertAngle(int32_t rawAngle) {
    return (rawAngle + MAP_CONVERSION_ANGLE) * M_PI / 180 ;
}


// Converts an array of binary values to its decimal value
uint32_t convertBinaryArrayToDecimal(uint32_t *bits, uint32_t arraySize) {
  int result = 0;

  for (int i = 0; i < arraySize; i++) {
    result |= bits[i];
    if (i != arraySize - 1) {
      result <<= 1;
    }
  }

  return result;
}
