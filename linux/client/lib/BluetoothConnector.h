#pragma once

#include <string>
#include <vector>

#include <bluetooth/bluetooth.h>

struct ConnectedDevice {
    bdaddr_t address;
    std::string addressString;
};

class BluetoothConnector {
public:
    static std::vector<ConnectedDevice> GetConnectedDevices();
    static int FindRfcommChannel(const bdaddr_t &deviceAddress, const std::string &uuidString);
    static int ConnectRfcomm(const bdaddr_t &deviceAddress, int channel);
};
