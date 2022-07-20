#include "gameLogic.h"

osMutexId_t powerMutex;
osMutexId_t playerDirectionMutex;
osMutexId_t ballMutex;
osMutexId_t scoreMutex;
osMutexId_t isHitMutex;
osMutexId_t arrowBitMapMutex;

osSemaphoreId_t canDrawSem;

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


	// Init the semaphore
	canDrawSem = osSemaphoreNew(1, 0 , NULL);
	
	// Create the mutexes
	powerMutex = osMutexNew(NULL);
	playerDirectionMutex = osMutexNew(NULL);
	ballMutex = osMutexNew(NULL);
	scoreMutex = osMutexNew(NULL);
	isHitMutex = osMutexNew(NULL);
	arrowBitMapMutex = osMutexNew(NULL);
	
	// Set up the Actors (Hole and Ball)
	setupGame();

	osKernelInitialize();
	
	// Create the threads
	osThreadNew(readPowerInput, NULL, NULL);
	osThreadNew(updateLEDs, NULL, NULL);
	osThreadNew(readDirectionInput, NULL, NULL);
	//osThreadNew(hitBall, NULL, NULL);
	//osThreadNew(drawRando, NULL, NULL);
	//osThreadNew(draw, NULL, NULL);
	osThreadNew(hitBall, NULL, NULL); 
	//osThreadNew(createArrowBitMap, NULL, NULL);
	//osThreadNew(drawArrow, NULL, NULL);
	//osThreadNew(drawMovingBall, NULL, NULL);
	//osThreadNew(createArrowBitMap, NULL, NULL);
	
	osKernelStart();

	while(1);
}
