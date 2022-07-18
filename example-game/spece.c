#include "spece.h"
void drawBigPixelAt(int x, int y, int scale)
{
	int i;
	int j;
	for(i = 0; i < scale; i++)
	{
		for(j = 0; j < scale;j++)
			GLCD_PutPixel(x+i,y+j);
	}
}

//specifies top left corner.
void printSpriteAt(int x, int y,char* spriteBMP)
{
	//print the sprite left to right. Remember, x is the vertical component and y is the horizontal
	//given the orientation of the screen, joystick, and button
	int i;
	int j;
	int spriteIndex = SPRITE_ROWS-1;
	int spriteShift = 0;
	
	for(i = (SPRITE_ROWS-1)*SPRITE_SCALE; i >= 0; i-=SPRITE_SCALE)
	{
		spriteShift = 0;
		for(j = 0; j < SPRITE_COLS*SPRITE_SCALE; j+=SPRITE_SCALE)
		{
			if((spriteBMP[spriteIndex] >> spriteShift)&1)
			{
				drawBigPixelAt(x+((SPRITE_ROWS-1)*SPRITE_SCALE-i),y+j,SPRITE_SCALE);
			}
			spriteShift++; //This is the column coordinate of the sprite's bitmap
		}
		spriteIndex--; //this is the row coordinate
	}
}

//updates x and y position of the enemy
void updateEnemyPosition(actor* act)
{
	if(act->dir == 1)
	{
		if((act->horizontalPosition + SPRITE_COLS*SPRITE_SCALE + act->speed) < HORIZONTAL_LIM)
			act->horizontalPosition += act->speed;
		else
		{
			act->dir = -1;
			if((act->verticalPosition - SPRITE_ROWS*SPRITE_SCALE - act->speed )>0)
			{
				act->verticalPosition -= act->speed;
			}
		}
	}
	else
	{
		if((act->horizontalPosition - act->speed) > 0)
			act->horizontalPosition -= act->speed;
		else
		{
			act->dir = 1;
			if((act->verticalPosition - SPRITE_ROWS*SPRITE_SCALE - act->speed )>0)
			{
				act->verticalPosition -= act->speed;
			}
		}
	}
}

void printEnemy(actor* enemy)
{
	GLCD_SetTextColor(Black);
	printSpriteAt(enemy->verticalPosition,enemy->horizontalPosition,enemy->sprite);
	updateEnemyPosition(enemy);
	GLCD_SetTextColor(Green);
	printSpriteAt(enemy->verticalPosition,enemy->horizontalPosition,enemy->sprite);
}

void printPlayer(actor* play)
{
	GLCD_SetTextColor(Black);
	printSpriteAt(play->verticalPosition,play->horizontalPosition,play->sprite);
	
	if(play->dir == -1) //move left
		{
			if(play->horizontalPosition - play->speed > 0)
				play->horizontalPosition -= play->speed;
		}
		else if(play->dir == 1)//move right
		{
			if(play->horizontalPosition + play->speed + SPRITE_COLS*SPRITE_SCALE < HORIZONTAL_LIM)
				play->horizontalPosition += play->speed;
		}
		
	//updateEnemyPosition(enemy); //eventually this will be updatePlayerPosition
	GLCD_SetTextColor(Green);
	printSpriteAt(play->verticalPosition,play->horizontalPosition,play->sprite);
}

void printLaser(actor* beam)
{
	GLCD_SetTextColor(Black);
	printSpriteAt(beam->verticalPosition,beam->horizontalPosition,beam->sprite);
	
	if(beam->dir == -1) //move down
		{
			if(beam->verticalPosition - SPRITE_ROWS*SPRITE_SCALE - beam->speed > 0)
				beam->verticalPosition -= beam->speed;
			else
				beam->dir = 0;
		}
		else if(beam->dir == 1)//move up
		{
			if(beam->verticalPosition + beam->speed < VERTICAL_LIM)
				beam->verticalPosition += beam->speed;
			else
				beam->dir = 0; //the beam is gone
		}
		
	//updateEnemyPosition(enemy); //eventually this will be updatePlayerPosition
	GLCD_SetTextColor(Green);
	if(beam->dir != 0)
		printSpriteAt(beam->verticalPosition,beam->horizontalPosition,beam->sprite);
}

//determine if any part of Act 1 overlaps with Act 2
bool checkCollision(actor* act1, actor* act2, bool isSecondLaser)
{
	//extract the coordinates. It's a LOT easier this way
	//top corner act 1
	int x11;
	int y11;
	
	//top corner act 2
	int x21;
	int y21;
	
	//bottom corner act 1
	int x12;
	int y12;
	
	//bottom corner act 2
	int x22;
	int y22;
	x11 = act1->horizontalPosition;
	y11 = act1->verticalPosition;
	x12 = act1->horizontalPosition + SPRITE_COLS*SPRITE_SCALE;
	y12 = act1->verticalPosition + SPRITE_ROWS*SPRITE_SCALE;
	x21 = act2->horizontalPosition;
	y21 = act2->verticalPosition;
	x22 = act2->horizontalPosition + SPRITE_COLS*SPRITE_SCALE;
	y22 = act2->verticalPosition + SPRITE_ROWS*SPRITE_SCALE;
	
	//if the second argument is a laser, the sprite is far bigger than it needs to be, so we need to compensate
	//Note to students: you can play with these numbers to get it more precise. I didn't feel like doing that
	if(isSecondLaser)
	{
		x21 += (SPRITE_COLS-2)*SPRITE_SCALE/2;
		x22 -= (SPRITE_COLS-2)*SPRITE_SCALE/2;
	}
	
	return (x11<x22 && x12 > x21 && y11 < y22 && y12 > y21);
		
	
}
