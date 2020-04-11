/*****************************************************************************/
//							Splitflap Control								 //
/*****************************************************************************/

#include "globals.h"

#include <rom/uart.h>
#include <driver/gpio.h>

#include "nvs.h"
#include "splitflap_task.h"
#include "wifi.h"
#include "blue.h"
#include "http_server.h"
#include "mdns.h"

extern "C" {

	void app_main()
	{
		Nvs::setup();

		gpio_install_isr_service(0);

		SplitflapTask::setup();

		ESP_ERROR_CHECK(esp_event_loop_create_default()); // needed for mdns
        start_mdns_service();

		Wifi::setup();
		Blue::setup();
		HttpServer::start();

        #if OPTION_UART_INPUT
		std::string buffer;
		while (true) {
			uint8_t ch;
			STATUS s = uart_rx_one_char(&ch);
			if (s == OK) {
				buffer += ch;

				if (ch == '\n') {
					string response = Comx::interpret(buffer);
					buffer.clear();
					printf("%s", response.c_str());
				}
			}

			vTaskDelay(1);
		}
        #endif
		
	}
}
