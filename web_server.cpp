#include "ApiServer.h"

int main() {
    ApiServer server(3000);
    return server.start() ? 0 : 1;
}
