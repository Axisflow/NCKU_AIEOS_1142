#include <string.h>
#include <stdio.h>
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "drivers.h"
#include "vfs.h"
#include "dht11.h"

#define LED_COUNT 4

#define DHT22_GPIO_PORT         GPIOB
#define DHT22_GPIO_PIN          GPIO_PIN_5
#define DHT22_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define bodyTemp_Count 1
#define AD8232_Count 1


/**************/ 
/*    I2C1    */
/**************/ 

I2C_HandleTypeDef hi2c1;
ADC_HandleTypeDef hadc1;

void MX_I2C1_Init(void)
{
    /* I2C1 clock */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C peripheral config */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000; // 100 kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
    	Error_Handler();
    }
}

void MX_ADC1_Init(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	__HAL_RCC_ADC1_CLK_ENABLE();

	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;  //ADC 的工作時脈
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;  //電壓解析度為 12 位元
	hadc1.Init.ScanConvMode = DISABLE;  //單一感測器就設定為 DISABLE
	hadc1.Init.ContinuousConvMode = DISABLE;  //感測器是否持續量測，還是只量測一次就停止
	hadc1.Init.DiscontinuousConvMode = DISABLE;  //For 多個感測器的情況，是否在每次量測後暫停一下再繼續量測下一個感測器
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;  //不使用外部觸發事件。
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;  //由軟體觸發。
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;  //ADC結果要放在暫存器的靠左還是靠右
	hadc1.Init.NbrOfConversion = 1;  //感測器數量
	hadc1.Init.DMAContinuousRequests = DISABLE;  //是否使用 DMA 來傳輸 ADC 資料到記憶體
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV; //每完成一次量測就產生一次中斷
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_1;  //要量測的感測器連接到哪個 ADC channel。(PA1對應到 ADC_CHANNEL_1)
	sConfig.Rank = 1;  //在多個感測器的情況下，這個數字代表量測順序。單一感測器就設定為 1。
	sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

/*************/ 
/*    LED    */
/*************/ 

const LED_Config_t LED_Configs[LED_COUNT] =
{
	{
		.name = "GreenLED",
		.Led_Config = {
			.Port = "D",
			.Pin = 12,
			.Mode = "OUTPUT_PP",
			.Pull = "NOPULL",
			.Speed = "FREQ_LOW"
		}
	},
	{
		.name = "OrangeLED",
		.Led_Config = {
			.Port = "D",
			.Pin = 13,
			.Mode = "OUTPUT_PP",
			.Pull = "NOPULL",
			.Speed = "FREQ_LOW"
		}
	},
	{
		.name = "RedLED",
		.Led_Config = {
			.Port = "D",
			.Pin = 14,
			.Mode = "OUTPUT_PP",
			.Pull = "NOPULL",
			.Speed = "FREQ_LOW"
		}
	},
	{
		.name = "BlueLED",
		.Led_Config = {
			.Port = "D",
			.Pin = 15,
			.Mode = "OUTPUT_PP",
			.Pull = "NOPULL",
			.Speed = "FREQ_LOW"
		}
	}
};

static GPIO_TypeDef *GetGPIOPort(const char *portName)
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

	return GPIOD;
}

void EnableGPIOClock(const char *portName)
{
	if (strcmp(portName, "A") == 0)
		{
			__HAL_RCC_GPIOA_CLK_ENABLE();
		}
		else if (strcmp(portName, "B") == 0)
		{
			__HAL_RCC_GPIOB_CLK_ENABLE();
		}
		else if (strcmp(portName, "C") == 0)
		{
			__HAL_RCC_GPIOC_CLK_ENABLE();
		}
		else if (strcmp(portName, "D") == 0)
		{
			__HAL_RCC_GPIOD_CLK_ENABLE();
		}
		else if (strcmp(portName, "E") == 0)
		{
			__HAL_RCC_GPIOE_CLK_ENABLE();
		}
		else if (strcmp(portName, "H") == 0)
		{
			__HAL_RCC_GPIOH_CLK_ENABLE();
		}
}


static uint16_t GetGPIOPin(uint16_t pinName)
{
	return (1U << pinName); // GPIO_PIN_0 = 0x0001, GPIO_PIN_1 = 0x0002, ..., GPIO_PIN_15 = 0x8000
}

