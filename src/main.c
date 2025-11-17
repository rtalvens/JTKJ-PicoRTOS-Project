
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

//globalit muuttujat anturidatalle asentojen tunnistukseen
float ax = 0, ay = 0, az = 0;
float gx = 0, gy = 0, gz = 0;
float t = 0;

//tilaesittely
enum state { WAITING=1, DATA_READY, DOT, DASH, SPACE};
enum state programState = WAITING;

//Morse code aktiivinen tila
//Globaali muuttuja volatile, koska sitä käytetään keskeytyskäsittelijässä ja tehtävissä
volatile bool morseActive = false;

//Globaali muuttuja tehtävien synkronointiin ja merkkien välittämiseen
QueueHandle_t xSensorQueue;

/**
 * Keskeytys
 * GPIO keskeytys napille BUTTON1
 * reunat - laskeva ja nouseva
 * keskeytyskäsittelijä void buttonFxn(uint gpio, uint32_t events)
 * Vaihtaa morseActive tilaa true/false
 * Jos morseActive vaihtuu false tilaan, lähettää lopetusmerkin print_taskille (' ', ' ', '\n')
 */
void buttonFxn(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        morseActive = !morseActive;

        if (!morseActive) {
            // Lähetä lopetusmerkki print_taskille
            char endMsg[3] = {' ', ' ', '\n'};
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            for (int i = 0; i < 3; i++) {
                xQueueSendFromISR(xSensorQueue, &endMsg[i], &xHigherPriorityTaskWoken);  //synkronointimekanismi xSensorQueue
            }
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/**
 * Sensor_task
 * Lukee ICM42670 anturin dataa ja lähettää morse koodin jonoon
 * Tunnistaa asennot (DOT, DASH, SPACE) ja lähettää vastaavat merkit
 * Toimii kun nappi on painettu
 */
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
    
    while(1) {
        // Jos morse koodi ei ole aktiivinen, odota
        if (!morseActive) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Luetaan anturin dataa
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
            symbol = ' ';
        }else {
            programState = WAITING;

        }

        // Lähetetään tunnistettu symboli jonoon
         if (symbol != 0) {
            xQueueSend(xSensorQueue, &symbol, 0);
        }
       
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
}

/**
 * Print_task
 * Tulostaa morse koodin jonosta
 */

static void print_task(void *arg){
     (void)arg;
    char received;

    while (1) {
        tud_task(); 

        // Odotetaan merkkiä jonosta
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
    //Alustetaan nappi
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &buttonFxn);

    //Luodaan sensor task
    TaskHandle_t hSensorTask = NULL;
    xTaskCreate(sensor_task, "Sensor Task", 2048, NULL, 2, &hSensorTask);

    //Luodaan print task
    TaskHandle_t hPrintTask = NULL;
    xTaskCreate(print_task, "SPrint Task", 2048, NULL, 2, &hPrintTask);

    vTaskStartScheduler();

    return 0;
}
