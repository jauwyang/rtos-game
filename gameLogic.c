#include "gameLogic.h"

#include <stdbool.h>
#include <math.h>


// Power Mutex and Data
extern osMutexId_t powerMutex;
const uint32_t MAX_POWER = 8;
const uint32_t MIN_POWER = 1;
uint32_t powerLevel = 1;

// Player Direction Mutex and Data
// Note: the playerDirection is kept in degrees and is not converted in accordance to the map. It must be updated immediately before using.
extern osMutexId_t playerDirectionMutex;
uint32_t playerDirection = 0;  // direction stored in degrees
uint32_t prevPlayerDirection = 0;  // allows previous value to be read without having to access mutex-protected data of playerDirection
const uint32_t MAP_CONVERSION_ANGLE = 80; // Note: 90 degrees means straight up.

// Position Struct, Ball Mutex and Data
extern osMutexId_t ballMutex;
typedef struct { // Struct to store the (x, y) coordinates of in-game sprites
  uint32_t x;
  uint32_t y;
} Pos;

typedef struct {  // The ball the player interacts with in-game is a point mass
  float x_velocity;    // Get speed and direction information from these fields
  float y_velocity;    // Velocity information is only used for the ball and not the hole
  Pos pos;
  char *bitmap;
} Actor;

Actor *golfBall;
Actor *hole;

// Ball Physics Values
const int32_t FRICTION_ACCEL = -5;
const double ZERO_TOLERANCE = 0.001;
const uint32_t TIME = 1;

// Golf Score Mutex and Data
int golfScore;  
extern osMutexId_t scoreMutex;


// GLCD
extern osThreadId_t animateID;

// Mapping Data Values
// -> Note: - The GLCD values are set for portrait view
//          - The Bottom Left will be x = y = 0??? (CHECK)<<<<<< HERE
const uint32_t LCD_HEIGHT = 320;  // y-axis
const uint32_t LCD_WIDTH = 240;  // x-axis

// Sizes and bitmaps
const uint32_t BALL_SIZE = 5;
const uint32_t HOLE_SIZE = 5;
const uint32_t DIRECTION_ARROW_SIZE = 7;

