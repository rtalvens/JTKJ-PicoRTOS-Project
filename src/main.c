
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
//#include "usbSerialDebug/helper.h"



// Exercise 4. Include the libraries necessaries to use the usb-serial-debug, and tinyusb
// Tehtävä 4 . Lisää usb-serial-debugin ja tinyusbin käyttämiseen tarvittavat kirjastot.



#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1
#define DEFAULT_I2C_SDA_PIN 12
#define DEFAULT_I2C_SCL_PIN 13



float ax = 0, ay = 0, az = 0;
float gx = 0, gy = 0, gz = 0;
float t = 0;

enum state { WAITING=1, DATA_READY};
enum state programState = WAITING;

//uint32_t ambientLight;

/*static void btn_fxn(uint gpio, uint32_t eventMask) {
    // Tehtävä 1: Vaihda LEDin tila.
    //            Tarkista SDK, ja jos et löydä vastaavaa funktiota, sinun täytyy toteuttaa se itse.
    // Exercise 1: Toggle the LED. 
    //             Check the SDK and if you do not find a function you would need to implement it yourself. 
    toggle_led();
}*/

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

    /*while(1)
    {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            printf("%lu, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", xTaskGetTickCount()*100, ax, ay, az, gx, gy, gz);
            programState = DATA_READY;
        } else {
            printf("Failed to read sensor data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}*/
    uint32_t timestamp = 0; 

    while(1) {
        if (programState == WAITING) {
            
            if(ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0){
                programState = DATA_READY;
                //printf("%lu, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", timestamp, ax, ay, az, gx, gy, gz);
            } else {
                printf("Failed to read sensor data\n");
            }

            //programState = DATA_READY;
            timestamp += 100;
        }
      
    
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void print_task(void *arg){
    uint32_t timestamp = 0; 
    (void)arg;
    while(1){
         if (programState == DATA_READY) {
            printf("{%lu, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f},\n",
                   timestamp, ax, ay, az, gx, gy, gz);

            programState = WAITING;
            timestamp += 100;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}




//static void usbTask(void *arg) {
//    (void)arg;
//    while (1) {
//        tud_task();              // With FreeRTOS wait for events
                                 // Do not add vTaskDelay. 
//    }
//}

int main() {

    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);


    
    while (!stdio_usb_connected()){
       sleep_ms(10);
    }
    
    init_hat_sdk();
    sleep_ms(300); 

    printf("Starting sensor task... \n");

    TaskHandle_t hSensorTask = NULL;
    xTaskCreate(sensor_task, "Sensor Task", 2048, NULL, 2, &hSensorTask);

    TaskHandle_t hPrintTask = NULL;
    xTaskCreate(print_task, "SPrint Task", 2048, NULL, 2, &hPrintTask);

    vTaskStartScheduler();

    /*ICM42670_startAccel(100, 2);
    ICM42670_startGyro(100, 250);

    printf("ICM42670 initialized\n");

    while (1) {
        ICM42670_read_sensor_data(float*ax, float*ay, float*az, float*gx, float*gy, float*gz, float*t);

        printf("Accel: %.2f, %.2f, %.2f | Gyro: %.2f, %.2f, %.2f\n", ax, ay, az, gx, gy, gz);

    sleep_ms(500);

        }*/

    

    return 0;
}
        
        
        
        
        //gpio_init(BUTTON1);
    //gpio_set_dir(BUTTON1, GPIO_IN);
    
    //gpio_init(LED1);
    //gpio_set_dir(LED1, GPIO_OUT);

    //gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);


    //TaskHandle_t hSensorTask, hPrintTask, hUSB = NULL;
   // TaskHandle_t hSensorTask;

   // BaseType_t result = xTaskCreate(sensor_task, "sensor", DEFAULT_STACK_SIZE, NULL, 2, &hSensorTask); 
   // if(result != pdPASS) {
      //  printf("Sensor task creation failed\n");
   //     return 0;
   // }                

    /*
    BaseType_t result = xTaskCreate(sensor_task, // (en) Task function
                "sensor",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                            // (en) Arguments of the task 
                2,                               // (en) Priority of this task
                &hSensorTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Sensor task creation failed\n");
        return 0;
    }
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }*/ 
    
    

    // Start the scheduler (never returns)
  //  vTaskStartScheduler();
    
    // Never reach this line.
    //return 0;
//}

