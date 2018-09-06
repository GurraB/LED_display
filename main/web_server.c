#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_event_loop.h"
#include "web_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "http_server.h"
#include <sys/param.h>
#include "esp_log.h"

#define WEB_SERVER_TAG "WEB_SERVER"

void (*ota_cb)(char* address);
void (*ota_renew_pem_cb)(char* pem);

httpd_handle_t server = NULL;

httpd_uri_t uri_index = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = index_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_ota = {
    .uri      = "/ota_update",
    .method   = HTTP_POST,
    .handler  = ota_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_pem = {
    .uri      = "/ota_renew_pem",
    .method   = HTTP_POST,
    .handler  = ota_renew_pem_handler,
    .user_ctx = NULL
};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
    .uri      = "/uri",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

esp_err_t index_handler(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = INDEX_RESPONSE;
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req)
{
    /* Read request content */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret < 0) {
        /* In case of recv error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

esp_err_t ota_handler(httpd_req_t *req)
{
    /* Read request content */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret < 0) {
        /* In case of recv error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    httpd_resp_send(req, content, recv_size);
    //set_update_url(content);
    printf("OTA_ENDPOINT, address: %s\n", content);
    ota_cb(content);
    return ESP_OK;
}

void ota_callback_register(void (*ota_callback)(char* address)) {
    ota_cb = ota_callback;
}

esp_err_t ota_renew_pem_handler(httpd_req_t *req)
{
    /* Read request content */
    ESP_LOGI(WEB_SERVER_TAG, "inb4 allocate content");
    char content[1536];
    ESP_LOGI(WEB_SERVER_TAG, "after allocate content");

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret < 0) {
        /* In case of recv error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    /* Send a simple response */
    httpd_resp_send(req, content, recv_size);
    printf("OTA_UPDATE_PEM\n%s\n", content);
    printf("size: %d\n", recv_size);
    ota_renew_pem_cb(content);
    return ESP_OK;
}

void ota_renew_pem_callback_register(void (*ota_renew_pem_callback)(char* pem)) {
    ota_renew_pem_cb = ota_renew_pem_callback;
}

esp_err_t stop_webserver()
{
    if (server) {
        httpd_stop(server);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t start_web_server(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(WEB_SERVER_TAG, "Starting server on port: %d", config.server_port);

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_index);
        httpd_register_uri_handler(server, &uri_ota);
        httpd_register_uri_handler(server, &uri_pem);
        httpd_register_uri_handler(server, &uri_post);
    }
    // return whether it was a success
    esp_err_t status;
    if(server == NULL) {
        status = ESP_FAIL;
    } else {
        status = ESP_OK;
        ESP_LOGI(WEB_SERVER_TAG, "Web server started");
    }
    return status;
}