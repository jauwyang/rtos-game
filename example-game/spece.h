#ifndef SPECE
#define SPECE

#include "GLCD.h"
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

//sprites are 6x8 bitmaps, scaled by SPRITE_SCALE
#define SPRITE_ROWS 6
#define SPRITE_COLS 8
#define SPRITE_SCALE 3

//size of the screen
#define VERTICAL_LIM 320
#define HORIZONTAL_LIM 240

//speeds
#define LASER_SPEED SPRITE_ROWS*SPRITE_SCALE
#define ENEMY_LASER_SPEED SPRITE_ROWS*SPRITE_SCALE + 5
#define PLAYER_SPEED 10
#define ENEMY_SPEED 9

//randomly selecting when the enemy can shoot
#define ENEMY_SHOT_CHANCE 25000

//useful constants
#define PLAYER_LASER 0
#define ENEMY_LASER 1

typedef struct actor_struct{
	int verticalPosition;
	int horizontalPosition;
	char* sprite;
	int dir; //direction of motion, used for enemies but not for player
	int speed; //number of pixels to move each time
}actor;

//Prints a sprite starting at that sprite's top-left corner
void printSpriteAt(int x, int y,char* spriteBMP);

/*
	draws a scale x scale square of pixels. This function allows us
	to specify sprites as 6x8 bitmaps but scale them as much as we want, 
	which saves memory at the expense of CPU cycles
*/
void drawBigPixelAt(int x, int y, int scale);

/*
	Enemy movement logic is a bit complicated, so it is done in two functions. updateEnemyPosition
	moves the enemy and changes its direction if it hits a wall, printEnemy clears the sprite and re-draws it
	in the new position
*/
//updates x and y position of the enemy
void updateEnemyPosition(actor* act);

//prints an enemy
void printEnemy(actor* enemy);


//prints the player - the logic for this is a bit easier, so it's done in one function
void printPlayer(actor* play);

//prints laser beams...pew pew. The logic here is simpler than the enemy logic but lasers move vertically
void printLaser(actor* beam);

//determines if any part of act1 overlaps with act2
bool checkCollision(actor* act1, actor* act2,bool isLaser);

#endif
