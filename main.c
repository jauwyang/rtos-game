#include "gameLogic.h"

osMutexId_t ballMutex;
osMutexId_t scoreMutex;

osThreadId_t hitBallID;
osThreadId_t writeScoreID;

int main()
{
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

	// Create the mutexes
	ballMutex = osMutexNew(NULL);
	scoreMutex = osMutexNew(NULL);
	
	// Set up the Actors (Hole and Ball)
	setupGame();

	osKernelInitialize();
	
	// Create the threads
	osThreadNew(readPowerInput, NULL, NULL);
	osThreadNew(updateLEDs, NULL, NULL);
	osThreadNew(readDirectionInput, NULL, NULL);
	hitBallID = osThreadNew(hitBall, NULL, NULL); 
	osThreadNew(checkEndGame, NULL, NULL);
	osThreadNew(teleportBall, NULL, NULL);
	osThreadNew(writeGolfScore, NULL, NULL);
	
	osKernelStart();

	while(1);
}
