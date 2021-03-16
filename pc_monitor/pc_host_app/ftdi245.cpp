#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include "ftd2xx.h"

FT_HANDLE init_ft245_mode()
{

    FT_HANDLE handle;

    // check how many FTDI devices are attached to this PC
    uint32_t deviceCount = 0;
    if (FT_CreateDeviceInfoList(&deviceCount) != FT_OK)
    {
        printf("Unable to query devices. Exiting.\r\n");
        exit(EXIT_FAILURE);
    }

    // get a list of information about each FTDI device
    FT_DEVICE_LIST_INFO_NODE *deviceInfo = (FT_DEVICE_LIST_INFO_NODE *)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * deviceCount);
    if (FT_GetDeviceInfoList(deviceInfo, &deviceCount) != FT_OK)
    {
        printf("Unable to get the list of info. Exiting.\r\n");
        exit(EXIT_FAILURE);
    }

    // print the list of information
    for (unsigned long i = 0; i < deviceCount; i++)
    {

        printf("Device = %d\r\n", i);
        printf("Flags = 0x%X\r\n", deviceInfo[i].Flags);
        printf("Type = 0x%X\r\n", deviceInfo[i].Type);
        printf("ID = 0x%X\r\n", deviceInfo[i].ID);
        printf("LocId = 0x%X\r\n", deviceInfo[i].LocId);
        printf("SN = %s\r\n", deviceInfo[i].SerialNumber);
        printf("Description = %s\r\n", deviceInfo[i].Description);
        printf("Handle = 0x%X\r\n", deviceInfo[i].ftHandle);
        printf("\r\n");

        if (FT_OpenEx(deviceInfo[i].SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &handle) == FT_OK &&
            FT_SetBitMode(handle, 0x0, 0x40) == FT_OK &&
            FT_SetLatencyTimer(handle, 2) == FT_OK &&
            FT_SetUSBParameters(handle, 65536, 65536) == FT_OK &&
            FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0) == FT_OK &&
            FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX) == FT_OK &&
            FT_SetTimeouts(handle, 10000, 10000) == FT_OK)
        {
            return deviceInfo[i].ftHandle;
        }
        else
            printf("error getting handle for ft232h\n");
    }
}
