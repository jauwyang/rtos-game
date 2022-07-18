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

typedef struct {  // The ball the player interacts with in-game is a point mass
  float x_velocity;    // Get speed and direction information from these fields
  float y_velocity;    // Velocity information is only used for the ball and not the hole
  Pos pos;
  char *bitmap;
} Actor;

void initLEDs(void);
void updateLEDs(void *args);
void readPowerInput(void *args);
void readDirectionInput(void *args);
void hitBall(void *args);
void launchBall(void *args);
void writeGolfScore(void *args);

double convertAngle(uint32_t rawAngle);

void initActors(void);

void draw(void *args);
void drawPixelsAt(int x, int y, int limit);
void drawSpriteAt(int x, int y, char *bitmap, int bitmap_size);

void drawBall(void);

void animate(void *args);
bool inHole(Actor *ball, Actor *hole, int ball_size, int hole_size);

#endif
