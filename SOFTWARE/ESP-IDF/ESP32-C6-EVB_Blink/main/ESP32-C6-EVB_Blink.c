/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define	LED		8
#define	BUTTON	9
#define	NUMBER_OF_RELAYS	4
const char Output[NUMBER_OF_RELAYS] = {10, 11, 22, 23};

static void Init_GPIO(void)
{
	int i;
	for (i=0; i<4; i++)
	{
		gpio_reset_pin(Output[i]);
		gpio_set_direction(Output[i], GPIO_MODE_INPUT_OUTPUT);

	}
	gpio_reset_pin(LED);
	gpio_set_direction(LED, GPIO_MODE_INPUT_OUTPUT);

	gpio_reset_pin(BUTTON);
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
}


void app_main(void)
{
	int i, ActiveRelay=0;
	Init_GPIO();

	while (1)
	{
		for (i=0; i<NUMBER_OF_RELAYS; i++)
			if (i == ActiveRelay)	// check the active relay
				gpio_set_level (Output[i], 1);	// turn on the active relay
			else
				gpio_set_level (Output[i], 0);	// turn off the others

		gpio_set_level (LED, !gpio_get_level(LED));	// toggle LED
		vTaskDelay(1000 / portTICK_PERIOD_MS);	// short delay (1 sec)
		
		while (!gpio_get_level (BUTTON));	// wait while the button is hold down
		ActiveRelay=(ActiveRelay+1)%NUMBER_OF_RELAYS;	// change the active relay 0 to 3
	}
}
