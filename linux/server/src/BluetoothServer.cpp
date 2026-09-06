#include "BluetoothServer.h"

#include <unistd.h>

#include <dbus/dbus.h>

namespace {

const char *kObjectPath = "/rfcommserver/profile";

DBusConnection *g_connection = nullptr;
int g_connectionFd = -1;
std::string g_devicePath;
bool g_connectionReceived = false;

std::string ExtractAddressFromDevicePath(const std::string &devicePath) {
    size_t pos = devicePath.rfind("dev_");
    if (pos == std::string::npos) return devicePath;

    std::string address = devicePath.substr(pos + 4);
    for (char &c : address) {
        if (c == '_') c = ':';
    }
    return address;
}

DBusHandlerResult MessageFilter(DBusConnection *connection, DBusMessage *message, void *) {
    if (dbus_message_is_method_call(message, "org.bluez.Profile1", "NewConnection")) {
        DBusMessageIter args;
        dbus_message_iter_init(message, &args);

        const char *devicePath = nullptr;
        dbus_message_iter_get_basic(&args, &devicePath);
        dbus_message_iter_next(&args);

        int fd = -1;
        dbus_message_iter_get_basic(&args, &fd);

        g_devicePath = devicePath;
        g_connectionFd = dup(fd);
        g_connectionReceived = true;

        DBusMessage *reply = dbus_message_new_method_return(message);
        dbus_connection_send(connection, reply, nullptr);
        dbus_message_unref(reply);

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_method_call(message, "org.bluez.Profile1", "Release") ||
        dbus_message_is_method_call(message, "org.bluez.Profile1", "RequestDisconnection")) {
        DBusMessage *reply = dbus_message_new_method_return(message);
        dbus_connection_send(connection, reply, nullptr);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void AppendStringOption(DBusMessageIter *optionsIter, const char *key, const char *value) {
    DBusMessageIter entry;
    dbus_message_iter_open_container(optionsIter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

    DBusMessageIter variant;
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);

    dbus_message_iter_close_container(optionsIter, &entry);
}

void AppendUint16Option(DBusMessageIter *optionsIter, const char *key, uint16_t value) {
    DBusMessageIter entry;
    dbus_message_iter_open_container(optionsIter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

    DBusMessageIter variant;
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "q", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_UINT16, &value);
    dbus_message_iter_close_container(&entry, &variant);

    dbus_message_iter_close_container(optionsIter, &entry);
}

}

bool BluetoothServer::RegisterProfile(const std::string &uuidString, uint16_t channel, const std::string &profileName) {
    DBusError error;
    dbus_error_init(&error);

    g_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (!g_connection) {
        dbus_error_free(&error);
        return false;
    }

    dbus_connection_add_filter(g_connection, MessageFilter, nullptr, nullptr);

    DBusMessage *call = dbus_message_new_method_call("org.bluez", "/org/bluez", "org.bluez.ProfileManager1", "RegisterProfile");

    DBusMessageIter args;
    dbus_message_iter_init_append(call, &args);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_OBJECT_PATH, &kObjectPath);

    const char *uuidCStr = uuidString.c_str();
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &uuidCStr);

    DBusMessageIter optionsIter;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &optionsIter);
    AppendStringOption(&optionsIter, "Name", profileName.c_str());
    AppendUint16Option(&optionsIter, "Channel", channel);
    dbus_message_iter_close_container(&args, &optionsIter);

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(g_connection, call, -1, &error);
    dbus_message_unref(call);

    if (!reply) {
        dbus_error_free(&error);
        return false;
    }

    dbus_message_unref(reply);
    return true;
}

int BluetoothServer::WaitForConnection(std::string &deviceAddress) {
    g_connectionReceived = false;

    while (!g_connectionReceived) {
        dbus_connection_read_write_dispatch(g_connection, 100);
    }

    deviceAddress = ExtractAddressFromDevicePath(g_devicePath);
    return g_connectionFd;
}

void BluetoothServer::Shutdown() {
    if (g_connection) {
        dbus_connection_unref(g_connection);
        g_connection = nullptr;
    }
}
