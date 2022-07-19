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
  uint32_t x;
  uint32_t y;
} Pos;


typedef struct {
	Pos pos;
	char *bitmap;
	
	double xVelocity;
	double yVelocity;
	
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
void drawBall(void *args);
void drawHole(void);

void animate(void *args);
bool inHole(int ball_size, int hole_size);

#endif
