#ifndef GAMELOGIC
#define GAMELOGIC

#include <lpc17xx.h>
#include <stdlib.h>
#include <stdio.h>
#include "GLCD.h"
#include "spece.h"
#include <cmsis_os2.h>
#include <os_tick.h>

void initLEDs(void);
void updateLEDs(void *args);
void readPowerInput(void *args);
void readDirectionInput(void *args);
void hitBall(void *args);
void launchBall(void *args);
void writeGolfScore(void *args);

double convertAngle(double rawAngle);

void initializeActors(void);

#endif