static uint32_t GetMode(const char *modeName)
{
	if (strcmp(modeName, "OUTPUT_PP") == 0)
	{
		return GPIO_MODE_OUTPUT_PP;
	}

	if (strcmp(modeName, "OUTPUT_OD") == 0)
	{
		return GPIO_MODE_OUTPUT_OD;
	}
	if (strcmp(modeName, "INPUT") == 0)
	{
		return GPIO_MODE_INPUT;
	}
	if(strcmp(modeName, "AF_PP") == 0)
	{
		return GPIO_MODE_AF_PP;
	}
	if(strcmp(modeName, "AF_OD") == 0)
	{
		return GPIO_MODE_AF_OD;
	}
	if(strcmp(modeName, "ANALOG") == 0)
	{
		return GPIO_MODE_ANALOG;
	}

	return GPIO_MODE_OUTPUT_PP;
}

static uint32_t GetPull(const char *pullName)
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

static uint32_t GetSpeed(const char *speedName)
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

static const char *GetNameFromMountPoint(const char *mount_point)
{
    const char *slash = strrchr(mount_point, '/');

    if (slash != NULL) {
        return slash + 1;
    }

    return mount_point;
}

// implement the read function in the struct file_operations for the LED driver
__vf_ssize_t LED_read(struct file *file, char *buf, size_t count) 
{
	const char* led_name = file->fs->mount_point + 4; // 跳過 "dev/" 前綴，取得 LED 名稱

	for(uint8_t i=0;i<LED_COUNT;++i) {
		if(strcmp(LED_Configs[i].name, led_name) == 0) {
			GPIO_TypeDef* GPIO_Port = GetGPIOPort(LED_Configs[i].Led_Config.Port);
			uint16_t GPIO_Pin = GetGPIOPin(LED_Configs[i].Led_Config.Pin);
			buf[0] = HAL_GPIO_ReadPin(GPIO_Port, GPIO_Pin) == GPIO_PIN_SET ? '1' : '0';
			return 1; // Return the number of bytes read
		}
	}
	return VF_ERROR;
}

