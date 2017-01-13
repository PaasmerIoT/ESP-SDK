/*
 * Derived from examples/mqtt_client/mqtt_client.c - added TLS1.2 support and some minor modifications.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <ssid_config.h>

#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

// this must be ahead of any mbedtls header files so the local mbedtls/config.h can be properly referenced
#include "ssl_connection.h"

//added for configuration 
#include "../../Paasmer_iot_config.h"
//#include "config.h"
/*#define UserName "v" //your user name in website

#define DeviceName "v" //your device name

#define feedname1 "v" //feed name you use in the website

#define sensorpin1 1 //modify with the pin number which you connected the sensor

#define feedname2 "v" //feed name you use in the website

#define sensorpin2 1 //modify with the pin number which you connected the sensor

#define feedname3 "v" //feed name you use in the website

#define sensorpin3 1 //modify with the pin number which you connected the sensor

#define analogfeedname "v" //feed name you use in the website for analog readings

#define controlfeedname1 "v" //feed name you use in the website for control device pins

#define controlpin1 1 //modify with the pin number which you connected the control device (eg.: motor)

#define controlfeedname2 "v" //feed name you use in the website for control device pins

#define controlpin2 1 //modify with the pin number which you connected the control device (eg.: motor)

#define timePeriod 1000 //change the time delay as you required for sending sensor values to paasmer cloud*/
////////////

#define MQTT_PUB_TOPIC "esp8266/status"
#define MQTT_SUB_TOPIC "esp8266/control"
#define GPIO_LED 2
#define MQTT_PORT 8883
char PUB_TOPIC[50];
char SUB_TOPIC[100];
/* certs, key, and endpoint */
extern char *ca_cert, *client_endpoint, *client_cert, *client_key;

static int wifi_alive = 0;
static int ssl_reset;
static SSLConnection *ssl_conn;
static xQueueHandle publish_queue;

///added for publishing one by one
int devicedetails=1,sensordetails=0;

static void beat_task(void *pvParameters) {
    char msg[16];
    int count = 0;

    while (1) {
        if (!wifi_alive) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        printf("Schedule to publish\r\n");

        snprintf(msg, sizeof(msg), "%d", count);
        if (xQueueSend(publish_queue, (void *) msg, 0) == pdFALSE) {
            printf("Publish queue overflow\r\n");
        }

        //vTaskDelay(10000 / portTICK_RATE_MS);
	vTaskDelay(timePeriod / portTICK_RATE_MS);
    }
}

static void topic_received(mqtt_message_data_t *md) {
    mqtt_message_t *message = md->message;
    int i;

    printf("Received: ");
    for (i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[i]);

    printf(" = ");
    for (i = 0; i < (int) message->payloadlen; ++i)
        printf("%c", ((char *) (message->payload))[i]);
    printf("\r\n");

    /*if (!strncmp(message->payload, "on", 2)) {
        printf("Turning on LED\r\n");
        gpio_write(GPIO_LED, 1);
    } else if (!strncmp(message->payload, "off", 3)) {
        printf("Turning off LED\r\n");
        gpio_write(GPIO_LED, 0);
    }*/
    //if (!strncmp(message->payload, controlfeedname1, sizeof(controlfeedname1))) {
        //printf("\r\n");
        //gpio_write(GPIO_LED, 1);
char controlstring1[100],controlstring2[100],controlstring3[100],controlstring4[100];
sprintf(controlstring1,"%s on",controlfeedname1);
sprintf(controlstring2,"%s off",controlfeedname1);
sprintf(controlstring3,"%s on",controlfeedname2);
sprintf(controlstring4,"%s off",controlfeedname2);

if(!strncmp(message->payload,controlstring1,strlen(controlstring1)))
        {
        gpio_write(controlpin1, 1);
        printf("%s is on",controlfeedname1);
        }
if(!strncmp(message->payload,controlstring2,strlen(controlstring2)))
        {
        gpio_write(controlpin1, 0);
        printf("%s is off",controlfeedname1);
        }

if(!strncmp(message->payload,controlstring3,strlen(controlstring3)))
        {
        gpio_write(controlpin2, 1);
        printf("%s is on",controlfeedname2);
        }

if(!strncmp(message->payload,controlstring4,strlen(controlstring4)))
        {
        gpio_write(controlpin2, 0);
        printf("%s is off",controlfeedname2);
        }

	if (!strcmp(message->payload, "on")) {
        printf("Turning on %s\r\n",controlfeedname1);
        gpio_write(controlpin1, 1);}
     else if (!strncmp(message->payload, "off", 3)) {
        printf("Turning off LED\r\n");
        gpio_write(GPIO_LED, 0);
    }
	
}

