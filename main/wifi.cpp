#include "wifi.h"

#include <string.h>
#include <esp_event.h>
#include <esp_log.h>
#include <mdns.h>
#include <esp_netif_ip_addr.h>
#include "nvs.h"

#define TAG "wlan"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>


const uint32_t Wifi::SBIT_CONNECTED;
const uint32_t Wifi::SBIT_ACTIVE;
const int Wifi::max_retries;
int Wifi::number_of_retries = 0;
esp_netif_t* sta_netif = NULL;
EventGroupHandle_t Wifi::wifi_status_bits;
SemaphoreHandle_t Wifi::ip_semaphore;
esp_ip4_addr_t Wifi::wifi_client_ip;
esp_ip4_addr_t Wifi::zero_ip;
wifi_config_t Wifi::config;
esp_timer_handle_t Wifi::reconnect_timer;

void Wifi::setup() {

	wifi_status_bits = xEventGroupCreate();
	bool active = Nvs::getBit("wifi", "active", false);

    esp_netif_set_ip4_addr(&wifi_client_ip, 0, 0, 0, 0);
    esp_netif_set_ip4_addr(&zero_ip, 0, 0, 0, 0);

	ip_semaphore = xSemaphoreCreateBinary();
	xSemaphoreGive(ip_semaphore);

	ESP_ERROR_CHECK(esp_netif_init());
	sta_netif = esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    esp_timer_create_args_t timer_conf;
	timer_conf.callback = reconnect_timer_callback;
	timer_conf.dispatch_method = ESP_TIMER_TASK;
	timer_conf.name = "reconnect_timer";
	timer_conf.arg = NULL;
	esp_timer_create(&timer_conf, &reconnect_timer);

    if (active) {
    	enable();
    }
}

bool Wifi::enable() {
	if (is_enabled()) {
		return true;
	}

	number_of_retries = 0;
	esp_timer_stop(reconnect_timer);
	esp_err_t ret = esp_wifi_start();

	if (ret == ESP_OK) {
		xEventGroupSetBits(wifi_status_bits, SBIT_ACTIVE);
		Nvs::setBit("wifi", "active", true);
		Nvs::commit();
		return true;
	}
	return false;
}

bool Wifi::disable() {
	if (!is_enabled()) {
		return true;
	}

	// for the reconnect timer not to reconnect immediately
	xEventGroupClearBits(wifi_status_bits, SBIT_ACTIVE);

	if (is_connected()) {
		if (esp_wifi_disconnect() != ESP_OK) {
			xEventGroupSetBits(wifi_status_bits, SBIT_ACTIVE);
			return false;
		}
		xEventGroupClearBits(wifi_status_bits, SBIT_CONNECTED);
	}

	Nvs::setBit("wifi", "active", false);
	Nvs::commit();
	esp_err_t ret = esp_wifi_stop();

	if (ret == ESP_OK) {
		esp_timer_stop(reconnect_timer);
		return true;
	}

	// disabling was not successful
	xEventGroupSetBits(wifi_status_bits, SBIT_ACTIVE);
	return false;
}

bool Wifi::is_enabled() {
	return (xEventGroupGetBits(wifi_status_bits) & SBIT_ACTIVE) != 0;
}

bool Wifi::is_connected() {
	return (xEventGroupGetBits(wifi_status_bits) & SBIT_CONNECTED) != 0;
}

string Wifi::get_ip() {

	string result;
	xSemaphoreTake(ip_semaphore, portMAX_DELAY);
	{
		char response[64];
		esp_ip4addr_ntoa(&wifi_client_ip, response, 64);
	    result = response;
    }
    xSemaphoreGive(ip_semaphore);

    return result;
}

string Wifi::get_ssid() {
	load_config();
	return string((char*) config.sta.ssid);
}

bool Wifi::set_ssid(string ssid) {
	load_config();

	strcpy((char*) config.sta.ssid, ssid.c_str());

	return commit_config();
}

string Wifi::get_pass() {
	load_config();
	return string((char*) config.sta.password);
}

bool Wifi::set_pass(string pass) {
	load_config();

	strcpy((char*) config.sta.password, pass.c_str());

	return commit_config();
}

void Wifi::load_config() {
	esp_wifi_get_config(WIFI_IF_STA, &config);
}

bool Wifi::commit_config() {
	return esp_wifi_set_config(WIFI_IF_STA, &config) == ESP_OK;
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

	if (event_base == WIFI_EVENT) {

		switch(event_id) {
        
	        case WIFI_EVENT_STA_START:
	        	ESP_LOGI(TAG, "Subsystem started");
	            esp_wifi_connect();
	            break;

	        case WIFI_EVENT_STA_CONNECTED:
	        	ESP_LOGI(TAG, "Connected");
	        	esp_timer_stop(reconnect_timer);
	        	number_of_retries = 0;

	            esp_netif_set_hostname(sta_netif, DHCP_HOSTNAME);
	            break;

	        case WIFI_EVENT_STA_DISCONNECTED:
	        	ESP_LOGI(TAG, "Disconnected");

	            xEventGroupClearBits(wifi_status_bits, SBIT_CONNECTED);
	            xSemaphoreTake(ip_semaphore, portMAX_DELAY);
	            esp_netif_set_ip4_addr(&wifi_client_ip, 0, 0, 0, 0);
	            xSemaphoreGive(ip_semaphore);

	            if ((xEventGroupGetBits(wifi_status_bits) & SBIT_ACTIVE) != 0 &&
	            	number_of_retries < max_retries) {
	            	ESP_LOGI(TAG, "Try to reconnect (retry %d of %d)...", number_of_retries + 1, max_retries);
	            	esp_wifi_connect();
	            	number_of_retries++;

	            	if (number_of_retries == max_retries) {
	            		ESP_ERROR_CHECK(esp_timer_start_periodic(reconnect_timer, 60000000));
	            	}
	        	}
	            break;

	        default:
	            break;
        }
 
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

		ESP_LOGI(TAG, "IP-Address retrieved");
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        xSemaphoreTake(ip_semaphore, portMAX_DELAY);
        wifi_client_ip = *(&event->ip_info.ip);
        xSemaphoreGive(ip_semaphore);
        xEventGroupSetBits(wifi_status_bits, SBIT_CONNECTED);
	}
}

void Wifi::reconnect_timer_callback(void* arg) {
	if ((xEventGroupGetBits(wifi_status_bits) & SBIT_ACTIVE) != 0) {
		ESP_LOGI(TAG, "Periodic try to reconnect...");
		esp_wifi_connect();
	} else {
		esp_timer_stop(reconnect_timer);
	}
}
