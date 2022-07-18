#include "gameLogic.h"

#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif


// Power Mutex and Data
extern osMutexId_t powerMutex;
const uint32_t MAX_POWER = 8;
const uint32_t MIN_POWER = 1;
uint32_t powerLevel = 1;

// Player Direction Mutex and Data
// Note: the playerDirection is kept in degrees and is not converted in accordance to the map. It must be updated immediately before using.
extern osMutexId_t playerDirectionMutex;
uint32_t playerDirection = 0;  // direction stored in degrees
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

char ball_bitmap[] = {0x38, 0x38, 0x38};
char hole_bitmap[] = {0x38, 0x38, 0x38};



// ================================
// ====== GLCD + Draw on GLCD =====
// ================================

// Initializes in-game elements
// (1) Golf Score
// (2) Golf Ball State
// (3) Hole State

const uint32_t ARROW_BITMAP_SIZE = 15;
const uint32_t ARROW_BITMAP_MID = (ARROW_BITMAP_SIZE + 1) / 2;


void setupGame(void) {
  golfScore = 0;
  
  golfBall = malloc(sizeof(Ball));
  golfBall->pos.x = rand() % ((LCD_WIDTH - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->pos.y = rand() % ((LCD_HEIGHT - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->bitmap = ball_bitmap;

  hole = malloc(sizeof(Hole));
  hole->bitmap = hole_bitmap;

  // Hole location cannot be same as ball location when spawned
  do {
    hole->pos.x = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
    hole->pos.y = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
  } while (hole->pos.x == golfBall->pos.x && hole->pos.y == golfBall->pos.y);
}

void drawDirectionArrow() {
    double mappedDirection = convertAngle(playerDirection);
    for (uint32_t i = 1; i < DIRECTION_ARROW_SIZE + 1; i++) {  
        uint32_t x = i*cos(mappedDirection);
        uint32_t y = i*sin(mappedDirection);
        
        GLCD_PutPixel(x + golfBall->pos.x, y + golfBall->pos.y);
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
	
	for(int i =(bitmap_size - 1) * SPRITE_SCALE; i >= 0; i -= SPRITE_SCALE) {
		spriteShift = 0;
		for(int j = 0; j < SPRITE_COLS*SPRITE_SCALE; j+=SPRITE_SCALE) {
			if((bitmap[spriteIndex] >> spriteShift)&1) {
				drawPixelsAt(x + ((bitmap_size - 1) * SPRITE_SCALE - i),y + j, SPRITE_SCALE);
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

void draw(void *args) {
	/*for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 100; ++j) {
				GLCD_SetTextColor(Green);
				drawSpriteAt(j, i, ball_bitmap, 3);
			
				GLCD_SetTextColor(Black);
				drawSpriteAt(j + 1, i + 1, ball_bitmap, 3);
		}
	}*/
	while (1) {
		for (int i = 0; i < 100; ++i) {
			GLCD_SetTextColor(Green);
			drawSpriteAt(0, i, ball_bitmap, 3);
		
			GLCD_SetTextColor(White);
			drawSpriteAt(0, i + 1, ball_bitmap, 3);
		}		
	}

}

// ================================
// ===== DRAW HELPER FUNCTIONS ====
// ================================





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

void updateLEDs(void *args) {
  // -->> LEDs <<-- 
  // MUTEX: powerMutex
  // PROTECTED DATA: powerLevel

  while (1) {
		  /*LPC_GPIO1->FIOSET |= (1 << 28);
			LPC_GPIO1->FIOSET |= (1 << 29);
		  LPC_GPIO1->FIOSET |= (1 << 31);
		  LPC_GPIO2->FIOSET |= (1 << 2);*/

      // Initialize array of ints to store
      bool ledsOn[MAX_POWER] = {false, false, false, false, false, false, false, false};
      
      osMutexAcquire(powerMutex, osWaitForever);
      for (int i =0; i < powerLevel; ++i) {
        ledsOn[i] = true;
      }
      osMutexRelease(powerMutex);

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
// ====== READ USER INPUT FROM JOYSTICK ======
// ===========================================

void readPowerInput(void *args) {
  //  -->> JOYSTICK <<--
  // MUTEX: powerMutex
  // PROTECTED DATA: powerLevel

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
        osMutexAcquire(powerMutex, osWaitForever);
        if (powerLevel > MIN_POWER) {
					osDelay(150);		// Delay changes power level so it is slow enough for user control
          powerLevel--;
        }
        osMutexRelease(powerMutex);
      }

      // Joytick was pushed up, toward P.24 label and current state is not the same as previous state
      else if (currStateP24 && currStateP24 != lastStateP24) {
        osMutexAcquire(powerMutex, osWaitForever);
        if (powerLevel < MAX_POWER) {
					osDelay(150);
          powerLevel++;
        }
        osMutexRelease(powerMutex);
      }
  }
}

// todo: SET POTENTIOMETER INPUT
void readDirectionInput(void *args) {
    //  -->> POTENTIOMETER <<--
    // MUTEX: playerDirectionMutex
    // PROTECTED DATA: playerDirection
    
    while (1) {
        // Read current angle of potentiometer
        uint32_t currAngle;

        LPC_ADC -> ADCR |= 1<<24;
        while (!(LPC_ADC->ADGDR & (1<<31)));

        currAngle = LPC_ADC -> ADGDR & 0XFFFF;
        currAngle = currAngle >> 4;

        currAngle = currAngle / 12;  // used to convert value to 340 degrees of rotation

        // Update playerDirection if potentiometer direction changes
        if (currAngle - prevPlayerDirection > 5) {  // add a hysteresis to prevent unwanted jitter 
            osMutexAcquire(playerDirectionMutex, osWaitForever);
            playerDirection = currAngle;
            prevPlayerDirection = playerDirection;
            osMutexRelease(playerDirectionMutex);
            printf("%d\n", playerDirection - MAP_CONVERSION_ANGLE);
        } 
    }
}

void hitBall(void *args) {
    // -->> PUSH BUTTON << --
    // MUTEX: isHitMutex
    // PROTECTED DATA: isHit
    while (1) {
        if (!(LPC_GPIO2->FIOPIN & (1<<10))) {
						osMutexAcquire(isHitMutex, osWaitForever);
						isHit = true;
            osMutexRelease(isHitMutex);
        }
    }
}

// ===========================================
// ============== GAME  PHYSICS ==============
// ===========================================

void launchBall(void *args) { // might need to change this <<<<< HERE
    // Calculate x and y velocities
    // -> Convert player angle for map coordinate system
		while(1) {
				osMutexAcquire(isHitMutex, osWaitForever);  // acquire hit status to ensure hit status does not change while calcs are being performed
				
				osMutexAcquire(ballMutex, osWaitForever);
			
				// Convert player direction to a usable angle
				double initialAngle = convertAngle(playerDirection);
				
				// Set up the initial velocities using the powerLevel and the initialAngle
				double currXVelocity = powerLevel * cos(initialAngle);
				double currYVelocity = powerLevel * sin(initialAngle);
			
				// Get the initial (x, y) position of the ball
			  double xPosition = golfBall->pos.x;
				double yPosition = golfBall->pos.y;
			
				xPosition = xPosition + currXVelocity*TIME + 0.5*FRICTION_ACCEL*(pow(TIME, 2));
        yPosition = yPosition + currYVelocity*TIME + 0.5*FRICTION_ACCEL*(pow(TIME, 2));
			
        // Change direction if wall collision
        if ((xPosition > LCD_WIDTH) || (xPosition < 0)) {
            currXVelocity = -currXVelocity;
        }
        if ((yPosition > LCD_HEIGHT) || (yPosition < 0)) {
            currYVelocity = -currYVelocity;
        }

        // Calculate new Velocity Values
        currXVelocity = currXVelocity + FRICTION_ACCEL*TIME;
        currYVelocity = currYVelocity + FRICTION_ACCEL*TIME;
			
				// Semaphore to signal that the incremental position computations have completed -- readdy to draw
				osSemaphoreRelease(canDrawSem);
				
				osMutexRelease(ballMutex);			
			
			
				isHit = false;
				osMutexRelease(isHitMutex);
		}
	
		osMutexAcquire(ballMutex, osWaitForever);


    //while (fabs(currXVelocity) > ZERO_TOLERANCE || fabs(currYVelocity) > ZERO_TOLERANCE) { // while there the ball still has velocity
        // Calculate new Position Values and draw ball incrementally
				GLCD_SetTextColor(Green);
				drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ball_bitmap, sizeof(ball_bitmap)/sizeof(ball_bitmap[0]));
			  

				
				golfBall->pos.x = xPosition;
				golfBall->pos.y = yPosition;
			
				GLCD_SetTextColor(Black);
			  drawSpriteAt(golfBall->pos.x, golfBall->pos.y, ball_bitmap, sizeof(ball_bitmap)/sizeof(ball_bitmap[0]));

        /*// Calculate in Hole
        if (inHole()) {
            break; // << WHAT TO DO <<< HERE
        }*/



    //}

		// BALL STOPS
    // Update position and draw ball of ball globally for next hit
    golfBall->pos.x = xPosition;
    golfBall->pos.y = yPosition;
		osMutexRelease(ballMutex);
}


bool inHole(Ball *ball, Hole *hole, int size_ball, int size_hole) {
	// Extract coordinates of the ball
	int x_top_ball = ball->pos.x;
	int y_top_ball = ball->pos.y;
	
	int x_bot_ball = ball->pos.x + SPRITE_COLS*SPRITE_SCALE;
	int y_bot_ball = ball->pos.y + size_ball*SPRITE_SCALE;

	// Extract coordinates of the hole
	int x_top_hole = hole->pos.x;
	int y_top_hole = hole->pos.y;
	
	int x_bot_hole = hole->pos.x + SPRITE_COLS*SPRITE_SCALE;
	int y_bot_hole = hole->pos.y + size_hole*SPRITE_SCALE;
	
	
}



// ===========================================
// ============== MISCELLANEOUS ==============
// ===========================================

double convertAngle(uint32_t rawAngle) {
    return (rawAngle - MAP_CONVERSION_ANGLE) * M_PI / 180 ;
}

/*
==================================
*/

/*****  *****/

/*
        These functions rely on the existence of global variables in order to
   simplify the threading code for lab 1. They are declared in main, so must be
   listed as extern here
*/

/*
extern actor *player;
extern actor *enemy;
extern actor *lasers[2];
extern osThreadId_t animateID;
// sprites for the enemy, player, and laser bolt
char sprite[] = {0x81, 0xA5, 0xFF, 0xE7, 0x7E, 0x24};
char playerSprite[] = {0x00, 0x18, 0x3C, 0xFF, 0xFF, 0xFF};
char laserSprite[] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
void readPlayerInput(void *args) {
  while (1) {
    // check the joystick for movement
    if (!(LPC_GPIO1->FIOPIN & (1 << 23))) // 0 means ON, 1 means OFF
    {
      player->dir = -1;
    } else if (!(LPC_GPIO1->FIOPIN & (1 << 25))) {
      player->dir = 1;
    } else
      player->dir = 0;
    // check the button for laser shooting
    if (!(LPC_GPIO2->FIOPIN & (1 << 10))) {
      if (lasers[PLAYER_LASER]->dir == 0) {
        // put it in the middle
        lasers[PLAYER_LASER]->horizontalPosition =
            player->horizontalPosition + (SPRITE_ROWS - 2) * SPRITE_SCALE / 2;
        lasers[PLAYER_LASER]->verticalPosition =
            player->verticalPosition + (SPRITE_COLS - 1) * SPRITE_SCALE;
        lasers[PLAYER_LASER]->dir = 1;
      }
    }
    osThreadYield();
  }
}
void enemyShoot(actor *enemy) {
  // put it in the middle
  lasers[ENEMY_LASER]->horizontalPosition =
      enemy->horizontalPosition + (SPRITE_ROWS - 2) * SPRITE_SCALE / 2;
  lasers[ENEMY_LASER]->verticalPosition =
      enemy->verticalPosition - (SPRITE_COLS - 1) * SPRITE_SCALE;
  lasers[ENEMY_LASER]->dir = -1;
}
// definitely screw this up with both being task 1 or something
void animate(void *args) {
  while (1) {
    printEnemy(enemy);
    // see if there is shooting to do
    if (rand() > ENEMY_SHOT_CHANCE && lasers[ENEMY_LASER]->dir == 0)
      enemyShoot(enemy);
    if (player->dir != 0) // we don't want to print more than we need. The print
                          // logic is a bit more complex than for enemies
      printPlayer(player);
    if (lasers[PLAYER_LASER]->dir != 0)
      printLaser(lasers[PLAYER_LASER]);
    if (lasers[1]->dir != 0)
      printLaser(lasers[ENEMY_LASER]);
    osDelay(100U);
  }
}
// thread to check for end-game conditions
void checkEndGame(void *args) {
  while (1) {
    // endgame in three ways: enemy hits player, bullet hits player (both lose),
    // bullet hits enemy (win!)
    if (checkCollision(enemy, player, 0))
      osThreadTerminate(animateID);
    else if (checkCollision(enemy, lasers[PLAYER_LASER], 1) &&
             lasers[PLAYER_LASER]->dir != 0)
      osThreadTerminate(animateID);
    else if (checkCollision(player, lasers[ENEMY_LASER], 1) &&
             lasers[ENEMY_LASER]->dir != 0)
      osThreadTerminate(animateID);
    osThreadYield();
  }
}
void initializeActors() {
  /*
          Generally this should be handled by a separate function. However, it
     made sense for simplicity of understanding the code that the first thing we
     do is initialize the actors, so we did this in main instead.
  
  enemy = malloc(sizeof(actor));
  enemy->horizontalPosition = 20; // start almost in the top corner
  enemy->verticalPosition = 300;
  enemy->speed = ENEMY_SPEED;
  enemy->dir = 1;
  enemy->bitmap = sprite;
  player = malloc(sizeof(actor));
  player->horizontalPosition = 10; // start in a bottom corner
  player->verticalPosition = 20;
  player->speed = PLAYER_SPEED;
  player->dir = 0; // set to zero until the player moves
  player->bitmap = playerSprite;
  // init the two lasers, but set their dir to zero so they don't appear or
  // affect anything
  lasers[PLAYER_LASER] = malloc(sizeof(actor));
  lasers[PLAYER_LASER]->horizontalPosition = 0;
  lasers[PLAYER_LASER]->verticalPosition = 0;
  lasers[PLAYER_LASER]->speed = LASER_SPEED;
  lasers[PLAYER_LASER]->dir = 0;
  lasers[PLAYER_LASER]->sprite = laserSprite;
  lasers[ENEMY_LASER] = malloc(sizeof(actor));
  lasers[ENEMY_LASER]->horizontalPosition = 0;
  lasers[ENEMY_LASER]->verticalPosition = 0;
  lasers[ENEMY_LASER]->speed = ENEMY_LASER_SPEED;
  lasers[ENEMY_LASER]->dir = 0;
  lasers[ENEMY_LASER]->sprite = laserSprite;
}*/
