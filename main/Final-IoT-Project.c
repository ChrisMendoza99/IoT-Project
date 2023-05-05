/**
 * Final Project made by Christopher A. Mendoza
 * Traffic Light System W. WIFI Integration
 * UTEP 2023
 * last edited: 4/16/2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
/*Library for the Traffic System*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include <driver/dac.h>


/*Library for the WIFI*/
#include <esp_http_server.h>
#include "spi_flash_mmap.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

/*---Global and Handle Initializtions*/
#define TIME_0 75                   //Total Period of the Traffic Light System
#define HIGH 1                      //Logic High 
#define LOW 0                       //Logic Low 
#define INPUT_CAP 15                //To insure that any pedestrian presses multiple times, it resets 
/*---Task Handles---*/
TaskHandle_t Task1;
/*---Semaphore Handles---*/
SemaphoreHandle_t semaphore_pedestrian = NULL; 
// SemaphoreHandle_t mySemaphore2 = NULL;
// /*---Timers---*/
static TimerHandle_t NS_Traffic_Light = NULL;
static TimerHandle_t EW_Traffic_Light = NULL;
int32_t arrIndex_1 = 0;
int32_t arrIndex_2 = 0;
/*---Queue Handles---*/
QueueHandle_t pedestrian_queue;
QueueHandle_t pedestrian_assist_queue;
/*---Interrupt Flags and Buttons---*/
long int flag_1, flag_2, flag_3, flag_4 = 0;
#define ESP_INTR_FLAG_DEFAULT 0 //Interrupt config
/*---First four buttons are for North and South---*/
#define BUTTON_0 19
#define BUTTON_1 21
#define BUTTON_2 22
#define BUTTON_3 23
/*---First four buttons are for West and East---*/
#define BUTTON_4 35
#define BUTTON_5 34
#define BUTTON_6 39
#define BUTTON_7 36

/*================================================================== CH-1.0 - WI-FI=======================================================================*/
/*============================================================= Chapter 1.0: Wi-Fi and Web-Server=========================================================*/
char on_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center; background-color: #101010;}h2{  color: white;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Traffic Pedestrian Request</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>Traffic Pedestrian Request</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-walking fa-2x\" style=\"color:#2a2a2a;\"></i>     <strong>North & South</strong></p>        <p>          <a href=\"/nsbutton1\"><button class=\"button\">ON</button></a>          <a href=\"/nsbutton2\"><button class=\"button button2\">OFF</button></a>        </p>      </div>      <div class=\"card\">        <p><i class=\"fas fa-walking fa-2x\" style=\"color:#2a2a2a;\"></i>     <strong>East & West</strong></p>        <p>          <a href=\"/ewbutton1\"><button class=\"button\">ON</button></a>          <a href=\"/ewbutton2\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>";

char off_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;background-color: #101010;}h2{  color: white;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Traffic Pedestrian Request</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>Traffic Pedestrian Request</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-walking fa-2x\" style=\"color:#2a2a2a;\"></i>     <strong>North & South</strong></p>        <p>          <a href=\"/nsbutton1\"><button class=\"button\">ON</button></a>          <a href=\"/nsbutton2\"><button class=\"button button2\">OFF</button></a>        </p>      </div>      <div class=\"card\">        <p><i class=\"fas fa-walking fa-2x\" style=\"color:#2a2a2a;\"></i>     <strong>East & West</strong></p>        <p>          <a href=\"/ewbutton1\"><button class=\"button\">ON</button></a>          <a href=\"/ewbutton2\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>";

static const char *TAG = "espressif"; // TAG for debug

#define WIFI_SSID "" //username
#define WIFI_PASS "" //password
#define MAXIMUM_RETRY 10 //The esp32 will try to connect the wifi(or router) and try to connect to the preset user and pass
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
//#define LED_PIN 2
int led_state =  0;
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void connect_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
  //This struct is where we are setting the WIFI USER and PASS
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    //If we read the monitor in the terminal, we can see the WiFi reading into this
    if (bits & WIFI_CONNECTED_BIT)
    {
      ESP_LOGI(TAG, "Connection Succesfull!!!")
      ESP_LOGI(TAG, "ESP32 is connected to SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
      ESP_LOGI(TAG, "Connection Failed!!!")
      ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    vEventGroupDelete(s_wifi_event_group);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    if (led_state == 0)
        response = httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
    else
        response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    return response;
}
esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}
/*======================= Chapter 1.4 - Button Handlers =========================*/
esp_err_t NW1led_on_handler(httpd_req_t *req)
{
    //gpio_set_level(LED_PIN, 1);
    //led_state = 1;
    const int button_1 = 21;
    xQueueSendToFront(pedestrian_queue,&button_1, 0);
    return send_web_page(req);
}

