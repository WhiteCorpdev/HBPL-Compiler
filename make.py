from pathlib import Path

ROOT = Path(__file__).resolve().parent
CORE = ROOT / "core"

FILES = {
    "math/math.hpp": r'''
#pragma once

extern "C" {

double hbpl_math_abs(double x);
double hbpl_math_sqrt(double x);
double hbpl_math_pow(double x, double y);

double hbpl_math_sin(double x);
double hbpl_math_cos(double x);
double hbpl_math_tan(double x);

double hbpl_math_asin(double x);
double hbpl_math_acos(double x);
double hbpl_math_atan(double x);

double hbpl_math_floor(double x);
double hbpl_math_ceil(double x);
double hbpl_math_round(double x);

double hbpl_math_min(double a, double b);
double hbpl_math_max(double a, double b);
double hbpl_math_clamp(double x, double min, double max);

double hbpl_math_log(double x);
double hbpl_math_log10(double x);
double hbpl_math_exp(double x);

double hbpl_math_pi();
double hbpl_math_e();

double hbpl_math_random();
double hbpl_math_random_range(double min, double max);

}
''',

    "math/math.cpp": r'''
#include "math.hpp"

#include <cmath>
#include <random>
#include <algorithm>

extern "C" {

double hbpl_math_abs(double x) {
    return std::abs(x);
}

double hbpl_math_sqrt(double x) {
    return std::sqrt(x);
}

double hbpl_math_pow(double x, double y) {
    return std::pow(x, y);
}

double hbpl_math_sin(double x) {
    return std::sin(x);
}

double hbpl_math_cos(double x) {
    return std::cos(x);
}

double hbpl_math_tan(double x) {
    return std::tan(x);
}

double hbpl_math_asin(double x) {
    return std::asin(x);
}

double hbpl_math_acos(double x) {
    return std::acos(x);
}

double hbpl_math_atan(double x) {
    return std::atan(x);
}

double hbpl_math_floor(double x) {
    return std::floor(x);
}

double hbpl_math_ceil(double x) {
    return std::ceil(x);
}

double hbpl_math_round(double x) {
    return std::round(x);
}

double hbpl_math_min(double a, double b) {
    return std::min(a, b);
}

double hbpl_math_max(double a, double b) {
    return std::max(a, b);
}

double hbpl_math_clamp(double x, double min, double max) {
    return std::clamp(x, min, max);
}

double hbpl_math_log(double x) {
    return std::log(x);
}

double hbpl_math_log10(double x) {
    return std::log10(x);
}

double hbpl_math_exp(double x) {
    return std::exp(x);
}

double hbpl_math_pi() {
    return 3.14159265358979323846;
}

double hbpl_math_e() {
    return 2.71828182845904523536;
}

double hbpl_math_random() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);

    return dist(gen);
}

double hbpl_math_random_range(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<double> dist(min, max);

    return dist(gen);
}

}
''',

    "path/path.hpp": r'''
#pragma once

extern "C" {

char* hbpl_path_current_dir();

char* hbpl_path_join(
    const char* a,
    const char* b
);

bool hbpl_path_exists(
    const char* path
);

bool hbpl_path_is_file(
    const char* path
);

bool hbpl_path_is_dir(
    const char* path
);

char* hbpl_path_filename(
    const char* path
);

char* hbpl_path_extension(
    const char* path
);

char* hbpl_path_parent(
    const char* path
);

}
''',

    "path/path.cpp": r'''
#include "path.hpp"

#include <filesystem>
#include <string>
#include <cstring>
#include <cstdlib>

namespace fs = std::filesystem;

static char* hbpl_make_string(const std::string& value) {
    char* result =
        static_cast<char*>(std::malloc(value.size() + 1));

    if (!result)
        return nullptr;

    std::memcpy(
        result,
        value.c_str(),
        value.size() + 1
    );

    return result;
}

extern "C" {

char* hbpl_path_current_dir() {
    return hbpl_make_string(
        fs::current_path().string()
    );
}

char* hbpl_path_join(
    const char* a,
    const char* b
) {
    if (!a || !b)
        return nullptr;

    fs::path result = fs::path(a) / fs::path(b);

    return hbpl_make_string(
        result.lexically_normal().string()
    );
}

bool hbpl_path_exists(const char* path) {
    if (!path)
        return false;

    return fs::exists(path);
}

bool hbpl_path_is_file(const char* path) {
    if (!path)
        return false;

    return fs::is_regular_file(path);
}

bool hbpl_path_is_dir(const char* path) {
    if (!path)
        return false;

    return fs::is_directory(path);
}

char* hbpl_path_filename(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).filename().string()
    );
}

char* hbpl_path_extension(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).extension().string()
    );
}

char* hbpl_path_parent(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).parent_path().string()
    );
}

}
''',

    "network/network.hpp": r'''
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
''',

    "network/network.cpp": r'''
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
''',

    "http/http.hpp": r'''
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

void hbpl_http_response_status(
    void* response,
    int status
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
''',

    "http/http.cpp": r'''
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
''',
}


def create_files():
    print("Creando HBPL Core...\n")

    for relative_path, content in FILES.items():
        path = CORE / relative_path

        path.parent.mkdir(
            parents=True,
            exist_ok=True
        )

        path.write_text(
            content.strip() + "\n",
            encoding="utf-8"
        )

        print(f"[OK] {path.relative_to(ROOT)}")

    print("\nHBPL Core creado correctamente.")
    print("\nEstructura:")

    for path in sorted(CORE.rglob("*")):
        if path.is_file():
            print("  ", path.relative_to(ROOT))


if __name__ == "__main__":
    create_files()
