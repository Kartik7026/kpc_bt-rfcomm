#define TARGET_SERVICE_UUID "4de17a00-52cb-11e6-bdf4-0800200c9a66"
#define POLL_INTERVAL_SECONDS 2

#include "BluetoothConnector.h"

#include <cstdio>
#include <unistd.h>

int main() {
    std::printf("Waiting for a connected Bluetooth device advertising service %s\n", TARGET_SERVICE_UUID);

    ConnectedDevice targetDevice;
    int channel = -1;

    while (channel <= 0) {
        std::vector<ConnectedDevice> devices = BluetoothConnector::GetConnectedDevices();

        for (const ConnectedDevice &device : devices) {
            int foundChannel = BluetoothConnector::FindRfcommChannel(device.address, TARGET_SERVICE_UUID);
            if (foundChannel > 0) {
                targetDevice = device;
                channel = foundChannel;
                break;
            }
        }

        if (channel <= 0) sleep(POLL_INTERVAL_SECONDS);
    }

    std::printf("Found RFCOMM channel %d on %s\n", channel, targetDevice.addressString.c_str());

    int sock = BluetoothConnector::ConnectRfcomm(targetDevice.address, channel);
    if (sock < 0) {
        std::fprintf(stderr, "Failed to connect to %s on channel %d\n", targetDevice.addressString.c_str(), channel);
        return 1;
    }

    std::printf("Connected to %s on RFCOMM channel %d\n", targetDevice.addressString.c_str(), channel);

    char buffer[1024];
    while (true) {
        ssize_t bytesRead = read(sock, buffer, sizeof(buffer));
        if (bytesRead <= 0) break;
        std::fwrite(buffer, 1, static_cast<size_t>(bytesRead), stdout);
        std::fflush(stdout);
    }

    close(sock);
    return 0;
}