esp_err_t NW2led_off_handler(httpd_req_t *req)
{
    // gpio_set_level(LED_PIN, 0);
  const int button_2 = 23;
  xQueueSendToFront(pedestrian_queue,&button_2,0);
  return send_web_page(req);
}

esp_err_t EW1led_on_handler(httpd_req_t *req)
{
    // gpio_set_level(LED_PIN, 1);
    // led_state = 1;
  const int button_3 = 34;
  xQueueSendToFront(pedestrian_queue,&button_3,0 );
  return send_web_page(req);
}

esp_err_t EW2led_off_handler(httpd_req_t *req)
{
    // gpio_set_level(LED_PIN, 0);
    // led_state = 0;
  const int button_4 = 36;
  xQueueSendToFront(pedestrian_queue, &button_4,0);
  return send_web_page(req);
}

/*======================== Chapter 1.5 HTTP-URI Handlers======================*/
httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_on_NW1 = {
    .uri = "/nsbutton1",
    .method = HTTP_GET,
    .handler = NW1led_on_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_off_NW2 = {
    .uri = "/nsbutton2",
    .method = HTTP_GET,
    .handler = NW2led_off_handler,
    .user_ctx = NULL
};
httpd_uri_t uri_on_EW1 = {
    .uri = "/ewbutton1",
    .method = HTTP_GET,
    .handler = EW1led_on_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_off_EW2 = {
    .uri = "/ewbutton2",
    .method = HTTP_GET,
    .handler = EW2led_off_handler,
    .user_ctx = NULL
};

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_on_NW1);
        httpd_register_uri_handler(server, &uri_off_NW2);
        httpd_register_uri_handler(server, &uri_on_EW1);
        httpd_register_uri_handler(server, &uri_off_EW2);
        
    }

    return server;
}



/* ==================================================== CH-2.0 - The Traffic Systems ================================================================*/
/*--- CH-2.1 Pedestrian GPIO Interrupt Handler---*/
static void IRAM_ATTR gpio_isr_handler(void* pinData)
{
    //This gets which pin was toggled
    gpio_num_t buttonPin = (gpio_num_t) pinData;
    xQueueSendToFront(pedestrian_queue, &buttonPin,0);
}
/* Initializing the Pins needed for this program */
void GPIOInitOutput(uint8_t* Outpins, int OutSize)
{    
    /*--- The for loop initializes the Outputs*/
    for(int i = 0; i < OutSize; i++)
    {
        esp_rom_gpio_pad_select_gpio(Outpins[i]);
        gpio_set_direction(Outpins[i], GPIO_MODE_OUTPUT);
    }
    for(int i = 0; i < OutSize; i++)
        gpio_set_level(Outpins[i], LOW);
}
/*----CH-2.2 Traffic lights for North and South----*/
void Traffic_Light_NS(TimerHandle_t xTimer)
{
    const int32_t street_time_1 = TIME_0;
    arrIndex_1 += 1;
    gpio_set_level(GPIO_NUM_4, HIGH);
    if(arrIndex_1 < 22 && flag_1 >= 1)
    {
        gpio_set_level(GPIO_NUM_18, HIGH);
    }
    if(arrIndex_1 < 22 && flag_2 >= 1)
    {
        gpio_set_level(GPIO_NUM_5, HIGH);
    }   
    if(arrIndex_1 > 22)
    {
        gpio_set_level(GPIO_NUM_4, LOW);
        gpio_set_level(GPIO_NUM_2, HIGH);


        if(flag_1 >= 1) //Flag 1 enabled
        {
            if((flag_1 >= 6 || flag_2 >= 6) && (arrIndex_1 > 22 && arrIndex_1 < 32))
            {
                xSemaphoreGive(semaphore_pedestrian);
            }
            gpio_set_level(GPIO_NUM_18, HIGH);
        }
        if(flag_2 >= 1) //Flag 2 enabled
        {
            if((flag_1 >= 6 || flag_2 >= 6) && (arrIndex_1 > 22 && arrIndex_1 < 32))
            {
                xSemaphoreGive(semaphore_pedestrian);
            }
            gpio_set_level(GPIO_NUM_5, HIGH);
        }
    }
    if(arrIndex_1 == 32)
    {
        flag_1 = 0; flag_2 = 0;
    }
    if(arrIndex_1 > 32)
    {
        /*Disable the Pedestrian Lights*/
        gpio_set_level(GPIO_NUM_5, LOW);
        gpio_set_level(GPIO_NUM_18, LOW);
        gpio_set_level(GPIO_NUM_2, LOW); //Yellow Timing off
        gpio_set_level(GPIO_NUM_15, HIGH); //Red Timing on

    }        
    if(arrIndex_1 == street_time_1)
    {
        arrIndex_1 = 0;
        gpio_set_level(GPIO_NUM_15, LOW);
        gpio_set_level(GPIO_NUM_4, HIGH);
    }
}
  
