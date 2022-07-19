#include "gameLogic.h"

#include <stdbool.h>
#include <math.h>

// Define PI Constants
# ifndef M_PI
# define M_PI 3.14159265358979323846
# endif


// Power Mutex and Data
const uint32_t MAX_POWER = 8;
const uint32_t MIN_POWER = 1;

// Note: the direction is kept in degrees and is not converted in accordance to the map. It must be updated immediately before using.
uint32_t prevPlayerDirection = 0;  // value to add hysteresis to prevent jittering in the pot output
const uint32_t MAP_CONVERSION_ANGLE = 80; // Note: 90 degrees means straight up.

// Position Struct, Actor Mutex and Data
extern osMutexId_t ballMutex;

Ball *golfBall;
Hole *hole;

// Ball Physics Values
const int32_t FRICTION_ACCEL = -5;
const double ZERO_TOLERANCE = 0.001;
const uint32_t TIME = 1;

bool isHit = false;
extern osMutexId_t isHitMutex;

// Golf Score Mutex and Data
int golfScore;  
extern osMutexId_t scoreMutex;


// GLCD
extern osThreadId_t animateID;

extern osSemaphoreId_t canDrawSem;

// Mapping Data Values
// -> Note: - The GLCD values are set for portrait view
//          - The Bottom Left will be x = y = 0??? (CHECK)<<<<<< HERE
const uint32_t LCD_HEIGHT = 320;  // y-axis
const uint32_t LCD_WIDTH = 240;  // x-axis

// Sizes and bitmaps
const uint32_t BALL_SIZE = 5;
const uint32_t HOLE_SIZE = 5;
const uint32_t DIRECTION_ARROW_SIZE = 7;
const uint32_t ARROW_BITMAP_SIZE_WIDTH = 15;
const uint32_t ARROW_BITMAP_MID = (ARROW_BITMAP_SIZE_WIDTH + 1) / 2;

char ballBitmap[] = {0x38, 0x38, 0x38};
char holeBitmap[] = {0x38, 0x38, 0x38};
char arrowBitMap[ARROW_BITMAP_SIZE_WIDTH] = {};

extern osThreadId_t arrowBitMapMutex;

// ================================
// ====== GLCD + Draw on GLCD =====
// ================================


