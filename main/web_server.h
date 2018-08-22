#ifndef web_server_h
#define web_server_h

#include "http_server.h"

typedef void (*ota_callback_t)(char* address);

#define INDEX_RESPONSE "Hello!"

esp_err_t index_handler(httpd_req_t* req);

esp_err_t post_handler(httpd_req_t* req);

esp_err_t ota_handler(httpd_req_t* req);

void ota_callback_register(void (*ota_callback)(char* address));

esp_err_t ota_renew_pem_handler(httpd_req_t* req);

void ota_renew_pem_callback_register(void (*ota_renew_pem_callback)(char* pem));

esp_err_t stop_webserver(void);

esp_err_t start_web_server(void);

#endif