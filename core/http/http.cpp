#include "http.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cctype>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using hbpl_socket_t = SOCKET;

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using hbpl_socket_t = int;

#endif

// Limites para no aceptar peticiones gigantes
static const size_t HBPL_MAX_HEADER = 64 * 1024;
static const size_t HBPL_MAX_BODY   = 16 * 1024 * 1024;

struct HBPLRequest {
    std::string method;
    std::string path;     // sin query
    std::string query;    // lo que va despues de '?'
    std::string body;
};

struct HBPLResponse {
    hbpl_socket_t socket;
    int status = 200;
    std::string contentType = "text/html; charset=utf-8";
    bool sent = false;
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

static char* hbpl_dup(const std::string& value) {
    char* result = static_cast<char*>(std::malloc(value.size() + 1));

    if (!result)
        return nullptr;

    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

static std::string hbpl_status_text(int status) {

    switch (status) {

        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";

        default:
            return "OK";
    }
}

static std::string hbpl_mime_type(const std::string& path) {

    auto dot = path.find_last_of('.');

    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);

    std::transform(
        ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );

    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css")  return "text/css; charset=utf-8";
    if (ext == "js")   return "application/javascript; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "txt")  return "text/plain; charset=utf-8";
    if (ext == "png")  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif")  return "image/gif";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "webp") return "image/webp";
    if (ext == "pdf")  return "application/pdf";

    return "application/octet-stream";
}

// Envia todo el buffer (send puede enviar menos de lo pedido)
static void hbpl_send_all(hbpl_socket_t socket, const std::string& data) {

    size_t total = 0;

    while (total < data.size()) {

        int sent = static_cast<int>(send(
            socket,
            data.c_str() + total,
            static_cast<int>(data.size() - total),
            0
        ));

        if (sent <= 0)
            return;

        total += static_cast<size_t>(sent);
    }
}

static void hbpl_send_response(
    HBPLResponse* response,
    const std::string& body
) {
    if (response->sent)
        return;

    response->sent = true;

    std::string header =
        "HTTP/1.1 " +
        std::to_string(response->status) +
        " " +
        hbpl_status_text(response->status) +
        "\r\n"
        "Content-Type: " + response->contentType + "\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n";

    hbpl_send_all(response->socket, header + body);
}

static std::string hbpl_lower(std::string s) {
    std::transform(
        s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
    return s;
}

// Lee la peticion completa: cabeceras + cuerpo (segun Content-Length).
// Devuelve 0 si ok, 400/413 si hay error, -1 si el cliente cerro.
static int hbpl_read_request(
    hbpl_socket_t socket,
    HBPLRequest& request
) {
    std::string data;
    char buffer[4096];

    size_t headerEnd = std::string::npos;

    while (headerEnd == std::string::npos) {

        int received = static_cast<int>(
            recv(socket, buffer, sizeof(buffer), 0)
        );

        if (received <= 0)
            return -1;

        data.append(buffer, static_cast<size_t>(received));

        headerEnd = data.find("\r\n\r\n");

        if (headerEnd == std::string::npos && data.size() > HBPL_MAX_HEADER)
            return 413;
    }

    std::string head = data.substr(0, headerEnd);
    std::string body = data.substr(headerEnd + 4);

    // Linea de peticion: METHOD TARGET VERSION
    std::istringstream stream(head);

    std::string method;
    std::string target;
    std::string version;

    stream >> method >> target >> version;

    if (method.empty() || target.empty())
        return 400;

    // Content-Length
    size_t contentLength = 0;

    {
        std::istringstream lines(head);
        std::string line;

        std::getline(lines, line); // linea de peticion

        while (std::getline(lines, line)) {

            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            auto colon = line.find(':');

            if (colon == std::string::npos)
                continue;

            if (hbpl_lower(line.substr(0, colon)) == "content-length") {

                std::string value = line.substr(colon + 1);

                try {
                    contentLength = static_cast<size_t>(std::stoull(value));
                } catch (...) {
                    return 400;
                }
            }
        }
    }

    if (contentLength > HBPL_MAX_BODY)
        return 413;

    while (body.size() < contentLength) {

        int received = static_cast<int>(
            recv(socket, buffer, sizeof(buffer), 0)
        );

        if (received <= 0)
            return -1;

        body.append(buffer, static_cast<size_t>(received));
    }

    body.resize(contentLength);

    // Separar ruta y query
    std::string path = target;
    std::string query;

    auto question = target.find('?');

    if (question != std::string::npos) {
        path = target.substr(0, question);
        query = target.substr(question + 1);
    }

    request.method = method;
    request.path = path;
    request.query = query;
    request.body = body;

    return 0;
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

        // El lambda ya captura server y clientSocket:
        // std::thread no necesita argumentos extra.
        std::thread(
            [server, clientSocket]() {

                HBPLRequest request;
                HBPLResponse response;

                response.socket = clientSocket;

                int status = hbpl_read_request(clientSocket, request);

                if (status == -1) {
                    hbpl_close_socket(clientSocket);
                    return;
                }

                if (status != 0) {
                    response.status = status;

                    hbpl_send_response(
                        &response,
                        "<h1>" + std::to_string(status) + " " +
                        hbpl_status_text(status) + "</h1>"
                    );

                    hbpl_close_socket(clientSocket);
                    return;
                }

                bool pathFound = false;
                bool handled = false;

                for (const auto& route : server->routes) {

                    if (route.path != request.path)
                        continue;

                    pathFound = true;

                    if (route.method != request.method)
                        continue;

                    handled = true;

                    route.handler(
                        &request,
                        &response
                    );

                    break;
                }

                if (!handled) {

                    // ruta existe pero con otro metodo -> 405
                    response.status = pathFound ? 405 : 404;
                    response.contentType = "text/html; charset=utf-8";

                    hbpl_send_response(
                        &response,
                        pathFound
                            ? "<h1>405 Method Not Allowed</h1>"
                            : "<h1>404 Not Found</h1>"
                    );
                }
                else if (!response.sent) {

                    // el handler no respondio: evitar dejar colgado al cliente
                    response.status = 204;
                    hbpl_send_response(&response, "");
                }

                hbpl_close_socket(
                    clientSocket
                );

            }
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

// ------------------------------------------------------------
// REQUEST
// ------------------------------------------------------------

char* hbpl_http_request_method(void* requestPtr) {
    if (!requestPtr)
        return hbpl_dup("");

    return hbpl_dup(static_cast<HBPLRequest*>(requestPtr)->method);
}

char* hbpl_http_request_path(void* requestPtr) {
    if (!requestPtr)
        return hbpl_dup("");

    return hbpl_dup(static_cast<HBPLRequest*>(requestPtr)->path);
}

char* hbpl_http_request_query(void* requestPtr) {
    if (!requestPtr)
        return hbpl_dup("");

    return hbpl_dup(static_cast<HBPLRequest*>(requestPtr)->query);
}

char* hbpl_http_request_body(void* requestPtr) {
    if (!requestPtr)
        return hbpl_dup("");

    return hbpl_dup(static_cast<HBPLRequest*>(requestPtr)->body);
}

// ------------------------------------------------------------
// RESPONSE
// ------------------------------------------------------------

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

void hbpl_http_response_type(
    void* responsePtr,
    const char* contentType
) {
    if (!responsePtr || !contentType)
        return;

    HBPLResponse* response =
        static_cast<HBPLResponse*>(responsePtr);

    response->contentType = contentType;
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
        response->contentType = "text/html; charset=utf-8";

        hbpl_send_response(
            response,
            "<h1>404 File Not Found</h1>"
        );

        return;
    }

    std::ostringstream content;

    content << file.rdbuf();

    response->contentType = hbpl_mime_type(path);

    hbpl_send_response(
        response,
        content.str()
    );
}

}