// Initializes in-game elements
// (1) Golf Score
// (2) Golf Ball State
// (3) Hole State
void setupGame(void) {
  golfScore = 0;
  
  golfBall = malloc(sizeof(Ball));
  golfBall->pos.x = rand() % ((LCD_WIDTH - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->pos.y = rand() % ((LCD_HEIGHT - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->bitmap = ball_bitmap;
  golfBall->power = 1;

  hole = malloc(sizeof(Hole));
  hole->bitmap = hole_bitmap;

  // Hole location cannot be same as ball location when spawned
  do {
    hole->pos.x = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
    hole->pos.y = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
  } while (hole->pos.x == golfBall->pos.x && hole->pos.y == golfBall->pos.y);
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


void drawBall(void *args) {	
	// Wait for the semaphore; if hitBall signals that calculations are done, can draw
	osSemaphoreAcquire(canDrawSem, osWaitForever);
	
	osMutexAcquire(ballMutex, osWaitForever);
	GLCD_SetTextColor(Green);
	drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ball_bitmap, 3);
	osMutexRelease(ballMutex);
}


void drawHole(void) {
	GLCD_SetTextColor(Black);
	drawSpriteAt(hole->pos.x, hole->pos.y, hole_bitmap, 3);
}


// MUTEX: ballMutex, arrowBitMapMutex
// PROTECTED DATA: golfBall, arrowBitMap
void createArrowBitMap(void *args) {
  osMutexAcquire(ballMutex, osWaitForever);
  double ballDirection = convertAngle(goldBall->direction);
  osMutexRelease(ballMutex);

  // Initialize Empty Bitmap with 1 at center
  uint32_t arrow[ARROW_BITMAP_SIZE_WIDTH][ARROW_BITMAP_SIZE_WIDTH] = {0};
  arrow[ARROW_BITMAP_MID-1][ARROW_BITMAP_MID-1] = 1;  // make center of bit map (where the ball would be) 1

  // Draw Arrow in the 2D Bitmap starting at center of map
  for (uint32_t i = 1; i < DIRECTION_ARROW_SIZE + 1; i++) {  
    uint32_t x = round(i*cos(ballDirection));
    uint32_t y = round(i*sin(ballDirection));
    arrow[x + ARROW_BITMAP_MID - 1][y+ ARROW_BITMAP_MID - 1] = 1;
  }

  char tempArrowBitMap[ARROW_BITMAP_SIZE_WIDTH] = {};
  
  // Convert 2D Binary Bitmap into Hexidecimal bitmap
  for (uint32_t rowOf16Bits = 0; rowOf16Bits < ARROW_BITMAP_SIZE_WIDTH; rowOf16Bits++) {
    uint32_t decimalValueOfRow = convertBinaryArrayToDecimal(arrowBitMap[rowOf16Bits], ARROW_BITMAP_SIZE_WIDTH);
    char hexRow[ARROW_BITMAP_SIZE_WIDTH];
    sprintf(hexRow, "0x%02X", decimalValueOfRow);
    tempArrowBitMap[rowOf16Bits] = hexRow;
  }
  
  // Update arrow bit map
  osMutexAcquire(arrowBitMapMutex, osWaitForever);
  arrowBitMap = tempArrowBitMap;
  osMutexRelease(arrowBitMapMutex);
}




// =============================
// ======= Serial Output =======
// =============================


void writeGolfScore(void *args) {
  // -->> SERIAL OUTPUT <<--
  // MUTEX: scoreMutex
  // PROTECTED DATA: golfScore

  osMutexAcquire(scoreMutex, osWaitForever);
  printf("Golf Score: %d", golfScore);
  osMutexRelease(scoreMutex);
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
      for (int i =0; i < golfBall->power; ++i) {
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
    uint32_t currAngle;

    LPC_ADC -> ADCR |= 1<<24;
    while (!(LPC_ADC->ADGDR & (1<<31)));

    currAngle = LPC_ADC -> ADGDR & 0XFFFF;
    currAngle = currAngle >> 4;

    currAngle = currAngle / 12;  // used to convert value to 340 degrees of rotation

    // Update ball direction if potentiometer direction changes
    if (currAngle - prevPlayerDirection > 5) {  // add a hysteresis to prevent unwanted jitter 
      osMutexAcquire(ballMutex, osWaitForever);
      golfBall -> direction = currAngle;
      prevPlayerDirection = currAngle;
      osMutexRelease(ballMutex);
      printf("%d\n", currAngle - MAP_CONVERSION_ANGLE);
    } 
  }
}


// -->> PUSH BUTTON << --
// MUTEX: isHitMutex
// PROTECTED DATA: isHit
void hitBall(void *args) {
  while (1) {
    if (!(LPC_GPIO2->FIOPIN & (1<<10))) {
      osMutexAcquire(isHitMutex, osWaitForever);
      isHit = true;

      // Can set here maybe? (See below)

      osMutexRelease(isHitMutex);
    }
  }
}





// ===========================================
// ============== GAME  PHYSICS ==============
// ===========================================


// TODO: Fix overwriting of current data every time the thread is run with initial data
// i.e. can initial values be set elsewhere?
void launchBall(void *args) { // might need to change this <<<<< HERE (acceleration should always be in opposite directin of travel)
  // Calculate x and y velocities
  // -> Convert player angle for map coordinate system

  while(1) {
    osMutexAcquire(isHitMutex, osWaitForever);  // acquire hit status to ensure hit status does not change while calcs are being performed
    
    osMutexAcquire(ballMutex, osWaitForever);
  
    // Convert player direction to a usable angle
    double initialAngle = convertAngle(golfBall->direction);
    double powerLevel = golfBall->power;

    // Set up the initial velocities using the powerLevel and the initialAngle
    double currentXVelocity = powerLevel * cos(initialAngle);
    double currentYVelocity = powerLevel * sin(initialAngle);
  
    // Perform kinematics computation 
    double xPosition = golfBall->pos.x + golfBall->xVelocity * TIME + 0.5 * FRICTION_ACCEL * (pow(TIME, 2));
    double yPosition = golfBall->pos.y + golfBall->yVelocity * TIME + 0.5 * FRICTION_ACCEL * (pow(TIME, 2));

    // Change direction and set a min or max cap on the (x, y) position of the ball if outermost wall collision
    bool isXLessThanMin = xPosition < 0;
    bool isXMoreThanMax = xPosition > LCD_WIDTH;

    bool isYLessThanMin = yPosition < 0;
    bool isYMoreThanMax = yPosition > LCD_HEIGHT;

    if (isXLessThanMin || isXMoreThanMax) {
        golfBall->xVelocity = -currXVelocity;

        // Set the x position to be the min or max so that the ball does not loop 
        xPosition = (isXLessThanMin) ? 0 : LCD_WIDTH;
    }

    if (isYLessThanMin || isYMoreThanMax) {
        golfBall->yVelocity = -currYVelocity;

        // Set the y position to be the min or max so that the ball does not loop
        yPosition = (isYLessThanMin) ? 0 : LCD_HEIGHT;
    }

    // Win Condition --> may need to move 
    if (inHole(sizeof(ballBitmap)/sizeof(ballBitmap[0]), sizeof(holeBitmap)/sizeof(holeBitmap[0]))) {

    }

    // Calculate new Velocity Values
    currXVelocity + FRICTION_ACCEL * TIME;
    currYVelocity = currYVelocity + FRICTION_ACCEL * TIME;
  
    // Semaphore to signal that the incremental position computations have completed -- ready to draw
    osSemaphoreRelease(canDrawSem);
    
    osMutexRelease(ballMutex);			
  
  
    isHit = false;
    osMutexRelease(isHitMutex);
  }

  // BALL STOPS
  // Update position and draw ball of ball globally for next hit
  golfBall->pos.x = xPosition;
  golfBall->pos.y = yPosition;
  osMutexRelease(ballMutex);
}


bool inHole(int sizeBall, int sizeHole) {
	// Extract coordinates of the ball
	int xTopBall = ball->pos.x;
	int yTopBall = ball->pos.y;
	
	int xBotBall = ball->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotBall = ball->pos.y + sizeBall * SPRITE_SCALE;

	// Extract coordinates of the hole
	int xTopHole = hole->pos.x;
	int y_top_hole = hole->pos.y;
	
	int xBotHole = hole->pos.x + SPRITE_COLS * SPRITE_SCALE;
	int yBotHole = hole->pos.y + sizeHole * SPRITE_SCALE;
	
  // If this returns true, then the ball is in the hole
	return !(xTopBall < xBotHole && 
           xBotBall > xTopHole &&
           yTopBall < yBotHole &&
           yBotBall > y_top_hole);
}




// ===========================================
// ============== MISCELLANEOUS ==============
// ===========================================


// Converts the angle (degrees) obtained from potentiometer into radians and positions it correctly w.r.t. map orientation
double convertAngle(uint32_t rawAngle) {
    return (rawAngle - MAP_CONVERSION_ANGLE) * M_PI / 180 ;
}


// Converts an array of binary values to its decimal value
uint32_t convertBinaryArrayToDecimal(uint32_t *bits, uint32_t arraySize) {
  int result = 0;

  for (int i = 0; i < arraySize; i++) {
    result |= bits[i];
    if (i != arrraySize - 1) {
      result <<= 1;
    }
  }

  return result;
}