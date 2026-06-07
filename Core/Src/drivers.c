#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"
#include "drivers.h"
#include "vfs.h"

#define LED_COUNT 4

#define DHT22_GPIO_PORT         GPIOB
#define DHT22_GPIO_PIN          GPIO_PIN_5
#define DHT22_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/*************/ 
/*    LED    */
/*************/ 

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

// implement the read function in the struct file_operations for the LED driver
__vf_ssize_t LED_read(struct file *file, char *buf, size_t count) {
	const char* led_name = file->fs->mount_point;

	for(uint8_t i=0;i<LED_COUNT;++i) {
		if(strcmp(LED_Configs[i].name, led_name) == 0) {
			GPIO_TypeDef* GPIO_Port = LED_GetGPIOPort(LED_Configs[i].GPIO_Port);
			uint16_t GPIO_Pin = LED_GetGPIOPin(LED_Configs[i].GPIO_Pin);
			buf[0] = HAL_GPIO_ReadPin(GPIO_Port, GPIO_Pin) == GPIO_PIN_SET ? '1' : '0';
			return 1; // Return the number of bytes read
		}
	}
	return VF_ERROR;
}

// implement the write function in the struct file_operations for the LED driver
__vf_ssize_t LED_write(struct file *file, const char *buf, size_t count) {
    const char* led_name = file->fs->mount_point; // The mount point is the LED name
	
	for(uint8_t i=0;i<LED_COUNT;++i) {
		if(strcmp(LED_Configs[i].name, led_name) == 0) {
			GPIO_TypeDef* GPIO_Port = LED_GetGPIOPort(LED_Configs[i].GPIO_Port);
			uint16_t GPIO_Pin = LED_GetGPIOPin(LED_Configs[i].GPIO_Pin);
			
			if(count > 0 && buf[0] == '1') {
				HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET); // Turn on the LED
			} else if(count > 0 && buf[0] == '0') {
				HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET); // Turn off the LED
			} else {
				return VF_INVALID; // Invalid command
			}
			return 1; // Return the number of bytes written
		}
	}
	return VF_ERROR;
}

// Define the file operations for the LED driver
struct file_operations LED_fops={
	.read = LED_read,
	.write = LED_write
};

void initialize_LED(void)
{
	/*Hardware initialization*/
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
	/*************************/

	/*VFS initialization*/

	for (uint8_t i = 0; i < LED_COUNT; ++i)
	{
		struct file_system *fs = pvPortMalloc(sizeof(struct file_system));
		strcpy(fs->name, LED_Configs[i].name);
		fs->mount_point = pvPortMalloc(32);
		snprintf((char*)fs->mount_point, 32, "dev/%s", LED_Configs[i].name);
		fs->fops = &LED_fops;
		fs->nops = NULL;
		vfs_mount(fs);
	}
	/********************/

}





/************/ 
/*  DHT 11  */
/************/ 
static void DHT22_SetPinOutput(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT22_SetPinInput(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}

void initialize_DHT22(void)
{
	DHT22_GPIO_CLK_ENABLE();
	DHT22_SetPinInput();

	// Enable DWT Cycle Counter
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us)
{
	uint32_t start = DWT->CYCCNT;
	uint32_t ticks = us * (SystemCoreClock / 1000000);
	while ((DWT->CYCCNT - start) < ticks);
}



