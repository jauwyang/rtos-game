#ifndef GAMELOGIC
#define GAMELOGIC

#include <lpc17xx.h>
#include <stdlib.h>
#include <stdio.h>
#include "GLCD.h"
#include "spece.h"
#include <cmsis_os2.h>
#include <os_tick.h>

// Struct to store the (x, y) coordinates of in-game sprites
typedef struct { 
  int x;
  int y;
} Pos;

// Struct for the Ball sprite
typedef struct {
	Pos pos;
	Pos arrowPos[2];
	char *bitmap;
	
	int xVelocity;
	int yVelocity;
	
	double direction;
	uint32_t power;
	
} Ball;

// Struct to store environment objects, i.e. the Hole and the Teleporter
typedef struct {
	Pos pos;
	char *bitmap;
	
} Environment;

//***** LEDs and Power Mechanism *****//
void initLEDs(void);
void updateLEDs(void *args);
void readPowerInput(void *args);

//***** POTENTIOMETER and Angle Mechanism *****//
void readDirectionInput(void *args);


//***** PUSHBUTTON, Ball Control, and In-Game Functionality *****//	
void hitBall(void *args);
void launchBall(void);
void teleportBall(void *args);
bool inTeleporter(int sizeBall, int sizeTeleporter);

//***** SETUP, LOSE AND WIN CONDITION *****//
void setupGame(void);
void writeGolfScore(void *args);
void checkEndGame(void *args);
bool inHole(int ball_size, int hole_size);

//***** DRAWING and HELPER FUNCTIONS *****//
void drawPixelsAt(int x, int y, int limit);
void drawSpriteAt(int x, int y, char *bitmap, int bitmap_size);
double convertAngle(int32_t rawAngle);
uint32_t convertBinaryArrayToDecimal(uint32_t *bits, uint32_t arraySize);

#endif