char ball_bitmap[] = {};
char hole_bitmap[] = {};
char arrow_bitmap[] = {};


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
  
  golfBall = malloc(sizeof(Actor));
  golfBall->pos.x = rand() % ((LCD_WIDTH - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->pos.y = rand() % ((LCD_HEIGHT - BALL_SIZE) - BALL_SIZE + 1) + BALL_SIZE;
  golfBall->sprite = ball_bitmap;

  hole = malloc(sizeof(Actor));
  hole->sprite = hole_bitmap;

  // Hole location cannot be same as ball location when spawned
  do {
    hole.x = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
    hole.y = rand() % ((LCD_WIDTH - HOLE_SIZE) - HOLE_SIZE + 1) + HOLE_SIZE;
  } while (hole.x == golfBall.pos.x && hole.y == golfBall.pos.y);

  
}

void createArrowBitMap(Ball ball) {
  double mappedDirection = convertAngle(playerDirection);

  // Initialize Empty Bitmap with 1 at center
  uint32_t arrow[ARROW_BITMAP_SIZE][ARROW_BITMAP_SIZE] = {0};
  arrow[ARROW_BITMAP_MID-1][ARROW_BITMAP_MID-1] = 1;  // make center of bit map (where the ball would be) 1
  
  // Draw Arrow in 2D Bitmap starting at center of map
  for (uint32_t i = 1; i < DIRECTION_ARROW_SIZE + 1; i++) {  
      uint32_t x = round(i*cos(mappedDirection));
      uint32_t y = round(i*sin(mappedDirection));
      arrow[x + ARROW_BITMAP_MID - 1][y+ ARROW_BITMAP_MID - 1] = 1;
      //GLCD_PutPixel(x + ball->pos.x, y + ball->pos.y);
  }

  char arrowBitMap





}

// ================================
// ===== DRAW HELPER FUNCTIONS ====
// ================================

void drawPixelsAt(int x, int y) {
  for (int i = 0; i < )  IAM DONE POOPING OK 
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

void updateLEDs(void *args) {
  // -->> LEDs <<-- 
  // MUTEX: powerMutex
  // PROTECTED DATA: powerLevel

  while (1) {

      // Initialize array of ints to store
      bool ledsOn[MAX_POWER];
      
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
          powerLevel--;
          printf("Power Level: %d\n", powerLevel);
        }
        osMutexRelease(powerMutex);
      }

      // Joytick was pushed up, toward P.24 label and current state is not the same as previous state
      else if (currStateP24 && currStateP24 != lastStateP24) {
        osMutexAcquire(powerMutex, osWaitForever);
        if (powerLevel < MAX_POWER) {
          powerLevel++;
          printf("Power Level: %d\n", powerLevel);
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
            //printf("%d\n", playerDirection);
        } 
    }
}

void hitBall(void *args) {
    // -->> PUSH BUTTON << --
    // MUTEX: powerMutex, ballMutex, playerDirectionMutex
    // PROTECTED DATA: powerLevel, ball, playerDirection
    
    while (1) {
        if (LPC_GPIO2->FIOPIN & (1<<10)) {
            osMutexAcquire(powerMutex, osWaitForever);
            osMutexAcquire(ballMutex, osWaitForever);
            osMutexAcquire(playerDirectionMutex, osWaitForever);

            launchBall();

            osMutexRelease(playerDirectionMutex);
            osMutexRelease(ballMutex);
            osMutexRelease(powerMutex);
        }
    }
}

// ===========================================
// ============== GAME  PHYSICS ==============
// ===========================================

void launchBall(void *args) { // might need to change this <<<<< HERE
    // Calculate x and y velocities
    // -> Convert player angle for map coordinate system 
    double initialAngle = convertAngle(playerDirection);

    double initXVelocity = powerLevel * cos(initialAngle);
    double initYVelocity = powerLevel * sin(initialAngle);

    double currXVelocity = initXVelocity;
    double currYVelocity = initYVelocity;

    double xPosition = golfBall->pos.x;
    double yPosition = golfBall->pos.y;

    while (fabs(currXVelocity) > ZERO_TOLERANCE || fabs(currYVelocity) > ZERO_TOLERANCE) { // while there the ball still has velocity
        // Calculate new Position Values
        xPosition = xPosition + currXVelocity*TIME + 0.5*FRICTION_ACCEL*(pow(TIME, 2));
        yPosition = yPosition + currYVelocity*TIME + 0.5*FRICTION_ACCEL*(pow(TIME, 2));

        // Calculate in Hole
        if (inHole()) {
            break; // << WHAT TO DO <<< HERE
        }

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

    }

    // Update position of ball globally for next hit
    golfBall->pos.x = xPosition;
    golfBall->pos.y = yPosition;
}

bool inHole(Ball ball) {

    // insert hole detection here
}

// ===========================================
// =============== DRAW ON LCD ===============
// ===========================================



// ===========================================
// ============== MISCELLANEOUS ==============
// ===========================================

double convertAngle(uint32_t rawAngle) {
    return (rawAngle - MAP_CONVERSION_ANGLE) * 2*M_PI / 360;
}

uint32_t convertBinaryArrayToDecimal(uint32_t* bits, uint32_t arraySize) {
  int result = 0;

  for (int i = 0; i < arraySize; i++) {
    result |= bits[i];
    if (i != arrraySize - 1) {
      result <<= 1;
    }
  }

  return result;
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
  enemy->sprite = sprite;

  player = malloc(sizeof(actor));
  player->horizontalPosition = 10; // start in a bottom corner
  player->verticalPosition = 20;
  player->speed = PLAYER_SPEED;
  player->dir = 0; // set to zero until the player moves
  player->sprite = playerSprite;

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

//////////////////////////////////////// HEREREREREERE
/*
#include <stdio.h>
#include <math.h>
#include <stdint.h>
const int ARROW_BITMAP_SIZE = 15;
const int ARROW_BITMAP_MID = (ARROW_BITMAP_SIZE + 1) / 2;
const double PI = 3.14;

const int L = 7;
const double theta = 254;

double deg2rad(double angle) {
  return (angle/180)*PI;
}


int ConvertToDec(int* bits, int size)
{
  int result = 0;

    for (int i = 0; i < size; i++) {

        result |= bits[i];
        if(i != size-1)
            result <<= 1;
    }

    return result;

}




int main() {
  int arrow[ARROW_BITMAP_SIZE][ARROW_BITMAP_SIZE] = {0};
  arrow[ARROW_BITMAP_MID-1][ARROW_BITMAP_MID-1] = 1;
  for (int i = L-1; i >= 0; i--) {
    int x = round(i*cos(deg2rad(theta)));
    int y = round(i*sin(deg2rad(theta)));
    
    arrow[x+ARROW_BITMAP_MID-1][y+ARROW_BITMAP_MID-1] = 1;
  }
  // Print out 2D ARRAY
  for (int i = 0; i < ARROW_BITMAP_SIZE; i++) {
    for (int j = 0; j < ARROW_BITMAP_SIZE; j++) {
      printf("%d ", arrow[i][j]);
    }
    printf("\n");
  }

  char arrowBitMap[15] = {};
  for (int row = 0; row < ARROW_BITMAP_SIZE; row++) {
    int result = ConvertToDec(arrow[row], ARROW_BITMAP_SIZE);
    char hex[15];
    sprintf(hex, "0x%02X", result);
    arrowBitMap[row] = hex;
    printf("%s\n", hex);
  }
  //int result = ConvertToDec(arrow[6], ARROW_BITMAP_SIZE);
  //printf("%d\n", result);
  //char arrowBitMap[20] = {};
  
  //sprintf(hex, "0x%02X", result);
  
  
  return 0;
}
*/
