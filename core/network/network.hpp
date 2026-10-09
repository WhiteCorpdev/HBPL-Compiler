#pragma once

extern "C" {

void* hbpl_tcp_create();

bool hbpl_tcp_connect(
    void* socket,
    const char* host,
    int port
);

int hbpl_tcp_send(
    void* socket,
    const char* data,
    int size
);

int hbpl_tcp_receive(
    void* socket,
    char* buffer,
    int size
);

void hbpl_tcp_close(
    void* socket
);

}