/*----CH-2.3 Traffic lights for West and East----*/
void Traffic_Light_EW(TimerHandle_t xTimer)
{
    const int32_t street_time_2 = TIME_0;
    arrIndex_2 += 1;
    gpio_set_level(GPIO_NUM_13, HIGH);
    if(arrIndex_2 > 32)
    {
        gpio_set_level(GPIO_NUM_13, LOW);
        gpio_set_level(GPIO_NUM_14, HIGH);
        if(flag_3 >= 1)
        {
            gpio_set_level(GPIO_NUM_27, HIGH);
        }
        if(flag_4 >= 1)
        {
            gpio_set_level(GPIO_NUM_26, HIGH);
        }
    }
    if(arrIndex_2 > 61)
    {
        gpio_set_level(GPIO_NUM_14, LOW);
        gpio_set_level(GPIO_NUM_12, HIGH);
        if(flag_3 >= 1)
        {
            if((flag_3 >= 6 || flag_4 >= 6) && (arrIndex_2 > 61 && arrIndex_2 < TIME_0))
            {
                xSemaphoreGive(semaphore_pedestrian);
            }
            gpio_set_level(GPIO_NUM_27, HIGH);
        }
        if(flag_4 >= 1)
        {
            if((flag_3 >= 6 || flag_4 >= 6) && (arrIndex_2 > 61 && arrIndex_2 < TIME_0))
            {
                xSemaphoreGive(semaphore_pedestrian);
            }
            gpio_set_level(GPIO_NUM_26, HIGH);
        }
    }
    if(arrIndex_2 == street_time_2)
    {
        arrIndex_2 = 0;
        flag_3 = 0; flag_4 = 0;
        
        gpio_set_level(GPIO_NUM_12, LOW);
        gpio_set_level(GPIO_NUM_27, LOW);
        gpio_set_level(GPIO_NUM_26, LOW);
        gpio_set_level(GPIO_NUM_13, HIGH);
    }
}
/*---CH-2.4 Interrupt Subroutine Setup---*/
void pedestrian_input_setup()
{
     /* ---IO configuration */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;  /* Set up as Negative Edge */ 
    io_conf.mode = GPIO_MODE_INPUT;     /* Set pins as input */
    io_conf.pin_bit_mask = (1ULL << BUTTON_0) | (1ULL << BUTTON_1)| (1ULL << BUTTON_2)| (1ULL << BUTTON_3)| (1ULL << BUTTON_4)| (1ULL << BUTTON_5)| (1ULL << BUTTON_6)| (1ULL << BUTTON_7);  /* Add input bit mask */
    io_conf.pull_down_en = 1;   /* Enable pulldown */
    io_conf.pull_up_en = 0;     /* Disable pullup */
    /* Set configuration */
    gpio_config(&io_conf);
    /* Set default interrupt flag */
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    /*---All the buttons are set to be interruptable---*/
    gpio_isr_handler_add(BUTTON_0, gpio_isr_handler, (void*) BUTTON_0); 
    gpio_isr_handler_add(BUTTON_1, gpio_isr_handler, (void*) BUTTON_1);
    gpio_isr_handler_add(BUTTON_2, gpio_isr_handler, (void*) BUTTON_2);
    gpio_isr_handler_add(BUTTON_3, gpio_isr_handler, (void*) BUTTON_3); 
    gpio_isr_handler_add(BUTTON_4, gpio_isr_handler, (void*) BUTTON_4);
    gpio_isr_handler_add(BUTTON_5, gpio_isr_handler, (void*) BUTTON_5);
    gpio_isr_handler_add(BUTTON_6, gpio_isr_handler, (void*) BUTTON_6);
    gpio_isr_handler_add(BUTTON_7, gpio_isr_handler, (void*) BUTTON_7);  
}

