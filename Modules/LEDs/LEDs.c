/*!
**  @addtogroup LEDs_module LEDs module documentation
**  @{
*/
/* MODULE LEDs */
/*! @file LEDs.c
 *
 *  @brief Routines to access the LEDs on the FRDM-K64F.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author Uldis Bagley and Prashant Shrestha
 *  @date 2020-04-27
 */
#include "fsl_gpio.h"
#include "fsl_port.h"

#include "LEDs.h"

const port_pin_config_t LED_PORT_PIN_CONFIG =
{
		.pullSelect = kPORT_PullDisable,
		.slewRate = kPORT_SlowSlewRate,
		.passiveFilterEnable = kPORT_PassiveFilterDisable,
		.openDrainEnable = kPORT_OpenDrainDisable,
		.driveStrength = kPORT_LowDriveStrength,
		.mux = kPORT_MuxAsGpio, //for pin multiplexing see page 248
		.lockRegister = kPORT_UnlockRegister
};

const gpio_pin_config_t LED_GPIO_CONFIG =
{
		kGPIO_DigitalOutput,
		0,
};

bool LEDs_Init(void)
{
	//clock enable for PortE and PortB
	CLOCK_EnableClock(kCLOCK_PortE);
	CLOCK_EnableClock(kCLOCK_PortB);

	//enabling ports and pins for each LED
	PORT_SetPinConfig(PORTB,22, &LED_PORT_PIN_CONFIG);
	PORT_SetPinConfig(PORTB,21, &LED_PORT_PIN_CONFIG);
	PORT_SetPinConfig(PORTE,26, &LED_PORT_PIN_CONFIG);

	GPIO_PinInit(GPIOB, 22, &LED_GPIO_CONFIG);
	GPIO_PinInit(GPIOB, 21, &LED_GPIO_CONFIG);
	GPIO_PinInit(GPIOE, 26, &LED_GPIO_CONFIG);

	LEDs_Off(LED_RED);
	LEDs_Off(LED_BLUE);
	LEDs_Off(LED_GREEN);

	return true;
}


void LEDs_On(const LED_t color)
{
	if (color == LED_RED || color == LED_BLUE)
	{
		GPIO_PortClear(GPIOB,color); // sets (Blue or Red)LED port to 0
	}
	else if (color == LED_GREEN)
	{
		GPIO_PortClear(GPIOE,color); // sets Green LED port to 0
	}
}

void LEDs_Off(const LED_t color)
{
	if (color == LED_RED || color == LED_BLUE)
	{
		GPIO_PortSet(GPIOB,color); // sets (Blue or Red)LED port to 1
	}
	else if (color == LED_GREEN)
	{
		GPIO_PortSet(GPIOE,color);
	}
}


void LEDs_Toggle(const LED_t color)
{
	if (color == LED_RED || color == LED_BLUE)
	{
		GPIO_PortToggle(GPIOB,color);
	}
	else if (color == LED_GREEN)
	{
		GPIO_PortToggle(GPIOE,color);
	}
}

/* END LEDs */
/*!
** @}
*/