static const char *get_my_id(void) {
    // Use MAC address for Station as unique ID
    static char my_id[13];
    static bool my_id_done = false;
    int8_t i;
    uint8_t x;
    if (my_id_done)
        return my_id;
    if (!sdk_wifi_get_macaddr(STATION_IF, (uint8_t *) my_id))
        return NULL;
    for (i = 5; i >= 0; --i) {
        x = my_id[i] & 0x0F;
        if (x > 9)
            x += 7;
        my_id[i * 2 + 1] = x + '0';
        x = my_id[i] >> 4;
        if (x > 9)
            x += 7;
        my_id[i * 2] = x + '0';
    }
    my_id[12] = '\0';
    my_id_done = true;
    return my_id;
}

static int mqtt_ssl_read(mqtt_network_t * n, unsigned char* buffer, int len,
        int timeout_ms) {
    int r = ssl_read(ssl_conn, buffer, len, timeout_ms);
    if (r <= 0
            && (r != MBEDTLS_ERR_SSL_WANT_READ
                    && r != MBEDTLS_ERR_SSL_WANT_WRITE
                    && r != MBEDTLS_ERR_SSL_TIMEOUT)) {
        printf("%s: TLS read error (%d), resetting\n\r", __func__, r);
        ssl_reset = 1;
    };
    return r;
}

static int mqtt_ssl_write(mqtt_network_t* n, unsigned char* buffer, int len,
        int timeout_ms) {
    int r = ssl_write(ssl_conn, buffer, len, timeout_ms);
    if (r <= 0
            && (r != MBEDTLS_ERR_SSL_WANT_READ
                    && r != MBEDTLS_ERR_SSL_WANT_WRITE)) {
        printf("%s: TLS write error (%d), resetting\n\r", __func__, r);
        ssl_reset = 1;
    }
    return r;
}

static void mqtt_task(void *pvParameters) {
    int ret = 0;
    struct mqtt_network network;
    mqtt_client_t client = mqtt_client_default;
    char mqtt_client_id[20];
    //uint8_t mqtt_buf[100];
    //uint8_t mqtt_readbuf[100];
    uint8_t mqtt_buf[512];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, "ESP-");
    strcat(mqtt_client_id, get_my_id());

    ssl_conn = (SSLConnection *) malloc(sizeof(SSLConnection));
    while (1) {
        if (!wifi_alive) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        printf("%s: started\n\r", __func__);
        ssl_reset = 0;
        ssl_init(ssl_conn);
        ssl_conn->ca_cert_str = ca_cert;
        ssl_conn->client_cert_str = client_cert;
        ssl_conn->client_key_str = client_key;

        mqtt_network_new(&network);
        network.mqttread = mqtt_ssl_read;
        network.mqttwrite = mqtt_ssl_write;

        printf("%s: connecting to MQTT server %s ... ", __func__,
                client_endpoint);
        ret = ssl_connect(ssl_conn, client_endpoint, MQTT_PORT);

        if (ret) {
            printf("error: %d\n\r", ret);
            ssl_destroy(ssl_conn);
            continue;
        }
        printf("done\n\r");
        mqtt_client_new(&client, &network, 5000, mqtt_buf, 512, mqtt_readbuf,
                100);

        data.willFlag = 0;
        data.MQTTVersion = 4;
        data.cleansession = 1;
        data.clientID.cstring = mqtt_client_id;
        data.username.cstring = NULL;
        data.password.cstring = NULL;
        data.keepAliveInterval = 1000;
        printf("Send MQTT connect ... ");
        ret = mqtt_connect(&client, &data);
        if (ret) {
            printf("error: %d\n\r", ret);
            ssl_destroy(ssl_conn);
            continue;
        }
        printf("done\r\n");
	sprintf(SUB_TOPIC,"%s_%s",UserName,DeviceName);
        mqtt_subscribe(&client, SUB_TOPIC, MQTT_QOS1, topic_received);
        xQueueReset(publish_queue);

        while (wifi_alive && !ssl_reset) {
            char msg[512];
            while (xQueueReceive(publish_queue, (void *) msg, 0) == pdTRUE) {
                portTickType task_tick = xTaskGetTickCount();
                uint32_t free_heap = xPortGetFreeHeapSize();
                uint32_t free_stack = uxTaskGetStackHighWaterMark(NULL);
		if(UserName != "" && DeviceName !=""){
		if (sensordetails==1){
                //snprintf(msg, sizeof(msg), "%u: free heap %u, free stack %u",
                        //task_tick, free_heap, free_stack * 4);
		//sprintf(msg, "%u: free heap %u, free stack %u",
                  //      task_tick, free_heap, free_stack * 4);
		float a = sdk_system_adc_read();
		int b = gpio_read(sensorpin1),c = gpio_read(sensorpin2),d = gpio_read(sensorpin3);
		//int b = 1,c = 0,d = 1;//int a = 1;
		
		snprintf(msg,sizeof(msg),"{\n\"feedname1\" : \"%s\",\n\"sensorvalue1\" : \"%d\",\n\"feedname2\" : \"%s\",\n\"sensorvalue2\" : \"%d\",\n\"feedname3\" : \"%s\",\n\"sensorvalue3\" : \"%d\",\n\"feedname4\" : \"%s\",\n\"sensorvalue4\" : \"%f\",\n\"username\" : \"%s\",\n\"devicename\" : \"%s\"\n}",feedname1,b,feedname2,c,feedname3,d,analogfeedname,a,UserName,DeviceName);
		//sprintf(msg,"{\n\"analogfeedname\" : \"%s\",\n\"analogsensorvalue\" : \"%d\",\n\"username\" : \"%s\",\n\"devicename\" : \"%s\"\n}",analogfeedname,a,UserName,DeviceName);
		sprintf(PUB_TOPIC,"paasmer_sensor_details");
		}
		
		else if (devicedetails==1){
                //snprintf(msg, sizeof(msg), "%u: free heap %u, free stack %u",
                       // task_tick, free_heap, free_stack * 4);
		//snprintf(msg,sizeof(msg), "{\n\"username\" : \"%s\",\n\"devicename\" : \"%s\"\n}",UserName,DeviceName);
		snprintf(msg,sizeof(msg),"{\n\"feedname1\" : \"%s\",\n\"feedtype1\" : \"sensor\",\n\"feedname2\" : \"%s\",\n\"feedtype2\" : \"sensor\",\n\"feedname3\" : \"%s\",\n\"feedtype3\" : \"sensor\",\n\"feedname4\" : \"%s\",\n\"feedtype4\" : \"sensor\",\n\"feedname5\" : \"%s\",\n\"feedtype5\" : \"control\",\n\"feedname6\" : \"%s\",\n\"feedtype6\" : \"control\",\n\"username\" : \"%s\",\n\"devicename\" : \"%s\"\n}",feedname1,feedname2,feedname3,analogfeedname,controlfeedname1,controlfeedname2,UserName,DeviceName);
                //printf("Publishing: %s\r\n", msg);
		sprintf(PUB_TOPIC,"paasmer_device_details");
                devicedetails=0;
		sensordetails=1;
		//vTaskDelay(timePeriod);//vTaskDelay(timePeriod);
		}
		
		
                printf("Publishing: %s\r\n", msg);

                mqtt_message_t message;
                message.payload = msg;
                message.payloadlen = strlen(msg);
                message.dup = 0;
                message.qos = MQTT_QOS1;
                message.retained = 0;
                ret = mqtt_publish(&client, PUB_TOPIC, &message);
                if (ret != MQTT_SUCCESS) {
                    printf("error while publishing message: %d\n", ret);
                    break;
                }}
		//int i=0;
		//for(i=0;i<=timePeriod;i++){}
            }

            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED)
                break;
	    
        }
        printf("Connection dropped, request restart\n\r");
        ssl_destroy(ssl_conn);
    }
}

