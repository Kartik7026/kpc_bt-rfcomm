#pragma once

#include <cstdint>
#include <string>

class BluetoothServer {
public:
    static bool RegisterProfile(const std::string &uuidString, uint16_t channel, const std::string &profileName);
    static int WaitForConnection(std::string &deviceAddress);
    static void Shutdown();
};
