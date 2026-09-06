#define TARGET_SERVICE_UUID "4de17a00-52cb-11e6-bdf4-0800200c9a66"
#define RFCOMM_CHANNEL 1
#define PROFILE_NAME "RfcommServerProfile"

#include "BluetoothServer.h"

#include <cstdio>
#include <unistd.h>

int main() {
    if (!BluetoothServer::RegisterProfile(TARGET_SERVICE_UUID, RFCOMM_CHANNEL, PROFILE_NAME)) {
        std::fprintf(stderr, "Failed to register BlueZ profile\n");
        return 1;
    }

    std::printf("Advertising service %s on RFCOMM channel %d\n", TARGET_SERVICE_UUID, RFCOMM_CHANNEL);
    std::printf("Waiting for a client connection\n");

    std::string deviceAddress;
    int clientSocket = BluetoothServer::WaitForConnection(deviceAddress);
    if (clientSocket < 0) {
        std::fprintf(stderr, "Failed to accept client connection\n");
        BluetoothServer::Shutdown();
        return 1;
    }

    std::printf("Client %s connected\n", deviceAddress.c_str());

    char buffer[1024];
    while (true) {
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) break;
        std::fwrite(buffer, 1, static_cast<size_t>(bytesRead), stdout);
        std::fflush(stdout);
    }

    close(clientSocket);
    BluetoothServer::Shutdown();
    return 0;
}
