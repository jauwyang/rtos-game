#include "gameLogic.h"

osMutexId_t powerMutex;

const uint32_t MAX_POWER = 8;
const uint32_t MIN_POWER = 1;

uint32_t powerLevel = 1;

osMutexId_t playerDirectionMutex;
const uint32_t MAX_ANGLE = 340;
uint32_t playerDirection = 0;  // direction stored in degrees
uint32_t prevPlayerDirection = playerDirection;  // allows previous value to be read without having to access mutex-protected data of playerDirection

osMutexId_t ballMutex;

// Golf Score
int score;  // to be initialized to 0 in main

// Struct to store the (x, y) coordinates of in-game sprites
typedef struct {
  uint32_t x;
  uint32_t y;
} Pos;

// The ball the player interacts with in-game is a point mass
typedef struct {
  float x_velocity;    // Get direction from these fields
  float y_velocity;
  Pos pos;
} Ball;

Ball golf_ball;

void hit_ball(void *args) {
  while (1) {
    if (LPC_GPIO2->FIOPIN & (1<<10)) {
      osMutexAcquire(powerMutex, osWaitForever);
      osMutexAcquire();
      
      computeTrajectories();

      osMutexRelease(powerMutex);
      osMutexRelease(ballMutex);
    }
  }
}

void computeTrajectories(void *args) {
  // check if ball hit is true
  osMutexAcquire(playerDirectionMutex, osWaitForever);
  // An angle of 140 will be facing upwards, 
  uint32_t ballAngle = playerDirection;
  osMutexRelease(playerDirectionMutex);
  
  osMutexAcquire(powerMutex, osWaitForever);
  uint32_t ballSpeed = powerLevel;
  osMutexRelease(powerMutex);

  double xVelocity = ballSpeed * tan(ballAngle);
  double yVelocity = ballSpeed * tan()
}

void readPowerInput(void *args) {
  while (1) {
    // 0 means ON, 1 means OFF
    // MAKE SURE THE VALUE DOES NOT GO BELOW 1 OR ABOVE 8
    if (!(LPC_GPIO1->FIOPIN & (1 << 26))) {  // Pull Joystick down (decrease power level)
      if (powerLevel > MIN_POWER) {
          osMutexAcquire(powerMutex, osWaitForever);
          powerLevel--;
          osMutexRelease(powerMutex);
      }
    } else if (!(LPC_GPIO1->FIOPIN & (1 << 24))) {  // Push Joystick up (increase power level)
      if (powerLevel < MAX_POWER) {
        osMutexAcquire(powerMutex, osWaitForever);
        powerLevel++;
        osMutexRelease(powerMutex);
      }
    }
  }
}


// todo: SET POTENTIOMETER INPUT
void readDirectionInput(void *args) {
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
  while (1) {
    osMutexAcquire(powerMutex, osWaitForever);

    int onLEDs[MAX_POWER];
    for (int i = 0; i < powerLevel; ++i) {
      onLEDs[i] = 1;
    }

    // First LED -- may not need this check since MIN_POWER == 1
    if (onLEDs[0] == 1) {
      LPC_GPIO1->FIOSET |= 1<<28;
    }
    else {
      LPC_GPIO1->FIOCLR |= 1<<28;
    }

    // Second LED
    if (onLEDs[1] == 1) {
      LPC_GPIO1->FIOSET |= 1<<29;
    }
    else {
      LPC_GPIO1->FIOCLR |= 1<<29;
    }

    // Third LED
    if (onLEDs[2] == 1) {
      LPC_GPIO1->FIOSET |= 1<<31;
    }
    else {
      LPC_GPIO1->FIOCLR |= 1<<31;
    }

    // Fourth LED
    if (onLEDs[3] == 1) {
      LPC_GPIO2->FIOSET |= 1<<2;
    }
    else {
      LPC_GPIO2->FIOCLR |= 1<<2;
    }

    // Fifth LED
    if (onLEDs[4] == 1) {
      LPC_GPIO2->FIOSET |= 1<<3;
    }
    else {
      LPC_GPIO2->FIOCLR |= 1<<3;
    }

    // Sixth LED
    if (onLEDs[5] == 1) {
      LPC_GPIO2->FIOSET |= 1<<4;
    }
    else {
      LPC_GPIO2->FIOCLR |= 1<<4;
    }

    // Seventh LED
    if (onLEDs[6] == 1) {
      LPC_GPIO2->FIOSET |= 1<<5;
    }
    else {
      LPC_GPIO2->FIOCLR |= 1<<5;
    }

    // Eighth LED
    if (onLEDs[7] == 1) {
      LPC_GPIO2->FIOSET |= 1<<6;
    }
    else {
      LPC_GPIO2->FIOCLR |= 1<<6;
    }
    osMutexRelease(powerMutex);
  }
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
  */
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
}