// implement the write function in the struct file_operations for the LED driver
__vf_ssize_t LED_write(struct file *file, const char *buf, size_t count) 
{
    const char* led_name = file->fs->mount_point + 4; // 跳過 "dev/" 前綴，取得 LED 名稱
	

	for(uint8_t i=0;i<LED_COUNT;++i) {
		if(strcmp(LED_Configs[i].name, led_name) == 0) {
			GPIO_TypeDef* GPIO_Port = GetGPIOPort(LED_Configs[i].Led_Config.Port);
			uint16_t GPIO_Pin = GetGPIOPin(LED_Configs[i].Led_Config.Pin);
			
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
struct file_operations LED_fops=
{
	.read = LED_read,
	.write = LED_write
};

void initialize_LED(void)
{
	/*Hardware initialization*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	for (uint8_t i = 0; i < LED_COUNT; ++i)
	{
		EnableGPIOClock(LED_Configs[i].Led_Config.Port);
		GPIO_InitStruct.Pin = GetGPIOPin(LED_Configs[i].Led_Config.Pin);
		GPIO_InitStruct.Mode = GetMode(LED_Configs[i].Led_Config.Mode);
		GPIO_InitStruct.Pull = GetPull(LED_Configs[i].Led_Config.Pull);
		GPIO_InitStruct.Speed = GetSpeed(LED_Configs[i].Led_Config.Speed);
		HAL_GPIO_Init(GetGPIOPort(LED_Configs[i].Led_Config.Port), &GPIO_InitStruct);
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
static DHT11_HandleTypeDef g_dht11;

static __vf_ssize_t DHT11_read(struct file *file, char *buf, size_t count)
{
    const char *dev_name = GetNameFromMountPoint(file->fs->mount_point);

    int temp = 0;
    int hum = 0;

    DHT11_Status status = DHT11_ReadCached(&g_dht11, &temp, &hum);

    if (status != DHT11_OK)
    {
        printf("DHT11 read failed, status=%d (%s)\r\n",
               status, DHT11_StatusString(status));
        return VF_ERROR;
    }

    if (strcmp(dev_name, "temp1") == 0)
    {
        return snprintf(buf, count, "%d\r\n", temp);
    }

    if (strcmp(dev_name, "hum1") == 0)
    {
        return snprintf(buf, count, "%d\r\n", hum);
    }

    return VF_NOT_FOUND;
}

struct file_operations DHT11_fops =
{
    .read = DHT11_read,
    .write = NULL
};

void initialize_DHT11(void)
{
    DHT11_Init(&g_dht11, DHT11_GPIO_Port, DHT11_Pin);

    const char *dev_names[] =
    {
        "temp1",
        "hum1"
    };

    for (uint8_t i = 0; i < 2; ++i)
    {
        struct file_system *fs = pvPortMalloc(sizeof(struct file_system));

        if (fs == NULL)
        {
            printf("DHT11 fs malloc failed\r\n");
            return;
        }

        memset(fs, 0, sizeof(struct file_system));

        strcpy(fs->name, dev_names[i]);

        fs->mount_point = pvPortMalloc(32);

        if (fs->mount_point == NULL)
        {
            printf("DHT11 mount_point malloc failed\r\n");
            return;
        }

        snprintf((char *)fs->mount_point, 32, "dev/%s", dev_names[i]);

        fs->fops = &DHT11_fops;
        fs->nops = NULL;

        vfs_mount(fs);
    }

    printf("DHT11 mounted: dev/temp0, dev/hum0\r\n");
}

/************/ 
/*  DHT 22  */
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

	DHT22_SetPinOutput();
	HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_RESET);
	delay_us(18000); // 拉低至少 18ms
	HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
	delay_us(30);    // 拉高 30us

	DHT22_SetPinInput();

	uint32_t start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -1;
	}

	start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_RESET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -2;
	}

	start_cycles = DWT->CYCCNT;
	while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
	{
		if ((DWT->CYCCNT - start_cycles) > 100 * timeout_ticks) return -3;
	}

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

__vf_ssize_t DHT22_temp_read(struct file *file, char *buf, size_t count)
{
	float temp = 0.0f;
	float hum = 0.0f;
	int status = DHT22_Read_Data(&temp, &hum);
	if (status != 0)
	{
		return VF_ERROR; 
	}

	int temp_int = (int)temp;
	int temp_dec = (int)((temp - temp_int) * 10);
	if (temp_dec < 0)
	{
		temp_dec = -temp_dec;
	}

	int len;
	if (temp < 0.0f && temp_int == 0)
	{
		len = snprintf(buf, count, "-0.%d\n", temp_dec);
	}
	else
	{
		len = snprintf(buf, count, "%d.%d\n", temp_int, temp_dec);
	}

	return len; 
}

__vf_ssize_t DHT22_hum_read(struct file *file, char *buf, size_t count)
{
	float temp = 0.0f;
	float hum = 0.0f;
	int status = DHT22_Read_Data(&temp, &hum);
	if (status != 0)
	{
		return VF_ERROR; 
	}

	int hum_int = (int)hum;
	int hum_dec = (int)((hum - hum_int) * 10);
	if (hum_dec < 0)
	{
		hum_dec = -hum_dec;
	}

	int len = snprintf(buf, count, "%d.%d\n", hum_int, hum_dec);
	return len; 
}

struct file_operations DHT22_temp_fops = {
	.read = DHT22_temp_read
};

struct file_operations DHT22_hum_fops = {
	.read = DHT22_hum_read
};

void initialize_DHT22(void)
{
	DHT22_GPIO_CLK_ENABLE();
	DHT22_SetPinInput();

	// Enable DWT Cycle Counter
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/* VFS initialization for Temperature Device (dev/temp0) */
	struct file_system *fs_temp = pvPortMalloc(sizeof(struct file_system));
	if (fs_temp != NULL)
	{
		strcpy(fs_temp->name, "temp0");
		fs_temp->mount_point = pvPortMalloc(32);
		if (fs_temp->mount_point != NULL)
		{
			strcpy((char*)fs_temp->mount_point, "dev/temp0");
			fs_temp->fops = &DHT22_temp_fops;
			fs_temp->nops = NULL;
			vfs_mount(fs_temp);
		}
	}

	/* VFS initialization for Humidity Device (dev/hum0) */
	struct file_system *fs_hum = pvPortMalloc(sizeof(struct file_system));
	if (fs_hum != NULL)
	{
		strcpy(fs_hum->name, "hum0");
		fs_hum->mount_point = pvPortMalloc(32);
		if (fs_hum->mount_point != NULL)
		{
			strcpy((char*)fs_hum->mount_point, "dev/hum0");
			fs_hum->fops = &DHT22_hum_fops;
			fs_hum->nops = NULL;
			vfs_mount(fs_hum);
		}
	}
}

/**********************/ 
/*  Body Temperature  */
/**********************/ 

bodyTemp_Config_t bodyTemp_Config[bodyTemp_Count] =
{
	{
    .DeviceName     = "bodyTemp1",
    .I2C_Address    = 0x48,
    .I2Cx           = &hi2c1,  //目前只能使用 I2C1，故不可修正。
	.SCL_Config = {
		.Port = "B",
		.Pin = 6,
		.Mode = "AF_OD",
		.Pull = "PULLUP",
		.Speed = "FREQ_VERY_HIGH"
	},
	.SDA_Config = {
		.Port = "B",
		.Pin = 7,
		.Mode = "AF_OD",
		.Pull = "PULLUP",
		.Speed = "FREQ_VERY_HIGH"
	},
	.ALERT_Config = {
		.Port = "B",
		.Pin = 8,
		.Mode = "INPUT",
		.Pull = "NOPULL",
		.Speed = "FREQ_VERY_HIGH"
	},
    .ShutdownMode   = 0,
    .InterruptMode  = 0,
    .OSPolarity     = 0,
    .FaultQueue     = 0,
    .TimeoutEnable  = 1,
    .THYST          = 37.5f,
    .TOS            = 38.0f
	}
};

static void I2C1_ScanAndReport(I2C_HandleTypeDef *hi2c)
{
	uint8_t found = 0;

	for (uint8_t addr = 0x08; addr <= 0x77; ++addr)
	{
		if (HAL_I2C_IsDeviceReady(hi2c, addr << 1, 2, 10) == HAL_OK)
		{
			printf("I2C device found at 0x%02X\r\n", addr);
			found++;
		}
	}

	if (found == 0)
	{
		printf("I2C scan: no device acknowledged\r\n");
	}
}

// implement the read function in the struct file_operations for the body Temperature driver
__vf_ssize_t bodyTemp_read(struct file *file, char *buf, size_t count)
{
	const char *bodyTemp_name = file->fs->mount_point + 4; // 跳過 "dev/" 前綴，取得 body Temperature 設備名稱

    for(uint8_t i = 0; i < bodyTemp_Count; i++)
    {
        if(strcmp(bodyTemp_Config[i].DeviceName, bodyTemp_name) == 0)
        {
            uint8_t raw[2] = {0};
            int16_t temp_raw;
            float temperature;

            /* Read 2 bytes from temperature register (0x00) */
			HAL_StatusTypeDef status;
			status = HAL_I2C_Mem_Read(
			    bodyTemp_Config[i].I2Cx,
			    bodyTemp_Config[i].I2C_Address << 1,
			    0x00,
			    I2C_MEMADD_SIZE_8BIT,
			    raw,
			    2,
			    1000
			);
			if(status != HAL_OK)
			{
				uint32_t err = HAL_I2C_GetError(bodyTemp_Config[i].I2Cx);
				printf("MAX30205 read failed addr=0x%02X status=%d err=0x%08lX\r\n",
						bodyTemp_Config[i].I2C_Address,
						(int)status,
						(unsigned long)err);
				return VF_ERROR;
			} else {
				printf("status=%d raw=%02X %02X\r\n",
			       status, raw[0], raw[1]);
			}

            /* Convert raw data */
            temp_raw = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);  //若溫度為負數(用二補數表示)，需要轉型成int16_t，這樣解讀值時就會自動做二的補數轉回可讀負數
            temperature = temp_raw * 0.00390625f; // 1/256 resolution

            /* Return as string */
			int len = snprintf(buf, count, "%d\n", (int)temperature);

            return len;
        }
    }

    return VF_ERROR;
}

