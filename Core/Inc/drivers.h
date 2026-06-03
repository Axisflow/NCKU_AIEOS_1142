#ifndef __DRIVERS_H
#define __DRIVERS_H

typedef struct
{
	const char *name;
	const char *GPIO_Port;
	const char *GPIO_Pin;
	const char *Mode;
	const char *OutputType;
	const char *Pull;
	const char *Speed;
} LED_Config_t;

void initialize_LED(void);
void initialize_DHT22(void);

#endif /* __DRIVERS_H */


