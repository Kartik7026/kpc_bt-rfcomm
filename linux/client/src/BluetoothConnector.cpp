#include "BluetoothConnector.h"

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

namespace {

constexpr int kMaxConnections = 64;

bool ParseUuid128(const std::string &uuidString, uuid_t &uuid) {
    std::string hex;
    hex.reserve(32);
    for (char c : uuidString) {
        if (c == '-') continue;
        hex.push_back(c);
    }
    if (hex.size() != 32) return false;

    uint8_t data[16];
    for (int i = 0; i < 16; ++i) {
        std::string byteStr = hex.substr(static_cast<size_t>(i) * 2, 2);
        data[i] = static_cast<uint8_t>(std::strtoul(byteStr.c_str(), nullptr, 16));
    }
    sdp_uuid128_create(&uuid, data);
    return true;
}

}

std::vector<ConnectedDevice> BluetoothConnector::GetConnectedDevices() {
    std::vector<ConnectedDevice> devices;

    int deviceId = hci_get_route(nullptr);
    if (deviceId < 0) return devices;

    int sock = hci_open_dev(deviceId);
    if (sock < 0) return devices;

    size_t bufferSize = sizeof(hci_conn_list_req) + kMaxConnections * sizeof(hci_conn_info);
    hci_conn_list_req *connList = reinterpret_cast<hci_conn_list_req *>(std::malloc(bufferSize));
    connList->dev_id = static_cast<uint16_t>(deviceId);
    connList->conn_num = kMaxConnections;

    if (ioctl(sock, HCIGETCONNLIST, connList) < 0) {
        std::free(connList);
        close(sock);
        return devices;
    }

    for (int i = 0; i < connList->conn_num; ++i) {
        ConnectedDevice device;
        device.address = connList->conn_info[i].bdaddr;
        char addrStr[19];
        ba2str(&device.address, addrStr);
        device.addressString = addrStr;
        devices.push_back(device);
    }

    std::free(connList);
    close(sock);
    return devices;
}

int BluetoothConnector::FindRfcommChannel(const bdaddr_t &deviceAddress, const std::string &uuidString) {
    uuid_t serviceUuid;
    if (!ParseUuid128(uuidString, serviceUuid)) return -1;

    sdp_session_t *session = sdp_connect(BDADDR_ANY, &deviceAddress, SDP_RETRY_IF_BUSY);
    if (!session) return -1;

    sdp_list_t *searchList = sdp_list_append(nullptr, &serviceUuid);
    uint32_t range = 0x0000ffff;
    sdp_list_t *attrList = sdp_list_append(nullptr, &range);
    sdp_list_t *responseList = nullptr;

    int channel = -1;
    int result = sdp_service_search_attr_req(session, searchList, SDP_ATTR_REQ_RANGE, attrList, &responseList);

    if (result == 0) {
        for (sdp_list_t *r = responseList; r; r = r->next) {
            sdp_record_t *record = reinterpret_cast<sdp_record_t *>(r->data);
            sdp_list_t *protoList = nullptr;
            if (sdp_get_access_protos(record, &protoList) == 0) {
                int foundChannel = sdp_get_proto_port(protoList, RFCOMM_UUID);
                if (foundChannel > 0) channel = foundChannel;
                sdp_list_free(protoList, nullptr);
            }
            if (channel > 0) break;
        }
    }

    sdp_list_free(responseList, reinterpret_cast<sdp_free_func_t>(sdp_record_free));
    sdp_list_free(searchList, nullptr);
    sdp_list_free(attrList, nullptr);
    sdp_close(session);

    return channel;
}

int BluetoothConnector::ConnectRfcomm(const bdaddr_t &deviceAddress, int channel) {
    int sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock < 0) return -1;

    sockaddr_rc address{};
    address.rc_family = AF_BLUETOOTH;
    address.rc_channel = static_cast<uint8_t>(channel);
    address.rc_bdaddr = deviceAddress;

    if (connect(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}