// Define the file operations for the body Temperature driver
struct file_operations bodyTemp_fops={
	.read = bodyTemp_read,
};

void initialize_bodyTemp(void)
{
    /*Hardware initialization*/
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    for(uint8_t i = 0; i < bodyTemp_Count; i++)
    {
        /*--------------------------------------------------
         * GPIO Clock
         *-------------------------------------------------*/
        EnableGPIOClock(bodyTemp_Config[i].SCL_Config.Port);
		EnableGPIOClock(bodyTemp_Config[i].SDA_Config.Port);
		if (bodyTemp_Config[i].ALERT_Config.Port != NULL)
		{	
			EnableGPIOClock(bodyTemp_Config[i].ALERT_Config.Port);
		}

        /*--------------------------------------------------
         * SCL
         *-------------------------------------------------*/
        GPIO_InitStruct.Pin = GetGPIOPin(bodyTemp_Config[i].SCL_Config.Pin);
        GPIO_InitStruct.Mode = GetMode(bodyTemp_Config[i].SCL_Config.Mode); 
        GPIO_InitStruct.Pull = GetPull(bodyTemp_Config[i].SCL_Config.Pull);
        GPIO_InitStruct.Speed = GetSpeed(bodyTemp_Config[i].SCL_Config.Speed);
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; //把GPIO腳接到I2C1模組
		

        HAL_GPIO_Init(GetGPIOPort(bodyTemp_Config[i].SCL_Config.Port), &GPIO_InitStruct);

		/*--------------------------------------------------
		 * SDA
		 *-------------------------------------------------*/
		GPIO_InitStruct.Pin = GetGPIOPin(bodyTemp_Config[i].SDA_Config.Pin);
		GPIO_InitStruct.Mode = GetMode(bodyTemp_Config[i].SDA_Config.Mode);
		GPIO_InitStruct.Pull = GetPull(bodyTemp_Config[i].SDA_Config.Pull);
		GPIO_InitStruct.Speed = GetSpeed(bodyTemp_Config[i].SDA_Config.Speed);
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; //把GPIO腳接到I2C1模組
		

		HAL_GPIO_Init(GetGPIOPort(bodyTemp_Config[i].SDA_Config.Port), &GPIO_InitStruct);

        /*--------------------------------------------------
         * ALERT
         *-------------------------------------------------*/
        GPIO_InitStruct.Pin = GetGPIOPin(bodyTemp_Config[i].ALERT_Config.Pin);
        GPIO_InitStruct.Mode = GetMode(bodyTemp_Config[i].ALERT_Config.Mode);
        GPIO_InitStruct.Pull = GetPull(bodyTemp_Config[i].ALERT_Config.Pull);
        GPIO_InitStruct.Speed = GetSpeed(bodyTemp_Config[i].ALERT_Config.Speed);

        HAL_GPIO_Init(GetGPIOPort(bodyTemp_Config[i].ALERT_Config.Port), &GPIO_InitStruct);

        /*--------------------------------------------------
         * MAX30205 Config Register
         *-------------------------------------------------*/
        uint8_t config = 0;

        config |= (bodyTemp_Config[i].ShutdownMode);
        config |= (bodyTemp_Config[i].InterruptMode << 1);
        config |= (bodyTemp_Config[i].OSPolarity << 2);
        config |= (bodyTemp_Config[i].FaultQueue << 3);
        config |= (bodyTemp_Config[i].TimeoutEnable << 6); // D6 is Timeout, D5 is Data Format (Extended Mode)

		I2C1_ScanAndReport(bodyTemp_Config[i].I2Cx);

		HAL_StatusTypeDef ready_status = HAL_I2C_IsDeviceReady(
				bodyTemp_Config[i].I2Cx,
				bodyTemp_Config[i].I2C_Address << 1,
				3,
				100);

		if (ready_status != HAL_OK)
		{
			uint32_t err = HAL_I2C_GetError(bodyTemp_Config[i].I2Cx);
			printf("MAX30205 init not ready addr=0x%02X status=%d err=0x%08lX\r\n",
					bodyTemp_Config[i].I2C_Address,
					(int)ready_status,
					(unsigned long)err);
		}

        HAL_StatusTypeDef write_status = HAL_I2C_Mem_Write(
            bodyTemp_Config[i].I2Cx,
            bodyTemp_Config[i].I2C_Address << 1, //將原本的7-bit地址左移1位，並在最低位添加0表示為寫入操作
            0x01,  //MAX30205的Config Register地址
            I2C_MEMADD_SIZE_8BIT,  //Config Register地址為8位元，使STM32知道如何去組成傳送資料(Device Address + Register Address + Data)
            &config,
            1,   //寫入1個byte到config register
            HAL_MAX_DELAY  //等待直到寫入完成，或發生錯誤
        );

		if (write_status != HAL_OK)
		{
			uint32_t err = HAL_I2C_GetError(bodyTemp_Config[i].I2Cx);
			printf("MAX30205 config write failed addr=0x%02X status=%d err=0x%08lX\r\n",
					bodyTemp_Config[i].I2C_Address,
					(int)write_status,
					(unsigned long)err);
		}
    }
    /*************************/
    
	/*VFS initialization*/
	for (uint8_t i = 0; i < bodyTemp_Count; ++i)
	{
		struct file_system *fs = pvPortMalloc(sizeof(struct file_system));
		strcpy(fs->name, bodyTemp_Config[i].DeviceName);
		fs->mount_point = pvPortMalloc(32);
		snprintf((char*)fs->mount_point, 32, "dev/%s", bodyTemp_Config[i].DeviceName);
		fs->fops = &bodyTemp_fops;
		fs->nops = NULL;
		vfs_mount(fs);
	}
	/********************/
}

