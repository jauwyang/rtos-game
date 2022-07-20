#ifndef GAMELOGIC
#define GAMELOGIC

#include <lpc17xx.h>
#include <stdlib.h>
#include <stdio.h>
#include "GLCD.h"
#include "spece.h"
#include <cmsis_os2.h>
#include <os_tick.h>

typedef struct { // Struct to store the (x, y) coordinates of in-game sprites
  int x;
  int y;
} Pos;


typedef struct {
	Pos pos;
	char *bitmap;
	
	int xVelocity;
	int yVelocity;
	
	double direction;
	uint32_t power;
	
} Ball;


typedef struct {
	Pos pos;
	char *bitmap;
	
} Hole;


void initLEDs(void);
void updateLEDs(void *args);
void readPowerInput(void *args);
void readDirectionInput(void *args);
void hitBall(void *args);
void launchBall(void);
void writeGolfScore(void *args);
void setupGame(void);

double convertAngle(uint32_t rawAngle);

void initActors(void);

void drawPixelsAt(int x, int y, int limit);
void drawSpriteAt(int x, int y, char *bitmap, int bitmap_size);

void drawBall(void);
void orientStillBall(double unprocAngle);

void createArrowBitMap(void *args);
uint32_t convertBinaryArrayToDecimal(uint32_t *bits, uint32_t arraySize);
void drawArrow(void *args);

void checkEndGame(void *args);

void animate(void *args);
bool inHole(int ball_size, int hole_size);

#endif
