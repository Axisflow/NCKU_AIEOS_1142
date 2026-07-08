#include "dht11.h"
#include <stdio.h>

#define DHT11_CACHE_TIME_MS 1000U

static void DHT11_DelayInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void DHT11_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);

    while ((DWT->CYCCNT - start) < ticks)
    {
    }
}

static void DHT11_SetOutputOpenDrain(DHT11_HandleTypeDef *hdht)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = hdht->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(hdht->port, &GPIO_InitStruct);
}

static void DHT11_SetInput(DHT11_HandleTypeDef *hdht)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = hdht->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(hdht->port, &GPIO_InitStruct);
}

static DHT11_Status DHT11_WaitForLevel(DHT11_HandleTypeDef *hdht, GPIO_PinState level, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_ticks = timeout_us * (HAL_RCC_GetHCLKFreq() / 1000000U);

    while (HAL_GPIO_ReadPin(hdht->port, hdht->pin) != level)
    {
        if ((DWT->CYCCNT - start) > timeout_ticks)
        {
            return DHT11_ERR_TIMEOUT;
        }
    }

    return DHT11_OK;
}

void DHT11_Init(DHT11_HandleTypeDef *hdht, GPIO_TypeDef *port, uint16_t pin)
{
    if (hdht == NULL)
    {
        return;
    }

    DHT11_DelayInit();

    hdht->port = port;
    hdht->pin = pin;
    hdht->has_cache = 0;
    hdht->last_read_ms = 0;
    hdht->temperature_c = 0;
    hdht->humidity_percent = 0;
    hdht->last_status = DHT11_ERR_TIMEOUT;

    DHT11_SetInput(hdht);
}

DHT11_Status DHT11_ReadRaw(DHT11_HandleTypeDef *hdht, int *temperature_c, int *humidity_percent)
{
    uint8_t data[5] = {0};
    DHT11_Status status = DHT11_OK;
    uint32_t primask;

    if (hdht == NULL || temperature_c == NULL || humidity_percent == NULL)
    {
        return DHT11_ERR_ARG;
    }

    if (HAL_GetTick() < 1000U)
    {
        HAL_Delay(1000U - HAL_GetTick());
    }

    DHT11_SetOutputOpenDrain(hdht);
    HAL_GPIO_WritePin(hdht->port, hdht->pin, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(hdht->port, hdht->pin, GPIO_PIN_SET);
    DHT11_DelayUs(30);

    DHT11_SetInput(hdht);

    primask = __get_PRIMASK();
    __disable_irq();

    status = DHT11_WaitForLevel(hdht, GPIO_PIN_RESET, 100);
    if (status != DHT11_OK) goto done;

    status = DHT11_WaitForLevel(hdht, GPIO_PIN_SET, 100);
    if (status != DHT11_OK) goto done;

    status = DHT11_WaitForLevel(hdht, GPIO_PIN_RESET, 100);
    if (status != DHT11_OK) goto done;

    uint32_t min_high_us = 999;
    uint32_t max_high_us = 0;
    int ones_count = 0;
    
    for (int i = 0; i < 40; i++)
    {
        uint32_t high_start;
        uint32_t high_ticks;
        uint32_t high_us;

        status = DHT11_WaitForLevel(hdht, GPIO_PIN_SET, 100);
        if (status != DHT11_OK) goto done;

        high_start = DWT->CYCCNT;

        status = DHT11_WaitForLevel(hdht, GPIO_PIN_RESET, 120);
        if (status != DHT11_OK) goto done;

        high_ticks = DWT->CYCCNT - high_start;
        high_us = high_ticks / (HAL_RCC_GetHCLKFreq() / 1000000U);

        if (high_us < min_high_us)
        {
            min_high_us = high_us;
        }

        if (high_us > max_high_us)
        {
            max_high_us = high_us;
        }

        data[i / 8] <<= 1;

        if (high_us > 40)
        {
            data[i / 8] |= 1;
            ones_count++;
        }
    }

done:
    if (!primask)
    {
        __enable_irq();
    }

    DHT11_SetInput(hdht);

    if (status != DHT11_OK)
    {
        hdht->last_status = status;
        return status;
    }

    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);

    if (checksum != data[4])
    {
        printf("DHT11 checksum failed: %u %u %u %u checksum=%u expected=%u\r\n",
               data[0], data[1], data[2], data[3], data[4], checksum);

        hdht->last_status = DHT11_ERR_CHECKSUM;
        return DHT11_ERR_CHECKSUM;
    }

    if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0 && data[4] == 0)
    {
        printf("DHT11 invalid all-zero data\r\n");
        hdht->last_status = DHT11_ERR_TIMEOUT;
        return DHT11_ERR_TIMEOUT;
    }

    *humidity_percent = data[0];
    *temperature_c = data[2];

    hdht->humidity_percent = *humidity_percent;
    hdht->temperature_c = *temperature_c;
    hdht->last_read_ms = HAL_GetTick();
    hdht->has_cache = 1;
    hdht->last_status = DHT11_OK;

    return DHT11_OK;
}

DHT11_Status DHT11_ReadCached(DHT11_HandleTypeDef *hdht, int *temperature_c, int *humidity_percent)
{
    if (hdht == NULL || temperature_c == NULL || humidity_percent == NULL)
    {
        return DHT11_ERR_ARG;
    }

    if (hdht->has_cache &&
        (HAL_GetTick() - hdht->last_read_ms) < DHT11_CACHE_TIME_MS)
    {
        *temperature_c = hdht->temperature_c;
        *humidity_percent = hdht->humidity_percent;
        return DHT11_OK;
    }

    return DHT11_ReadRaw(hdht, temperature_c, humidity_percent);
}

const char *DHT11_StatusString(DHT11_Status status)
{
    switch (status)
    {
        case DHT11_OK:
            return "DHT11_OK";
        case DHT11_ERR_ARG:
            return "DHT11_ERR_ARG";
        case DHT11_ERR_TIMEOUT:
            return "DHT11_ERR_TIMEOUT";
        case DHT11_ERR_CHECKSUM:
            return "DHT11_ERR_CHECKSUM";
        default:
            return "DHT11_ERR_UNKNOWN";
    }
}