/**********************/ 
/*      AD8232        */
/**********************/ 

AD8232_Config_t AD8232_Config[AD8232_Count] =
{
	{
		.DeviceName = "ad8232",
		.ADCx = &hadc1,  //目前只能使用 ADC1，故不可修正。
		.OUT_Config = {
			.Port = "A",
			.Pin = 1,
			.Mode = "ANALOG",
			.Pull = "NOPULL",
			.Speed = "FREQ_LOW"
		},
		.LOPlus_Config = {
			.Port = "C",
			.Pin = 1,
			.Mode = "INPUT",
			.Pull = "PULLUP",
			.Speed = "FREQ_LOW"
		},
		.LOMinus_Config = {
			.Port = "C",
			.Pin = 2,
			.Mode = "INPUT",
			.Pull = "PULLUP",
			.Speed = "FREQ_LOW"
		}
	}
};

__vf_ssize_t AD8232_read(struct file *file, char *buf, size_t count)
{
    const char *device_name = file->fs->mount_point + 4;

    for (uint8_t i = 0; i < AD8232_Count; ++i)
    {
        if (strcmp(AD8232_Config[i].DeviceName, device_name) == 0)
        {
            uint32_t beat_count = 0;
            uint8_t peak_flag = 0;

            // 1. 2-second Dynamic Threshold Calibration
            uint32_t cal_start = HAL_GetTick();
            uint32_t max_val = 0;
            uint32_t min_val = 4095;
            
            while ((HAL_GetTick() - cal_start) < 2000)
            {
                uint32_t adc_value = 0;
                if (HAL_ADC_Start(AD8232_Config[i].ADCx) == HAL_OK)
                {
                    if (HAL_ADC_PollForConversion(AD8232_Config[i].ADCx, 10) == HAL_OK)
                    {
                        adc_value = HAL_ADC_GetValue(AD8232_Config[i].ADCx);
                        printf("%lu\n", (unsigned long)adc_value);
                        if (adc_value > max_val) max_val = adc_value;
                        if (adc_value < min_val) min_val = adc_value;
                    }
                    HAL_ADC_Stop(AD8232_Config[i].ADCx);
                }
                HAL_Delay(5);
            }
            
            uint32_t threshold = 3000;
            if (max_val > min_val && (max_val - min_val) > 200)
            {
                threshold = min_val + (max_val - min_val) * 7 / 10; // 70% level
            }
            
            // 2. 58-second Main Measurement Loop (Total 60 seconds)
            uint32_t main_start = HAL_GetTick();
            while ((HAL_GetTick() - main_start) < 58000)
            {
                uint32_t adc_value = 0;
                if (HAL_ADC_Start(AD8232_Config[i].ADCx) == HAL_OK)
                {
                    if (HAL_ADC_PollForConversion(AD8232_Config[i].ADCx, 10) == HAL_OK)
                    {
                        adc_value = HAL_ADC_GetValue(AD8232_Config[i].ADCx);
                        printf("%lu\n", (unsigned long)adc_value);
                        
                        // R-peak detection with hysteresis
                        if (adc_value > threshold && peak_flag == 0)
                        {
                            peak_flag = 1;
                            beat_count++;
                        }
                        if (adc_value < threshold - 200)
                        {
                            peak_flag = 0;
                        }
                    }
                    HAL_ADC_Stop(AD8232_Config[i].ADCx);
                }
                HAL_Delay(5);
            }
            
            uint32_t bpm = beat_count; // 1-minute beat count is exactly BPM
            return snprintf(buf, count, "%lu BPM\n", (unsigned long)bpm);
        }
    }

    return VF_ERROR;
}