/*---CH-2.5 Looks out for a pedestrian pushing a button---*/
void Task_street_pedestrian()
{   
    uint8_t rxData; //The data buffer for the recieving queue
    while(1)
    {
        if(xQueueReceive(pedestrian_queue, &rxData, (TickType_t)100) == pdPASS)
        {
            //printf("GPIO %i was recieved!! [%li]\n", rxData, xTaskGetTickCount()); //and here we print the data
            /*---First four "ifs" are for North and South*/
            if(rxData == 19 || rxData == 21) //Button 2
            {
                flag_1 += 1;
                if(flag_1 == INPUT_CAP)
                    flag_1 = 6;
                //printf("%li\n", flag_1);
            }
            if(rxData == 22 || rxData == 23) //Button 4
            {
                flag_2 += 1;
                if(flag_2 == INPUT_CAP)
                    flag_2 = 6;
                //printf("%li\n", flag_2);
            }
            /*The rest are for East and West*/
            if(rxData == 35 || rxData == 34) //Button 1
            {
                flag_3 += 1;
                if(flag_3 == INPUT_CAP)
                    flag_3 = 6;
                //printf("%li\n", flag_3);
            }
            if(rxData == 39 || rxData == 36) //Button 5
            {
                flag_4+= 1;
                if(flag_4 == INPUT_CAP)
                    flag_4 = 6;
                //printf("%li\n", flag_4);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
/*---CH-2.6 The task that will handle toggling the buzzer for however long it takes---*/
void task_pedestrian_assistance()
{
    while(1)
    {
        /* wait for 1000 ticks to receive semaphore */
         if(xSemaphoreTake(semaphore_pedestrian, 75/portTICK_PERIOD_MS) == pdTRUE)
         {
            gpio_set_level(GPIO_NUM_25, HIGH);  
            vTaskDelay(100/portTICK_PERIOD_MS); 
            gpio_set_level(GPIO_NUM_25, LOW);   
            vTaskDelay(100/portTICK_PERIOD_MS);
         }
         else
         {
            vTaskDelay(100/portTICK_PERIOD_MS); /* 100 ms delay */
         }
    }
}

/*=============CH-2.7 Our main app for the programm============*/
void app_main(void)
{
    /*=================*/
    /*---Pins for both the leds, and buttons---*/
    uint8_t OutPins[] = {15,2,4,13,12,14,18,5,27,26,25};
    int OutSize = sizeof(OutPins) / sizeof(uint8_t);
    GPIOInitOutput(OutPins, OutSize);
    /*---Setting up the Interrupt Services---*/
    pedestrian_input_setup();
    /*---Queue Creation(pending use)---*/
    pedestrian_queue = xQueueCreate(5, sizeof(int));
    /*---Semaphore Binary---*/
    semaphore_pedestrian = xSemaphoreCreateBinary();  
    /*---Creating the Timers for the Traffic Lights for North and South---*/
    NS_Traffic_Light = xTimerCreate("Traffic_Light_NW",1000/portTICK_PERIOD_MS,pdTRUE,(void*)0,Traffic_Light_NS);
    EW_Traffic_Light = xTimerCreate("Traffic_Light_EW",1000/portTICK_PERIOD_MS,pdTRUE,(void*)1,Traffic_Light_EW);
    
    /*--Creating Task*/
    xTaskCreate(Task_street_pedestrian,"Task_1",2048,NULL,5, &Task1);
    xTaskCreate(task_pedestrian_assistance,"Task 2", 2048,NULL, 5, NULL);    
    /*---Timer starts!---*/
    xTimerStart(NS_Traffic_Light, portMAX_DELAY);
    xTimerStart(EW_Traffic_Light, portMAX_DELAY);

    /*=========================WIFI INITILIZATION================================*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    connect_wifi();
    led_state = 0;

    ESP_LOGI(TAG, "TRAFFIC LED EVENT IS ACTIVE!!! ... ...\n");
    setup_server();

} 
