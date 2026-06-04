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
		snprintf((char*)fs->mount_point, 32, "/dev/%s", LED_Configs[i].name);
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

static int DHT22_Read_Raw(uint8_t *data)
{
	uint32_t timeout_ticks = SystemCoreClock / 1000000; // 1us對應的 CPU 週期數

	// 1. 發送啟動訊號
	DHT22_SetPinOutput();
	HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_RESET);
	delay_us(18000); // 拉低至少 18ms
	HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
	delay_us(30);    // 拉高 30us

	// 2. 切換為輸入模式，等待回應
	DHT22_SetPinInput();

	// 等待 DHT22 拉低總線
	uint32_t start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -1;
	}

	// 等待 DHT22 拉高總線
	start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_RESET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -2;
	}

	// 等待回應的高電平結束（開始傳送第一個 bit 的低電平）
	start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -3;
	}

	// 3. 讀取 40-bit 資料
	for (int i = 0; i < 40; ++i)
	{
		// 等待低電平結束變為高電平
		start_cycles = DWT->CYCCNT;
		while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_RESET)
		{
			if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -4;
		}

		// 測量高電平持續時間
		start_cycles = DWT->CYCCNT;
		while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
		{
			if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -5;
		}

		uint32_t high_duration = (DWT->CYCCNT - start_cycles) / timeout_ticks;

		data[i / 8] <<= 1;
		if (high_duration > 40) // 高電平大於 40us 代表 bit 1
		{
			data[i / 8] |= 1;
		}
	}

	// 4. 校驗碼檢查 (Checksum)
	uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
	if (checksum != data[4])
	{
		return -6;
	}

	return 0;
}

static float cached_temp = 0.0f;
static float cached_hum = 0.0f;
static uint32_t last_read_time = 0;

static int DHT22_Read_Data(float *temp, float *hum)
{
	uint32_t current_time = HAL_GetTick();

	// 每 2 秒才允許重新讀取一次硬體，防止讀取過於頻繁
	if (last_read_time == 0 || (current_time - last_read_time) >= 2000)
	{
		uint8_t data[5] = {0};
		int status = DHT22_Read_Raw(data);
		if (status == 0)
		{
			float h = ((uint16_t)data[0] << 8 | data[1]) / 10.0f;
			float t = (((uint16_t)(data[2] & 0x7F) << 8) | data[3]) / 10.0f;
			if (data[2] & 0x80)
			{
				t = -t;
			}
			cached_temp = t;
			cached_hum = h;
			last_read_time = current_time;
		}
		else
		{
			return status;
		}
	}

	*temp = cached_temp;
	*hum = cached_hum;
	return 0;
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