struct file_operations AD8232_fops =
{
	.read = AD8232_read,
};

void initialize_AD8232(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	for (uint8_t i = 0; i < AD8232_Count; ++i)
	{
		EnableGPIOClock(AD8232_Config[i].OUT_Config.Port);
		EnableGPIOClock(AD8232_Config[i].LOPlus_Config.Port);
		EnableGPIOClock(AD8232_Config[i].LOMinus_Config.Port);

		GPIO_InitStruct.Pin = GetGPIOPin(AD8232_Config[i].OUT_Config.Pin);
		GPIO_InitStruct.Mode = GetMode(AD8232_Config[i].OUT_Config.Mode);
		GPIO_InitStruct.Pull = GetPull(AD8232_Config[i].OUT_Config.Pull);
		GPIO_InitStruct.Speed = GetSpeed(AD8232_Config[i].OUT_Config.Speed);
		HAL_GPIO_Init(GetGPIOPort(AD8232_Config[i].OUT_Config.Port), &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GetGPIOPin(AD8232_Config[i].LOPlus_Config.Pin);
		GPIO_InitStruct.Mode = GetMode(AD8232_Config[i].LOPlus_Config.Mode);
		GPIO_InitStruct.Pull = GetPull(AD8232_Config[i].LOPlus_Config.Pull);
		GPIO_InitStruct.Speed = GetSpeed(AD8232_Config[i].LOPlus_Config.Speed);
		HAL_GPIO_Init(GetGPIOPort(AD8232_Config[i].LOPlus_Config.Port), &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GetGPIOPin(AD8232_Config[i].LOMinus_Config.Pin);
		GPIO_InitStruct.Mode = GetMode(AD8232_Config[i].LOMinus_Config.Mode);
		GPIO_InitStruct.Pull = GetPull(AD8232_Config[i].LOMinus_Config.Pull);
		GPIO_InitStruct.Speed = GetSpeed(AD8232_Config[i].LOMinus_Config.Speed);
		HAL_GPIO_Init(GetGPIOPort(AD8232_Config[i].LOMinus_Config.Port), &GPIO_InitStruct);
	}

	for (uint8_t i = 0; i < AD8232_Count; ++i)
	{
		struct file_system *fs = pvPortMalloc(sizeof(struct file_system));
		if (fs != NULL)
		{
			strcpy(fs->name, AD8232_Config[i].DeviceName);
			fs->mount_point = pvPortMalloc(32);
			if (fs->mount_point != NULL)
			{
				snprintf((char*)fs->mount_point, 32, "dev/%s", AD8232_Config[i].DeviceName);
				fs->fops = &AD8232_fops;
				fs->nops = NULL;
				vfs_mount(fs);
			}
		}
	}
}




void delay_us(uint32_t us)
{
	uint32_t start = DWT->CYCCNT;
	uint32_t ticks = us * (SystemCoreClock / 1000000);
	while ((DWT->CYCCNT - start) < ticks);
}



