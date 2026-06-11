#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"
#include <stdint.h>

typedef enum
{
    DHT11_OK = 0,
    DHT11_ERR_ARG = -1,
    DHT11_ERR_TIMEOUT = -2,
    DHT11_ERR_CHECKSUM = -3
} DHT11_Status;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;

    uint8_t has_cache;
    uint32_t last_read_ms;

    int temperature_c;
    int humidity_percent;

    DHT11_Status last_status;
} DHT11_HandleTypeDef;

void DHT11_Init(DHT11_HandleTypeDef *hdht, GPIO_TypeDef *port, uint16_t pin);

DHT11_Status DHT11_ReadRaw(DHT11_HandleTypeDef *hdht, int *temperature_c, int *humidity_percent);

DHT11_Status DHT11_ReadCached(DHT11_HandleTypeDef *hdht, int *temperature_c, int *humidity_percent);

const char *DHT11_StatusString(DHT11_Status status);

#endif /* INC_DHT11_H_ */
