#include "ApiServer.h"
#include <cstdlib>   // for getenv
#include <iostream>

int main() {
    int port = 3000;

    char* env_port = getenv("PORT");
    if (env_port) {
        port = atoi(env_port);
    }

    std::cout << "Starting server on port: " << port << std::endl;

    ApiServer server(port);
    return server.start() ? 0 : 1;
}