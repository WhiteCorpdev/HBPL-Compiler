#include "network.hpp"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct HBPLSocket {
    SOCKET socket;
};

static bool hbpl_network_init() {
    static bool initialized = false;

    if (initialized)
        return true;

    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return false;

    initialized = true;
    return true;
}

extern "C" {

void* hbpl_tcp_create() {
    if (!hbpl_network_init())
        return nullptr;

    HBPLSocket* result = new HBPLSocket;

    result->socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (result->socket == INVALID_SOCKET) {
        delete result;
        return nullptr;
    }

    return result;
}

bool hbpl_tcp_connect(
    void* socketPtr,
    const char* host,
    int port
) {
    if (!socketPtr || !host)
        return false;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(
        static_cast<u_short>(port)
    );

    if (inet_pton(
        AF_INET,
        host,
        &address.sin_addr
    ) != 1) {
        return false;
    }

    return connect(
        s->socket,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    ) == 0;
}

int hbpl_tcp_send(
    void* socketPtr,
    const char* data,
    int size
) {
    if (!socketPtr || !data || size <= 0)
        return -1;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    return send(
        s->socket,
        data,
        size,
        0
    );
}

int hbpl_tcp_receive(
    void* socketPtr,
    char* buffer,
    int size
) {
    if (!socketPtr || !buffer || size <= 0)
        return -1;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    return recv(
        s->socket,
        buffer,
        size,
        0
    );
}

void hbpl_tcp_close(void* socketPtr) {
    if (!socketPtr)
        return;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    closesocket(s->socket);

    delete s;
}

}

#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>

struct HBPLSocket {
    int socket;
};

extern "C" {

void* hbpl_tcp_create() {
    HBPLSocket* result = new HBPLSocket;

    result->socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (result->socket < 0) {
        delete result;
        return nullptr;
    }

    return result;
}

bool hbpl_tcp_connect(
    void* socketPtr,
    const char* host,
    int port
) {
    if (!socketPtr || !host)
        return false;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(
        AF_INET,
        host,
        &address.sin_addr
    ) != 1) {
        return false;
    }

    return connect(
        s->socket,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    ) == 0;
}

int hbpl_tcp_send(
    void* socketPtr,
    const char* data,
    int size
) {
    if (!socketPtr || !data)
        return -1;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    return send(
        s->socket,
        data,
        size,
        0
    );
}

int hbpl_tcp_receive(
    void* socketPtr,
    char* buffer,
    int size
) {
    if (!socketPtr || !buffer)
        return -1;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    return recv(
        s->socket,
        buffer,
        size,
        0
    );
}

void hbpl_tcp_close(void* socketPtr) {
    if (!socketPtr)
        return;

    HBPLSocket* s =
        static_cast<HBPLSocket*>(socketPtr);

    close(s->socket);

    delete s;
}

}

#endif
