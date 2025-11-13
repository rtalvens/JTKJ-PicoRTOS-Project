
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include <tusb.h>




#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1
#define DEFAULT_I2C_SDA_PIN 12
#define DEFAULT_I2C_SCL_PIN 13


float ax = 0, ay = 0, az = 0;
float gx = 0, gy = 0, gz = 0;
float t = 0;

enum state { WAITING=1, DATA_READY, DOT, DASH, SPACE};
enum state programState = WAITING;

QueueHandle_t xSensorQueue;


static void sensor_task(void *arg){
    (void)arg;

    if(init_ICM42670() == 0) {
        printf("ICM42670 initialized successfully\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("ICM42670 start failed\n");
        }
    } else {
        printf("ICM42670 initialization failed\n");
    }

    
    //uint32_t timestamp = 0; 
    
    while(1) {
        ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);

        char symbol = 0;
        
        if (ax <= -0.95) {
            programState = DOT;
            symbol = '.';
        } else if (ay <= -0.95) {
            programState = DASH;
            symbol = '-';
        } else if (az >=  0.95) {  
            programState = SPACE;
            symbol = '_';
        }else {
            programState = WAITING;

        }

         if (symbol != 0) {
            xQueueSend(xSensorQueue, &symbol, 0);
        }
       
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
}



static void print_task(void *arg){
     (void)arg;
    char received;

    while (1) {
        tud_task(); 

        if (xQueueReceive(xSensorQueue, &received, portMAX_DELAY) == pdPASS) {
            printf("%c", received);
            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

}



int main() {

    stdio_init_all();
    sleep_ms(1000);
    
    init_hat_sdk();
    sleep_ms(300); 

    printf("Starting sensor task... \n");

     xSensorQueue = xQueueCreate(10, sizeof(char));
    if (xSensorQueue == NULL) {
        printf("Queue creation failed!\n");
        while (1);
    }

    //Create sensor task
    TaskHandle_t hSensorTask = NULL;
    xTaskCreate(sensor_task, "Sensor Task", 2048, NULL, 2, &hSensorTask);

    //Create print task
    TaskHandle_t hPrintTask = NULL;
    xTaskCreate(print_task, "SPrint Task", 2048, NULL, 2, &hPrintTask);


    vTaskStartScheduler();

    return 0;
}
        
 
