#include "gameLogic.h"

osMutexId_t powerMutex;
osMutexId_t playerDirectionMutex;
osMutexId_t ballMutex;
osMutexId_t scoreMutex;

osSemaphoreId_t canDrawSem;

int main()
{
	osKernelInitialize();
	SystemInit();
	printf("Game Ready.");
	
	// Configure the LEDs as outputs
	initLEDs();
	

	// Initialize the GLCD

	GLCD_Init();
	GLCD_Clear(Green);
	GLCD_SetTextColor(Green);
	
	// Setting up and enabling the ADC
	LPC_PINCON->PINSEL1 &= ~(1<<18);
	LPC_PINCON->PINSEL1 &= ~(1<<19);
	LPC_PINCON->PINSEL1 |= 1<<18;
	
	// Power ON ADC
	LPC_SC->PCONP |= 1<<12;
	
	// Select pin 2, set CLK frequency, enable the ADC
	LPC_ADC->ADCR = (1<<2) | (4<<8) | (1<<21);


	// Init the semaphore
	canDrawSem = osSemaphoreNew(1, 0 , NULL);
	
	// Create the mutexes
	powerMutex = osMutexNew(NULL);
	playerDirectionMutex = osMutexNew(NULL);
	ballMutex = osMutexNew(NULL);
	scoreMutex = osMutexNew(NULL);
	
	// Set up the Actors (Hole and Ball)
	setupGame();
	drawBall();
	drawHole();

	// Create the threads
	osThreadNew(readPowerInput, NULL, NULL);
	osThreadNew(updateLEDs, NULL, NULL);
	osThreadNew(readDirectionInput, NULL, NULL);
	//osThreadNew(hitBall, NULL, NULL);
	//osThreadNew(drawRando, NULL, NULL);
	//osThreadNew(draw, NULL, NULL);
	osThreadNew(hitBall, NULL, NULL); 
	osThreadNew(drawBall, NULL, NULL);
	
	osKernelStart();

	while(1);
}


/*
//Yes! Globals!
actor* enemy;
actor* player;
actor* lasers[2]; //an array of actor pointers!
*/
/*
	we need to be able to stop the animation thread once one of the actors dies, so 
	we are keeping its ID global so that other threads checking on it can stop it
*/
//osThreadId_t animateID;

/*int main()
{
	//set up the main actors in the game
	initializeActors();
	//call this function to ensure that all of the internal settings of the LPC are correct
	SystemInit();*/
	
	/*
		Because of a quirk of Keil's multithreading, we need to use printf outside of a thread
		before we ever use it inside of one. On the assumption that you want to print
		debugging information this test string gets printed. You may now instrument
		the threads with printf statements if you wish
	*/
	//printf("Test String");

	/*
		Initialize the LCD screen, clear it so that it is mostly black, then
		set the "text" color to green. In reality the "text" color should be called "pixel" or "foreground"
		color, and it used to be. In a recent update from Keil the name was changed. Long story short,
		by setting the text color to green, every pixel we print is also going to be green
	*/
	/*GLCD_Init();
	GLCD_Clear(Black);
	GLCD_SetTextColor(Green); //actually sets the color of any pixels we directly write to the screen, not just text*/

	/*
		For efficiency reasons, the player character only gets re-drawn if it has moved, so
		we need to print it before the game starts or we won't see it
	*/
	/*printPlayer(player);
	//initialize the kernel so that we can create threads
	osKernelInitialize();
	
	//create a new thread for animate and store its ID so that we can shut it down at the endgame
	animateID = osThreadNew(animate,NULL,NULL);
	
	//create a new thread for reading player input
	osThreadNew(readPlayerInput,NULL, NULL);
	//create a new thread that checks for endgame
	osThreadNew(checkEndGame,NULL,NULL);
	//launch the kernel, which simultaneously starts all threads we have created
	osKernelStart();
	
	//Theoretically we will only ever entire this loop if something goes wrong in the kernel
	while(1){};
}*/
