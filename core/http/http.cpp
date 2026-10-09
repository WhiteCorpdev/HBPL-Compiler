#include "http.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using hbpl_socket_t = SOCKET;

#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using hbpl_socket_t = int;

#endif

struct HBPLRequest {
    std::string method;
    std::string path;
    std::string body;
};

struct HBPLResponse {
    hbpl_socket_t socket;
    int status = 200;
};

struct HBPLRoute {
    std::string method;
    std::string path;

    void (*handler)(
        void* request,
        void* response
    );
};

struct HBPLHTTPServer {
    hbpl_socket_t socket;
    int port = 0;
    bool running = false;

    std::vector<HBPLRoute> routes;
};

static void hbpl_close_socket(hbpl_socket_t socket) {

#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif

}

static std::string hbpl_status_text(int status) {

    switch (status) {

        case 200:
            return "OK";

        case 201:
            return "Created";

        case 400:
            return "Bad Request";

        case 404:
            return "Not Found";

        case 500:
            return "Internal Server Error";

        default:
            return "OK";
    }
}

static void hbpl_send_response(
    HBPLResponse* response,
    const std::string& body
) {
    std::string header =
        "HTTP/1.1 " +
        std::to_string(response->status) +
        " " +
        hbpl_status_text(response->status) +
        "\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n";

    std::string result = header + body;

    send(
        response->socket,
        result.c_str(),
        static_cast<int>(result.size()),
        0
    );
}

extern "C" {

void* hbpl_http_create() {

#ifdef _WIN32

    WSADATA data;

    WSAStartup(
        MAKEWORD(2, 2),
        &data
    );

#endif

    HBPLHTTPServer* server =
        new HBPLHTTPServer;

    server->socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    return server;
}

bool hbpl_http_listen(
    void* serverPtr,
    int port
) {
    if (!serverPtr)
        return false;

    HBPLHTTPServer* server =
        static_cast<HBPLHTTPServer*>(serverPtr);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(
        static_cast<unsigned short>(port)
    );

    int yes = 1;

#ifdef _WIN32

    setsockopt(
        server->socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&yes),
        sizeof(yes)
    );

#else

    setsockopt(
        server->socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &yes,
        sizeof(yes)
    );

#endif

    if (bind(
        server->socket,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    ) != 0) {
        return false;
    }

    if (listen(server->socket, 16) != 0)
        return false;

    server->port = port;

    return true;
}

void hbpl_http_start(void* serverPtr) {

    if (!serverPtr)
        return;

    HBPLHTTPServer* server =
        static_cast<HBPLHTTPServer*>(serverPtr);

    server->running = true;

    while (server->running) {

        sockaddr_in client{};

#ifdef _WIN32

        int clientSize = sizeof(client);

#else

        socklen_t clientSize = sizeof(client);

#endif

        hbpl_socket_t clientSocket =
            accept(
                server->socket,
                reinterpret_cast<sockaddr*>(&client),
                &clientSize
            );

#ifdef _WIN32

        if (clientSocket == INVALID_SOCKET)
            continue;

#else

        if (clientSocket < 0)
            continue;

#endif

        std::thread(
            [server, clientSocket]() {

                char buffer[16384];

                int received = recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (received <= 0) {
                    hbpl_close_socket(clientSocket);
                    return;
                }

                buffer[received] = '\0';

                std::string requestText(buffer);

                std::istringstream stream(
                    requestText
                );

                std::string method;
                std::string path;
                std::string version;

                stream >>
                    method >>
                    path >>
                    version;

                HBPLRequest request;

                request.method = method;
                request.path = path;

                HBPLResponse response;

                response.socket = clientSocket;

                bool found = false;

                for (const auto& route :
                     server->routes) {

                    if (
                        route.method == method &&
                        route.path == path
                    ) {

                        found = true;

                        route.handler(
                            &request,
                            &response
                        );

                        break;
                    }
                }

                if (!found) {
                    response.status = 404;

                    hbpl_send_response(
                        &response,
                        "<h1>404 Not Found</h1>"
                    );
                }

                hbpl_close_socket(
                    clientSocket
                );

            },
            server,
            clientSocket
        ).detach();
    }
}

void hbpl_http_stop(void* serverPtr) {

    if (!serverPtr)
        return;

    HBPLHTTPServer* server =
        static_cast<HBPLHTTPServer*>(serverPtr);

    server->running = false;

    hbpl_close_socket(server->socket);

    delete server;
}

void hbpl_http_get(
    void* serverPtr,
    const char* path,
    void (*handler)(
        void* request,
        void* response
    )
) {
    if (!serverPtr || !path || !handler)
        return;

    HBPLHTTPServer* server =
        static_cast<HBPLHTTPServer*>(serverPtr);

    server->routes.push_back({
        "GET",
        path,
        handler
    });
}

void hbpl_http_post(
    void* serverPtr,
    const char* path,
    void (*handler)(
        void* request,
        void* response
    )
) {
    if (!serverPtr || !path || !handler)
        return;

    HBPLHTTPServer* server =
        static_cast<HBPLHTTPServer*>(serverPtr);

    server->routes.push_back({
        "POST",
        path,
        handler
    });
}

void hbpl_http_response_status(
    void* responsePtr,
    int status
) {
    if (!responsePtr)
        return;

    HBPLResponse* response =
        static_cast<HBPLResponse*>(responsePtr);

    response->status = status;
}

void hbpl_http_response_send(
    void* responsePtr,
    const char* data
) {
    if (!responsePtr)
        return;

    HBPLResponse* response =
        static_cast<HBPLResponse*>(responsePtr);

    hbpl_send_response(
        response,
        data ? data : ""
    );
}

void hbpl_http_response_sendfile(
    void* responsePtr,
    const char* path
) {
    if (!responsePtr || !path)
        return;

    HBPLResponse* response =
        static_cast<HBPLResponse*>(responsePtr);

    std::ifstream file(
        path,
        std::ios::binary
    );

    if (!file) {

        response->status = 404;

        hbpl_send_response(
            response,
            "<h1>404 File Not Found</h1>"
        );

        return;
    }

    std::ostringstream content;

    content << file.rdbuf();

    hbpl_send_response(
        response,
        content.str()
    );
}

}