static void wifi_task(void *pvParameters) {
    uint8_t status = 0;
    uint8_t retries = 30;
    struct sdk_station_config config = { .ssid = WIFI_SSID, .password =
            WIFI_PASS, };

    printf("%s: Connecting to WiFi\n\r", __func__);
    sdk_wifi_set_opmode (STATION_MODE);
    sdk_wifi_station_set_config(&config);

    while (1) {
        wifi_alive = 0;

        while ((status != STATION_GOT_IP) && (retries)) {
            status = sdk_wifi_station_get_connect_status();
            printf("%s: status = %d\n\r", __func__, status);
            if (status == STATION_WRONG_PASSWORD) {
                printf("WiFi: wrong password\n\r");
                break;
            } else if (status == STATION_NO_AP_FOUND) {
                printf("WiFi: AP not found\n\r");
                break;
            } else if (status == STATION_CONNECT_FAIL) {
                printf("WiFi: connection failed\r\n");
                break;
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
            --retries;
        }

        while ((status = sdk_wifi_station_get_connect_status())
                == STATION_GOT_IP) {
            if (wifi_alive == 0) {
                printf("WiFi: Connected\n\r");
                wifi_alive = 1;
            }
            vTaskDelay(500 / portTICK_RATE_MS);
        }

        wifi_alive = 0;
        printf("WiFi: disconnected\n\r");
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void user_init(void) {
    uart_set_baud(0, 115200);
    printf("SDK version: %s, free heap %u\n", sdk_system_get_sdk_version(),
            xPortGetFreeHeapSize());

    gpio_enable(GPIO_LED, GPIO_OUTPUT);
    gpio_write(GPIO_LED, 1);
    gpio_enable(controlpin1, GPIO_OUTPUT);
    gpio_enable(controlpin2, GPIO_OUTPUT);
	

    publish_queue = xQueueCreate(3, 16);
    xTaskCreate(&wifi_task, (int8_t *) "wifi_task", 256, NULL, 2, NULL);
    xTaskCreate(&beat_task, (int8_t *) "beat_task", 256, NULL, 2, NULL);
    xTaskCreate(&mqtt_task, (int8_t *) "mqtt_task", 2048, NULL, 2, NULL);
}
