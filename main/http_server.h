#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

//#include <esp_wifi.h>
//#include <esp_event_loop.h>
//#include <esp_log.h>
//#include <esp_system.h>
//#include <nvs_flash.h>
//#include <sys/param.h>
#include <string>
#include <esp_http_server.h>
#include "names.h"

namespace HttpServer {

    using namespace std;

	namespace {

        httpd_handle_t handle;

        esp_err_t whoareyou_handler(httpd_req_t *req);

        httpd_uri_t whoareyou = {
                .uri       = "/whoareyou/?",
                .method    = HTTP_GET,
                .handler   = whoareyou_handler,
                .user_ctx  = NULL
        };
        httpd_uri_t root = {
                .uri       = "/?",
                .method    = HTTP_GET,
                .handler   = whoareyou_handler,
                .user_ctx  = NULL
        };

        esp_err_t position_get_handler(httpd_req_t *req);

        httpd_uri_t position_get = {
                .uri       = "/[0-9]+/pos/?",
                .method    = HTTP_GET,
                .handler   = position_get_handler,
                .user_ctx  = NULL
        };

        esp_err_t position_set_handler(httpd_req_t *req);

        httpd_uri_t position_set = {
                .uri       = "/[0-9]+/[0-9]+/?",
                .method    = HTTP_GET,
                .handler   = position_set_handler,
                .user_ctx  = NULL
        };

        esp_err_t cmd_get_handler(httpd_req_t *req);

        httpd_uri_t cmd_get = {
                .uri       = "/[0-9]+/cmd/?",
                .method    = HTTP_GET,
                .handler   = cmd_get_handler,
                .user_ctx  = NULL
        };

        esp_err_t reboot_handler(httpd_req_t *req);

        httpd_uri_t reboot = {
                .uri       = "/reboot/?",
                .method    = HTTP_GET,
                .handler   = reboot_handler,
                .user_ctx  = NULL
        };

        bool uri_matcher(const char *uri_template, const char *uri_to_match, size_t match_upto);
        inline string get_url(httpd_req_t *req);
        inline void respond_200(httpd_req_t *req, string message);
        inline void respond_500(httpd_req_t *req);
    }

	void start();
    void stop();    

}

#endif //HTTP_SERVER_H_