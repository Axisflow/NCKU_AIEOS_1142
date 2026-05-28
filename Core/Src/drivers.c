#include <string.h>
#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"
#include "drivers.h"

#define LED_COUNT 4

const LED_Config_t LED_Configs[LED_COUNT] =
{
	{
		.name = "GreenLED",
		.GPIO_Port = "D",
		.GPIO_Pin = "12",
		.Mode = "OUTPUT_PP",
		.OutputType = "OUTPUT_PP",
		.Pull = "NOPULL",
		.Speed = "FREQ_LOW"
	},
	{
		.name = "OrangeLED",
		.GPIO_Port = "D",
		.GPIO_Pin = "13",
		.Mode = "OUTPUT_PP",
		.OutputType = "OUTPUT_PP",
		.Pull = "NOPULL",
		.Speed = "FREQ_LOW"
	},
	{
		.name = "RedLED",
		.GPIO_Port = "D",
		.GPIO_Pin = "14",
		.Mode = "OUTPUT_PP",
		.OutputType = "OUTPUT_PP",
		.Pull = "NOPULL",
		.Speed = "FREQ_LOW"
	},
	{
		.name = "BlueLED",
		.GPIO_Port = "D",
		.GPIO_Pin = "15",
		.Mode = "OUTPUT_PP",
		.OutputType = "OUTPUT_PP",
		.Pull = "NOPULL",
		.Speed = "FREQ_LOW"
	}
};

static GPIO_TypeDef *LED_GetGPIOPort(const char *portName)
{
	if (strcmp(portName, "A") == 0)
	{
		return GPIOA;
	}

	if (strcmp(portName, "B") == 0)
	{
		return GPIOB;
	}

	if (strcmp(portName, "C") == 0)
	{
		return GPIOC;
	}

	if (strcmp(portName, "D") == 0)
	{
		return GPIOD;
	}

	if (strcmp(portName, "E") == 0)
	{
		return GPIOE;
	}

	if (strcmp(portName, "H") == 0)
	{
		return GPIOH;
	}

	return GPIOD;
}

static uint16_t LED_GetGPIOPin(const char *pinName)
{
	if (strcmp(pinName, "12") == 0)
	{
		return GPIO_PIN_12;
	}

	if (strcmp(pinName, "13") == 0)
	{
		return GPIO_PIN_13;
	}

	if (strcmp(pinName, "14") == 0)
	{
		return GPIO_PIN_14;
	}

	if (strcmp(pinName, "15") == 0)
	{
		return GPIO_PIN_15;
	}

	return GPIO_PIN_0;
}

static uint32_t LED_GetMode(const char *modeName)
{
	if (strcmp(modeName, "OUTPUT_PP") == 0)
	{
		return GPIO_MODE_OUTPUT_PP;
	}

	if (strcmp(modeName, "OUTPUT_OD") == 0)
	{
		return GPIO_MODE_OUTPUT_OD;
	}

	return GPIO_MODE_OUTPUT_PP;
}

static uint32_t LED_GetPull(const char *pullName)
{
	if (strcmp(pullName, "NOPULL") == 0)
	{
		return GPIO_NOPULL;
	}

	if (strcmp(pullName, "PULLUP") == 0)
	{
		return GPIO_PULLUP;
	}

	if (strcmp(pullName, "PULLDOWN") == 0)
	{
		return GPIO_PULLDOWN;
	}

	return GPIO_NOPULL;
}

static uint32_t LED_GetSpeed(const char *speedName)
{
	if (strcmp(speedName, "FREQ_LOW") == 0)
	{
		return GPIO_SPEED_FREQ_LOW;
	}

	if (strcmp(speedName, "FREQ_MEDIUM") == 0)
	{
		return GPIO_SPEED_FREQ_MEDIUM;
	}

	if (strcmp(speedName, "FREQ_HIGH") == 0)
	{
		return GPIO_SPEED_FREQ_HIGH;
	}

	if (strcmp(speedName, "FREQ_VERY_HIGH") == 0)
	{
		return GPIO_SPEED_FREQ_VERY_HIGH;
	}

	return GPIO_SPEED_FREQ_LOW;
}

void initialize_LED(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	for (uint8_t i = 0; i < LED_COUNT; ++i)
	{
		if (strcmp(LED_Configs[i].GPIO_Port, "A") == 0)
		{
			__HAL_RCC_GPIOA_CLK_ENABLE();
		}
		else if (strcmp(LED_Configs[i].GPIO_Port, "B") == 0)
		{
			__HAL_RCC_GPIOB_CLK_ENABLE();
		}
		else if (strcmp(LED_Configs[i].GPIO_Port, "C") == 0)
		{
			__HAL_RCC_GPIOC_CLK_ENABLE();
		}
		else if (strcmp(LED_Configs[i].GPIO_Port, "D") == 0)
		{
			__HAL_RCC_GPIOD_CLK_ENABLE();
		}

		GPIO_InitStruct.Pin = LED_GetGPIOPin(LED_Configs[i].GPIO_Pin);
		GPIO_InitStruct.Mode = LED_GetMode(LED_Configs[i].Mode);
		GPIO_InitStruct.Pull = LED_GetPull(LED_Configs[i].Pull);
		GPIO_InitStruct.Speed = LED_GetSpeed(LED_Configs[i].Speed);
		HAL_GPIO_Init(LED_GetGPIOPort(LED_Configs[i].GPIO_Port), &GPIO_InitStruct);
	}
}

