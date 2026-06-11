#ifndef __DRIVERS_H
#define __DRIVERS_H

#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#ifndef HAL_ADC_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;

typedef struct
{
    const char *Port;
    uint16_t Pin;
    const char *Mode;
    const char *Pull;
    const char *Speed;

} GPIO_Config_t;


typedef struct
{
	const char *name;
	GPIO_Config_t Led_Config;
} LED_Config_t;

typedef struct
{
    const char *DeviceName;
    ADC_HandleTypeDef *ADCx;
    GPIO_Config_t OUT_Config;  // AD8232輸出腳位配置
    GPIO_Config_t LOPlus_Config;  // 正輸入端電極脫落偵測腳位
    GPIO_Config_t LOMinus_Config;  // 負輸入端電極脫落偵測腳位
} AD8232_Config_t;

typedef struct
{
    /* Device */
    const char *DeviceName;
    uint8_t I2C_Address;

    /* I2C */
    I2C_HandleTypeDef *I2Cx;
	GPIO_Config_t SCL_Config;
	GPIO_Config_t SDA_Config;

    /* ALERT Pin (Optional) */
    GPIO_Config_t ALERT_Config; // if ALERT_Config.Port is NULL, it means this device does not use an ALERT.
    uint16_t ALERT_Pin;  

    /* MAX30205 Configuration Register */
    bool ShutdownMode;   //為0時晶片停止持續量測，可降低耗電。
    bool InterruptMode;  //為0時為Comparator Mode，為1時為Interrupt Mode
    bool OSPolarity;     //為0時ALERT觸發時為低電位，為1時ALERT觸發時為高電位
    uint8_t FaultQueue;  //ALERT觸發前的連續錯誤次數，0:1次、1:2次、2:4次、3:6次
    bool TimeoutEnable;  //為0時ALERT在溫度回到正常範圍後會自動解除，為1時ALERT會持續直到軟體清除alert狀態

    /* Temperature Limits */
    float THYST;   //溫度回到正常範圍的threshold，當溫度從高於TOS下降到THYST以下時，ALERT會解除
    float TOS;     //溫度超過此值時會觸發ALERT

} bodyTemp_Config_t;

void initialize_LED(void);
void initialize_DHT11(void);
void initialize_DHT22(void);
void initialize_bodyTemp(void);
void initialize_AD8232(void);
void MX_I2C1_Init(void);
void MX_ADC1_Init(void);		
void delay_us(uint32_t us);

#endif /* __DRIVERS_H */


