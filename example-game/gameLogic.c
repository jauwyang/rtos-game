#include "gameLogic.h"

/*
	These functions rely on the existence of global variables in order to simplify
	the threading code for lab 1. They are declared in main, so must be listed as extern here
*/
extern actor* player;
extern actor* enemy;
extern actor* lasers[2];
extern osThreadId_t animateID;

//sprites for the enemy, player, and laser bolt
char sprite[] = {0x81,0xA5,0xFF,0xE7,0x7E,0x24};
char playerSprite[] = {0x00,0x18,0x3C, 0xFF, 0xFF, 0xFF};
char laserSprite[] = {0x18,0x18,0x18,0x18,0x18,0x18};

void readPlayerInput(void *args)
{
	while(1)
	{
		//check the joystick for movement
		if(!(LPC_GPIO1->FIOPIN & (1<<23))) //0 means ON, 1 means OFF
		{
			player->dir = -1;
		}
		else if(!(LPC_GPIO1->FIOPIN & (1<<25)))
		{
			player->dir = 1;
		}
		else
			player->dir = 0;
		
		//check the button for laser shooting
		if(!(LPC_GPIO2->FIOPIN & (1<<10)))
		{
			if(lasers[PLAYER_LASER]->dir == 0)
			{
				//put it in the middle
				lasers[PLAYER_LASER]->horizontalPosition = player->horizontalPosition + (SPRITE_ROWS-2)*SPRITE_SCALE/2;
				lasers[PLAYER_LASER]->verticalPosition = player->verticalPosition + (SPRITE_COLS-1)*SPRITE_SCALE;
				lasers[PLAYER_LASER]->dir = 1;
			}
		}
		osThreadYield();
	}
}

void enemyShoot(actor* enemy)
{
	//put it in the middle
	lasers[ENEMY_LASER]->horizontalPosition = enemy->horizontalPosition + (SPRITE_ROWS-2)*SPRITE_SCALE/2;
	lasers[ENEMY_LASER]->verticalPosition = enemy->verticalPosition - (SPRITE_COLS-1)*SPRITE_SCALE;
	lasers[ENEMY_LASER]->dir = -1;
}

//definitely screw this up with both being task 1 or something
void animate(void *args)
{
	while(1)
	{
		printEnemy(enemy);
		
		//see if there is shooting to do
		if(rand() > ENEMY_SHOT_CHANCE && lasers[ENEMY_LASER]->dir == 0)
			enemyShoot(enemy);
		
		if(player->dir != 0) //we don't want to print more than we need. The print logic is a bit more complex than for enemies
			printPlayer(player);
		
		if(lasers[PLAYER_LASER]->dir != 0)
			printLaser(lasers[PLAYER_LASER]);
		
		if(lasers[1]->dir != 0)
			printLaser(lasers[ENEMY_LASER]);
	
		osDelay(100U);
	}
}

//thread to check for end-game conditions
void checkEndGame(void* args)
{
	while(1)
	{
		//endgame in three ways: enemy hits player, bullet hits player (both lose), bullet hits enemy (win!)
		if(checkCollision(enemy, player,0))
			osThreadTerminate(animateID);
		else if(checkCollision(enemy,lasers[PLAYER_LASER],1) && lasers[PLAYER_LASER]->dir != 0)
			osThreadTerminate(animateID);
		else if(checkCollision(player,lasers[ENEMY_LASER],1) && lasers[ENEMY_LASER]->dir != 0)
			osThreadTerminate(animateID);
	
		osThreadYield();
	}
}

void initializeActors()
{
	/*
		Generally this should be handled by a separate function. However, it made sense
		for simplicity of understanding the code that the first thing we do is initialize the 
		actors, so we did this in main instead.
	*/
	enemy = malloc(sizeof(actor));
	enemy->horizontalPosition = 20; //start almost in the top corner
	enemy->verticalPosition = 300;
	enemy->speed = ENEMY_SPEED;
	enemy->dir = 1;
	enemy->sprite = sprite;
	
	player = malloc(sizeof(actor));
	player->horizontalPosition = 10; //start in a bottom corner
	player->verticalPosition = 20;
	player->speed = PLAYER_SPEED;
	player->dir = 0; //set to zero until the player moves
	player->sprite = playerSprite;
	
	//init the two lasers, but set their dir to zero so they don't appear or affect anything
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
