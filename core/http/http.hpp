#pragma once

extern "C" {

void* hbpl_http_create();

bool hbpl_http_listen(
    void* server,
    int port
);

void hbpl_http_start(
    void* server
);

void hbpl_http_stop(
    void* server
);

void hbpl_http_get(
    void* server,
    const char* path,
    void (*handler)(void* request, void* response)
);

void hbpl_http_post(
    void* server,
    const char* path,
    void (*handler)(void* request, void* response)
);

// ---- request: devuelven memoria malloc'd (liberar con free) ----
char* hbpl_http_request_method(void* request);
char* hbpl_http_request_path(void* request);
char* hbpl_http_request_query(void* request);
char* hbpl_http_request_body(void* request);

// ---- response ----
void hbpl_http_response_status(
    void* response,
    int status
);

void hbpl_http_response_type(
    void* response,
    const char* contentType
);

void hbpl_http_response_send(
    void* response,
    const char* data
);

void hbpl_http_response_sendfile(
    void* response,
    const char* path
);

}
