// ICNC2.cpp : implementation file
//


#include "stdafx.h"
#include "stdlib.h"

#include<TCHAR.H> // Implicit or explicit include

#include <stdio.h>
#include <sysinfoapi.h>

//#include <windows.h>
//#include <string.h> 
#include <setupapi.h>
#include <initguid.h>
#include <winioctl.h>
#include "ioctls.h"
#include "USBCom.h"
#include "ICNC2.h"
#include "CKEtimer.h"

#include <assert.h>
#include <math.h>

#include <winusb.h>


#define USING_EP2 0

#define MIN(A,B) (A>B?B:A)



extern CKETimer aTimer;  // AccurateTimer
extern void FlushSendData(void);


typedef GUID* PGUID;


typedef BOOL(__stdcall* myWinUsb_init)(HANDLE, PWINUSB_INTERFACE_HANDLE);
myWinUsb_init WinUsbDLL_Initialize;
typedef BOOL(__stdcall* myWinUsb_readpipe)(WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
myWinUsb_readpipe WinUsbDLL_ReadPipe;
typedef BOOL(__stdcall* myWinUsb_writepipe)(WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
myWinUsb_writepipe WinUsbDLL_WritePipe;
typedef BOOL(__stdcall* myWinUsb_abortpipe)(WINUSB_INTERFACE_HANDLE, UCHAR);
myWinUsb_abortpipe WinUsbDLL_AbortPipe;
typedef BOOL(__stdcall* myWinUsb_free)(WINUSB_INTERFACE_HANDLE);
myWinUsb_free WinUsbDLL_Free;
typedef BOOL(__stdcall* myWinUsb_setpowerpolicy)(WINUSB_INTERFACE_HANDLE, ULONG, ULONG, PVOID);
myWinUsb_setpowerpolicy WinUsbDLL_SetPowerPolicy;
typedef BOOL(__stdcall* myWinUsb_GetDescriptor)(WINUSB_INTERFACE_HANDLE, UCHAR, UCHAR, USHORT, PUCHAR, ULONG, PULONG);
myWinUsb_GetDescriptor WinUsbDLL_GetDescriptor;




//Private protoypes
HANDLE WinUSB_MPUSBOpen(DWORD instance, TCHAR* pVID_PID, TCHAR* pEP, DWORD dwDir, DWORD dwReserved);
DWORD WinUSB_MPUSBGetDescriptor(HANDLE, PVOID, DWORD, PVOID, DWORD, PDWORD);
DWORD WinUSB_MPUSBGetDeviceDescriptor(HANDLE, PVOID, DWORD, PDWORD);
DWORD WinUSB_MPUSBGetConfigurationDescriptor(HANDLE, UCHAR, PVOID, DWORD, PDWORD);
DWORD WinUSB_MPUSBGetStringDescriptor(HANDLE, UCHAR, USHORT, PVOID, DWORD, PDWORD);
DWORD WinUSB_MPUSBGetConfigInfoSize(HANDLE);
DWORD WinUSB_MPUSBGetConfigInfo(HANDLE, PVOID, DWORD);
DWORD WinUSB_MPUSBSetConfiguration(HANDLE, USHORT);
DWORD WinUSB_MPUSBRead(HANDLE, PVOID, DWORD, PDWORD, DWORD);
DWORD WinUSB_MPUSBWrite(HANDLE, PVOID, DWORD, PDWORD, DWORD);
DWORD WinUSB_MPUSBReadInt(HANDLE, PVOID, DWORD, PDWORD, DWORD);
BOOL WinUSB_MPUSBClose(HANDLE);

BOOL LoadWinUSBDLL(void);


#pragma hdrstop



#define	MPUSB_DEV_NO_INFO           2
#define	MPUSB_DEV_INVALID_INST      3
#define	MPUSB_DEV_VIDPID_NOT_FOUND  4
#define MPUSB_DEV_NOT_ATTACHED      5




// Global Vars

TCHAR ICNC_vid_pid[] = _T("vid_04d8&pid_0001");    // ICNC VID PID
TCHAR ICNC_out_pipe[] = _T("\\MCHP_EP1");
TCHAR ICNC_in_pipe[] = _T("\\MCHP_EP1");
TCHAR ICNC_inevent_pipe[] = _T("\\MCHP_EP2");


TCHAR ICNC_WINUSB_vid_pid[] = _T("vid_04d8&pid_0057");

//GUID GUID_DEVINTERFACE_WINUSB = { 0x14121969L, 0x27C1, 0x11DD, 0xBD, 0x0B, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 };
DEFINE_GUID(GUID_DEVINTERFACE_WINUSB, 0x14121969L, 0x27C1, 0x11DD, 0xBD, 0x0B, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66);
// Dans IOCTL DEFINE_GUID(GUID_DEVINTERFACE_MCHPUSB, 0x5354fa28L, 0x6d14, 0x4e35, 0xa1, 0xf5, 0x75, 0xbb, 0x54, 0xe6, 0x03, 0x0f);



BOOL UsingWinUSBDriver = FALSE;
BOOL WinUSBLibraryLoaded = FALSE;
HANDLE GlobalWinUSBDeviceHandle = INVALID_HANDLE_VALUE;
HANDLE GlobalWinUSBInterfaceHandle = INVALID_HANDLE_VALUE;
BOOL WinUSBInterfaceInitialized = FALSE;
UINT ApparentOpenPipes = 0;
HINSTANCE libHandle;


BOOL USBConnected = false;

HANDLE ICNC_myOutPipe;
HANDLE ICNC_myInPipe;

// Used to make variable initialisation only one time
INT DLLInitialized = FALSE;


// global varaible for using USB endpoint 2 to receive some event (instead of polling)
#if USING_EP2==1
DWORD WINAPI ThreadReadEP2RX(LPVOID lpParam);

HANDLE ICNC_myInPipeEvent;	// Pipe for EP2 event receive

HANDLE hThreadReadEP2RX = NULL;
DWORD ThreadReadEP2RXPrm = 0;
BOOL TerminateBasicRX = false;
DWORD dwThreadIdBasicRX;
#endif



HANDLE hThreadAutoConnect = NULL;
DWORD ThreadAutoConnectPrm = 0;
BOOL ThreadAutoConnectTerminate = false;
DWORD ThreadAutoConnectID;


DWORD WINAPI ThreadAutoConnect(LPVOID lpParam)
{
	//char data[64];
	DWORD pLength = 0;
	static int count = 0;
	unsigned int TickCount = 0;
	while (!ThreadAutoConnectTerminate) {
		if (!USBConnected)
			USBConnect(0);
		Sleep(100);
	}
	ThreadAutoConnectTerminate = false;
	return 0;
}

void EnableAutoConnect(void) {
	if (!hThreadAutoConnect)	// Create reading on EP2 thread
	{
		ThreadAutoConnectTerminate = false;
		hThreadAutoConnect = CreateThread(
			NULL,                        // no security attributes
			0,                           // use default stack size
			ThreadAutoConnect,                  // thread function

			& ThreadAutoConnectPrm,                // argument to thread function
			0,                           // use default creation flags
			& ThreadAutoConnectID);                // returns the thread identifier
	}

	
}
void DisableAutoConnect(void) {
	ThreadAutoConnectTerminate = true;
	WaitForSingleObject(hThreadAutoConnect, 500);
	hThreadAutoConnect = NULL;
}



void USBInitialisation(void) {
	ICNC_myOutPipe = ICNC_myInPipe = INVALID_HANDLE_VALUE;

#if USING_EP2==1
	ICNC_myInPipeEvent = INVALID_HANDLE_VALUE;
#endif

	LoadWinUSBDLL();	// Try to load WINUSB DLL if available on the PC
}


void USBDisconnect() {

	if (ICNC_myOutPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myOutPipe);
	if (ICNC_myInPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipe);
#if USING_EP2==1
	if (ICNC_myInPipeEvent != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipeEvent);
ICNC_myInPipeEvent = INVALID_HANDLE_VALUE;
#endif
	ICNC_myOutPipe = ICNC_myInPipe = INVALID_HANDLE_VALUE;

}




BOOL LoadWinUSBDLL(void)
{
	UsingWinUSBDriver = FALSE;  //Assume we will be using mchpusb driver, unless we later determine we are actually using WinUSB driver
	WinUSBLibraryLoaded = FALSE;
	GlobalWinUSBDeviceHandle = INVALID_HANDLE_VALUE;
	GlobalWinUSBInterfaceHandle = INVALID_HANDLE_VALUE;
	WinUSBInterfaceInitialized = FALSE;
	ApparentOpenPipes = 0;

	libHandle = NULL;
	libHandle = LoadLibrary(_T("winusb.dll"));
	if (libHandle == NULL)
	{
		//Couldn't load winusb.dll in this case...
		UsingWinUSBDriver = FALSE;  //Assume we are using mchpusb driver instead in this case.
		WinUSBLibraryLoaded = FALSE;
		return 1;     //return 1 anyway.  May still be able to use mchpusb.sys driver if it is running instead.
	}
	else
	{
		WinUsbDLL_Initialize = (myWinUsb_init)GetProcAddress(libHandle, "WinUsb_Initialize");
		WinUsbDLL_ReadPipe = (myWinUsb_readpipe)GetProcAddress(libHandle, "WinUsb_ReadPipe");
		WinUsbDLL_WritePipe = (myWinUsb_writepipe)GetProcAddress(libHandle, "WinUsb_WritePipe");
		WinUsbDLL_AbortPipe = (myWinUsb_abortpipe)GetProcAddress(libHandle, "WinUsb_AbortPipe");
		WinUsbDLL_Free = (myWinUsb_free)GetProcAddress(libHandle, "WinUsb_Free");
		WinUsbDLL_SetPowerPolicy = (myWinUsb_setpowerpolicy)GetProcAddress(libHandle, "WinUsb_SetPowerPolicy");      //WDK documentation (6001.18001 is wrong.  It claims there is a "WinUsb_SetInterfacePowerPolicy" function, but dumpbin does not show such a function.
		WinUsbDLL_GetDescriptor = (myWinUsb_GetDescriptor)GetProcAddress(libHandle, "WinUsb_GetDescriptor");
	}//end if else

	//Now check to make we successfully loaded the function address entry points.
	//To avoid application errors during runtime, we should make sure not to try and call
	//a function if we didn't successfully load the function entry point addresses.
	if ((WinUsbDLL_Initialize == NULL) || (WinUsbDLL_ReadPipe == NULL) || (WinUsbDLL_WritePipe == NULL) || (WinUsbDLL_AbortPipe == NULL) || (WinUsbDLL_Free == NULL) || (WinUsbDLL_GetDescriptor == NULL))
	{
		WinUSBLibraryLoaded = FALSE;
		UsingWinUSBDriver = FALSE;  //Assume we are using mchpusb driver instead in this case.
		return 1;
	}
	else
	{
		WinUSBLibraryLoaded = TRUE;
	}
	return 1;
}



DWORD MPUSBIsVidPidEqual(TCHAR* pDevicePath, TCHAR* pVID_PID)
{
	DWORD dwResult = ICNCUSB_FAIL;
	TCHAR lszValue[255];
	TCHAR lpSubKey[512] = _T("lsubkey");
	TCHAR LowerCaseVIDPID[50] = _T("vid_XXXX&pid_YYYY");     //Intermediate space, so as to avoid modifying the pVID_PID string.  It is only an input to this function.

	HKEY hKey;
	LONG returnStatus;
	DWORD dwType = REG_SZ;
	DWORD dwSize = 255;

	//First check for MCHPUSB based GUID.
	GUID guid = GUID_DEVINTERFACE_MCHPUSB;

	if (_tcslen(pVID_PID) < 50)  //Make sure it isn't too long.  Should only be (17+Null terminator) characters long.
	{
		_tcscpy_s(LowerCaseVIDPID, pVID_PID);
		_tcslwr_s(LowerCaseVIDPID);
	}

	/* Modify DevicePath to use registry format */
	pDevicePath[0] = '#';
	pDevicePath[1] = '#';
	pDevicePath[3] = '#';

	/* Form SubKey */
	int resprintf;
	resprintf = _stprintf_s(lpSubKey,
		_T("SYSTEM\\CURRENTCONTROLSET\\CONTROL\\DEVICECLASSES\\{%4.2x-%2.2x-%2.2x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}\\%s"),
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], pDevicePath);

	/* Open Key */
	returnStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		lpSubKey,
		0L,
		KEY_QUERY_VALUE,        //Changed from v1.0.  Used to be "KEY_ALL_ACCESS" which requires administrator logon (and application administrator execution level to work in Vista).
		&hKey);
	if (returnStatus == ERROR_SUCCESS)
	{
		returnStatus = RegQueryValueEx(hKey,
			_T("DeviceInstance"),
			NULL,
			&dwType,
			(LPBYTE) &lszValue,
			&dwSize);

		if (returnStatus == ERROR_SUCCESS)
		{
			/*
			 * The string info stored in 'DeviceInstance' is the same
			 * across all Windows platforms: 98SE, ME, 2K, and XP.
			 * Upper-case in 98SE,ME.
			 * Converts all to lower-case anyway.
			 */
			_tcslwr_s(lszValue, 255);
			if (_tcsstr(lszValue, LowerCaseVIDPID) != NULL)
			{
				dwResult = ICNCUSB_SUCCESS;
			}
		}
	}
	RegCloseKey(hKey);

	/* Modify DevicePath to use the original format */
	pDevicePath[0] = '\\';
	pDevicePath[1] = '\\';
	pDevicePath[3] = '\\';

	if (dwResult == ICNCUSB_SUCCESS)
	{
		UsingWinUSBDriver = FALSE;
		return dwResult;
	}


	//If we get to here, we didn't find the mchpusb interface attached.  However,
	//there still might be a WinUSB interface attached instead.  Need to check for
	//the WinUSB interface now.
	guid = GUID_DEVINTERFACE_WINUSB;

	if (_tclen(pVID_PID) < 50)  //Make sure it isn't too long.  Should only be (17+Null terminator) characters long.
	{
		_tcscpy_s(LowerCaseVIDPID, pVID_PID);
		_tcslwr_s(LowerCaseVIDPID);
	}

	/* Modify DevicePath to use registry format */
	pDevicePath[0] = '#';
	pDevicePath[1] = '#';
	pDevicePath[3] = '#';

	/* Form SubKey */
	_stprintf_s(lpSubKey,
		_T("SYSTEM\\CURRENTCONTROLSET\\CONTROL\\DEVICECLASSES\\{%4.2x-%2.2x-%2.2x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}\\%s"),
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], pDevicePath);

	/* Open Key */
	returnStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		(LPCTSTR)lpSubKey,
		0L,
		KEY_QUERY_VALUE,        //Changed from v1.0.  Used to be "KEY_ALL_ACCESS" which requires administrator logon (and application administrator execution level to work in Vista).
		&hKey);
	if (returnStatus == ERROR_SUCCESS)
	{
		returnStatus = RegQueryValueEx(hKey,
			_T("DeviceInstance"),
			NULL,
			&dwType,
			(LPBYTE)&lszValue,
			&dwSize);
		if (returnStatus == ERROR_SUCCESS)
		{
			/*
			 * The string info stored in 'DeviceInstance' is the same
			 * across all Windows platforms: 98SE, ME, 2K, and XP.
			 * Upper-case in 98SE,ME.
			 * Converts all to lower-case anyway.
			 */
			_tcslwr_s(lszValue);
			if (_tcsstr(lszValue, LowerCaseVIDPID) != NULL)
			{
				dwResult = ICNCUSB_SUCCESS;
			}
		}
	}
	RegCloseKey(hKey);

	/* Modify DevicePath to use the original format */
	pDevicePath[0] = '\\';
	pDevicePath[1] = '\\';
	pDevicePath[3] = '\\';

	if (dwResult == ICNCUSB_SUCCESS)
	{
		UsingWinUSBDriver = TRUE;
	}

	return dwResult;

}//end

///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetDeviceLink : Returns the path to device hardware with a given
//  instance number.

DWORD MPUSBGetDeviceLink(DWORD instance,    // Input
	TCHAR* pVID_PID,    // Input
	TCHAR* pPath,       // Output
	DWORD dwLen,       // Input
	PDWORD pLength)    // Output
{
	if (pLength != NULL)*pLength = 0;        // Initialization

	HDEVINFO info = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_MCHPUSB, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(info);
		return MPUSB_DEV_NO_INFO;
	}// end if

	// Get interface data for the requested instance
	SP_DEVICE_INTERFACE_DATA intf_data;
	intf_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (!SetupDiEnumDeviceInterfaces(info, NULL, (LPGUID)&GUID_DEVINTERFACE_MCHPUSB, instance, &intf_data))
	{
		//Failed to find a match.
		SetupDiDestroyDeviceInfoList(info);
		//Now check for a matching WinUSB device instead.
		info = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_WINUSB, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (info == INVALID_HANDLE_VALUE)
		{
			SetupDiDestroyDeviceInfoList(info);
			return MPUSB_DEV_NO_INFO;
		}// end if

	// Get interface data for the requested instance
		intf_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		if (!SetupDiEnumDeviceInterfaces(info, NULL, (LPGUID)&GUID_DEVINTERFACE_WINUSB, instance, &intf_data))
		{
			//Failed to find a match of either mchpusb or winusb.
			SetupDiDestroyDeviceInfoList(info);
			return MPUSB_DEV_INVALID_INST;
		}

	}// end if

	// Get size of symbolic link
	DWORD ReqLen;
	SetupDiGetDeviceInterfaceDetail(info, &intf_data, NULL, 0, &ReqLen, NULL);

	//PSP_DEVICE_INTERFACE_DETAIL_DATA intf_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(new char[ReqLen]);
	PSP_DEVICE_INTERFACE_DETAIL_DATA intf_detail;
	intf_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)GlobalAlloc(GPTR, ReqLen + 4);
	intf_detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

	if (intf_detail == NULL)
	{
		SetupDiDestroyDeviceInfoList(info);
		GlobalFree(intf_detail);
		return MPUSB_DEV_NO_INFO;
	}// end if

	// Get symbolic link name
	//intf_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	// sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) should equals 5.
	// In C++ Builder, go to Project/Options/Advanced Compiler/Data Alignment
	// and select "byte" align.

	if (!SetupDiGetDeviceInterfaceDetail(info, &intf_data, intf_detail, ReqLen, NULL, NULL))
	{
		SetupDiDestroyDeviceInfoList(info);
		GlobalFree(intf_detail);
		return MPUSB_DEV_NO_INFO;
	}// end if

	// Check for a valid VID&PID - if argument is not null)
	if (pVID_PID != NULL)
	{
		if (MPUSBIsVidPidEqual(intf_detail->DevicePath, pVID_PID) == NULL)
		{
			SetupDiDestroyDeviceInfoList(info);
			GlobalFree(intf_detail);
			return MPUSB_DEV_VIDPID_NOT_FOUND;
		}// end if
	}// end if

	// Set the length of the path string
	if (pLength != NULL)
		*pLength = (DWORD)_tclen(intf_detail->DevicePath);

	// Copy output string path to buffer pointed to by pPath
	if (pPath != NULL)
	{
		// Check that input buffer has enough room...
		// Use > not >= because strlen does not include null
		if (dwLen > _tclen(intf_detail->DevicePath))
			_tcscpy(pPath, intf_detail->DevicePath);
		else
		{
			SetupDiDestroyDeviceInfoList(info);
			GlobalFree(intf_detail);
			return ICNCUSB_FAIL;
		}// end if
	}// end if

	// Clean up
	SetupDiDestroyDeviceInfoList(info);
	GlobalFree(intf_detail);
	return ICNCUSB_SUCCESS;

}// end MPUSBGetDeviceLink



///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetDeviceCount : Returns the number of devices with matching VID & PID
//

// !!!!!!!!!!!! non utilisée
DWORD MPUSBGetDeviceCount(TCHAR* pVID_PID)
{
	DWORD count;        // Number of USB device with matching VID & PID
	count = 0;          // Initialization

	for (int i = 0; i < MAX_NUM_MPUSB_DEV; i++)
	{
		if (MPUSBGetDeviceLink(i, pVID_PID, NULL, NULL, NULL) == ICNCUSB_SUCCESS)
			count++;
	}//end for
	return count;
}//end MPUSBGetDeviceCount

 ///////////////////////////////////////////////////////////////////////////////
//	MPUSBOpen : Returns the handle to the endpoint pipe with matching VID & PID
//
//  All pipes are opened with the FILE_FLAG_OVERLAPPED attribute.
//  This allows MPUSBRead,MPUSBWrite, and MPUSBReadInt to have a time-out value.
//
//  Note: Time-out value has no meaning for Isochronous pipes.
//
//  instance - An instance number of the device to open.
//             Typical usage is to call MPUSBGetDeviceCount first to find out
//             how many instances there are.
//             It is important to understand that the driver is shared among
//             different devices. The number of devices returned by
//             MPUSBGetDeviceCount could be equal to or less than the number
//             of all the devices that are currently connected & using the
//             generic driver.
//
//             Example:
//             if there are 3 device with the following PID&VID connected:
//             Device Instance 0, VID 0x04d8, PID 0x0001
//             Device Instance 1, VID 0x04d8, PID 0x0002
//             Device Instance 2, VID 0x04d8, PID 0x0001
//
//             If the device of interest has VID = 0x04d8 and PID = 0x0002
//             Then MPUSBGetDeviceCount will only return '1'.
//             The calling function should have a mechanism that attempts
//             to call MPUSBOpen up to the absolute maximum of MAX_NUM_MPUSB_DEV
//             (MAX_NUM_MPUSB_DEV is defined in _mpusbapi.h).
//             It should also keep track of the number of successful calls
//             to MPUSBOpen(). Once the number of successes equals the
//             number returned by MPUSBGetDeviceCount, the attempts should
//             be aborted because there will no more devices with
//             a matching vid&pid left.
//
//  pVID_PID - A string containing the PID&VID value of the target device.
//             The format is "vid_xxxx&pid_yyyy". Where xxxx is the VID value
//             in hex and yyyy is the PID value in hex.
//             Example: If a device has the VID value of 0x04d8 and PID value
//                      of 0x000b, then the input string should be:
//                      "vid_04d8&pid_000b"
//
//  pEP      - A string of the endpoint number on the target endpoint to open.
//             The format is "\\MCHP_EPz". Where z is the endpoint number in
//             decimal.
//             Example: "\\MCHP_EP1"
//
//             This arguement can be NULL. A NULL value should be used to
//             create a handles for non-specific endpoint functions.
//             MPUSBRead, MPUSBWrite, MPUSBReadInt are endpoint specific
//             functions.
//             All others are not.
//             Non-specific endpoint functions will become available in the
//             next release of the DLL.
//
//             Note: To use MPUSBReadInt(), the format of pEP has to be
//                   "\\MCHP_EPz_ASYNC". This option is only available for
//                   an IN interrupt endpoint. A data pipe opened with the
//                   "_ASYNC" keyword would buffer the data at the interval
//                   specified in the endpoint descriptor upto the maximum of
//                   100 data sets. Any data received after the driver buffer
//                   is full will be ignored.
//                   The user application should call MPUSBReadInt() often
//                   enough so that the maximum limit of 100 is never reached.
//
//  dwDir    - Specifies the direction of the endpoint.
//             Use MP_READ for MPUSBRead, MPSUBReadInt
//             Use MP_WRITE for MPUSBWrite
//
//  dwReserved Future Use
//
//  Summary of transfer type usage:
//  ============================================================================
//  Transfer Type       Functions                       Time-Out Applicable?
//  ============================================================================
//  Interrupt - IN      MPUSBRead, MPUSBReadInt         Yes
//  Interrupt - OUT     MPUSBWrite                      Yes
//  Bulk - IN           MPUSBRead                       Yes
//  Bulk - OUT          MPUSBWrite                      Yes
//  Isochronous - IN    MPUSBRead                       No
//  Isochronous - OUT   MPUSBWrite                      No
//  ============================================================================
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
///////////////////////////////////////////////////////////////////////////////
//Entrypoint for any MPUSBOpen() call.  Also implements the MCHPUSB based version of MPUSBOpen().
HANDLE MPUSBOpen(DWORD instance,    // Input
	TCHAR* pVID_PID,    // Input NOT USED
	TCHAR* pEP,         // Input
	DWORD dwDir,       // Input
	DWORD dwReserved)  // Input <Future Use>
{
	HANDLE WinUSBReturnHandle;


	TCHAR myVID_PID[18];
	//memcpy(myVID_PID, ICNC_WINUSB_vid_pid, strlen(ICNC_WINUSB_vid_pid) + 1);
	_tcscpy_s( myVID_PID, 18, ICNC_WINUSB_vid_pid);

	//If WinUSB library was successfully loaded, make an initial attempt to connect to the
	//device using the WinUSB API.  If this succeeds, this means the device is attached
	//and the WinUSB driver has been loaded for the device.  If this fails, this could
	//mean either the device isn't connected at all, or that the device is using the mchpusb
	//driver instead.  In this case, we need to make another attempt to connect to it,
	//using the method appropriate for the mchpusb driver instead.
	if (WinUSBLibraryLoaded == TRUE)
	{

		WinUSBReturnHandle = WinUSB_MPUSBOpen(instance, myVID_PID/*pVID_PID*/, pEP, dwDir, dwReserved);
		if (UsingWinUSBDriver == TRUE)
		{
			return WinUSBReturnHandle;
		}
	}

	TCHAR path[MAX_PATH];
	DWORD dwReqLen;

	HANDLE handle;
	handle = INVALID_HANDLE_VALUE;

	//memcpy(myVID_PID, ICNC_vid_pid, strlen(ICNC_vid_pid) + 1);
	_tcscpy_s(myVID_PID, 18, ICNC_vid_pid);

	// Check arguments first
	if ((myVID_PID != NULL) && ((dwDir == MP_WRITE) || (dwDir == MP_READ)))
	{
		if (MPUSBGetDeviceLink(instance, myVID_PID /*pVID_PID*/, path, MAX_PATH, &dwReqLen) == ICNCUSB_SUCCESS)
		{
			TCHAR path_io[MAX_PATH];
			//strcpy_s(path_io, path);
			_tcscpy_s(path_io, path);
			if (pEP != NULL) _tcscat_s(path_io, pEP);

			if (dwDir == MP_READ)
			{
				handle = CreateFile(path_io,
					GENERIC_READ,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					NULL);
			}
			else
			{
				handle = CreateFile(path_io,
					GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					NULL);
			}//end if
		}//end if
	}//end if
	return handle;
}//end MPUSBOpen(...)







#if USING_EP2==1

void KillEP2Thread(void) {
	// Stop reading thread on EP2
	if (hThreadReadEP2RX) {
		TerminateBasicRX = true;
		WaitForSingleObject(hThreadReadEP2RX, 500);
		hThreadReadEP2RX = NULL;
	}
}


void CreateEP2Thread(void) {
	if (!hThreadReadEP2RX)	// Create reading on EP2 thread
	{
		TerminateBasicRX = false;
		hThreadReadEP2RX = CreateThread(
			NULL,                        // no security attributes
			0,                           // use default stack size
			ThreadReadEP2RX,                  // thread function

			&ThreadReadEP2RXPrm,                // argument to thread function
			0,                           // use default creation flags
			&dwThreadIdBasicRX);                // returns the thread identifier
	}

}

DWORD WINAPI ThreadReadEP2RX(LPVOID lpParam)
{
	char data[64];
	DWORD dwLen;
	DWORD pLength=0;
	static int count = 0;
	unsigned int TickCount = 0;
	while (!TerminateBasicRX) {
		try {
			if ((GlobalWinUSBInterfaceHandle == NULL) || (ICNC_myInPipeEvent== INVALID_HANDLE_VALUE))
				Sleep(100);
			else
			{
				int res = MPUSBRead(ICNC_myInPipeEvent,			// EP2 endpoint
					data,            // data received Output
					64,            // Expected input length Input
					&pLength,         // Received length Output
					999999999);				// timeout Input

				if (res == ICNCUSB_SUCCESS) {
					count++;
					TickCount = *((unsigned int*)data);

				}

			}
		}
		catch (...) {
			TerminateBasicRX = true;
		}
	}
	TerminateBasicRX = false;
	return 0;
}
#else 
	#define KillEP2Thread()		// empty function to avoid multiple test of  USING_EP2 in source code
#endif

DWORD USBConnect(DWORD ICNC_Number) {
	// Close if already opened
	if (ICNC_myOutPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myOutPipe);
	if (ICNC_myInPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipe);
	ICNC_myOutPipe = ICNC_myInPipe = INVALID_HANDLE_VALUE;
#if USING_EP2==1
	if (ICNC_myInPipeEvent != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipeEvent);
	ICNC_myInPipeEvent = INVALID_HANDLE_VALUE;
#endif

	ICNC_myOutPipe = MPUSBOpen(ICNC_Number, ICNC_vid_pid, ICNC_out_pipe, MP_WRITE, 0);
	ICNC_myInPipe = MPUSBOpen(ICNC_Number, ICNC_vid_pid, ICNC_in_pipe, MP_READ, 0);
#if USING_EP2==1
	ICNC_myInPipeEvent = MPUSBOpen(ICNC_Number, ICNC_vid_pid, ICNC_inevent_pipe, MP_READ, 0);
#endif

#if USING_EP2==1
	if (ICNC_myOutPipe == INVALID_HANDLE_VALUE || ICNC_myInPipe == INVALID_HANDLE_VALUE || ICNC_myInPipeEvent == INVALID_HANDLE_VALUE) {

		if (ICNC_myOutPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myOutPipe);
		if (ICNC_myInPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipe);
		if (ICNC_myInPipeEvent != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipeEvent);
		return ICNCUSB_FAIL;
	}
#else
	if (ICNC_myOutPipe == INVALID_HANDLE_VALUE || ICNC_myInPipe == INVALID_HANDLE_VALUE) {

		if (ICNC_myOutPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myOutPipe);
		if (ICNC_myInPipe != INVALID_HANDLE_VALUE) MPUSBClose(ICNC_myInPipe);
		return ICNCUSB_FAIL;
	}
#endif
#if USING_EP2==1
	CreateEP2Thread();
#endif
	USBConnected = true;
	return ICNCUSB_SUCCESS;
}



///////////////////////////////////////////////////////////////////////////////
//Function: WinUSB_MPUSBOpen()
//Note: API equivalent of MPUSBOpen().  Used for when using the WinUSB driver instead of mchpusb.
//
//      NOTE: This modified version (using WinUSB) of MPUSBOpen does not actually return a HANDLE.
//      Due to differences in the native API of WinUSB and MPUSBAPI, we return a value that
//      contains the info for the endpoint that will be used instead.  This will be used by the MPUSBRead() and
//      MPUSBWrite() functions to determine which pipe to send/receive the data to/from.
///////////////////////////////////////////////////////////////////////////////
HANDLE WinUSB_MPUSBOpen(DWORD instance,    // Input
	TCHAR* pVID_PID,    // Input
	TCHAR* pEP,         // Input
	DWORD dwDir,       // Input
	DWORD dwReserved)  // Input <Future Use>
{
	TCHAR path[MAX_PATH];
	DWORD dwReqLen;
	BOOL BoolStatus;
	HANDLE EPNumNotRealHandle = INVALID_HANDLE_VALUE;
	ULONG ValueLength = 1;      //Used by WinUsbDLL_SetInterfacePowerPolicy()
	UCHAR InterfacePowerPolicyValue;

	if (pEP != NULL) //Make sure pointer is valid before using it.
	{
		
		if ((_tcsstr(pEP, _T("\\MCHP_EP0")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep0")) != NULL))
		{
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x80;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x00;
		}

		if ((_tcscmp(pEP, _T("\\MCHP_EP1")) == 0) || (_tcscmp(pEP, _T("\\MCHP_EP1_ASYNC")) == 0))  //Make sure the string is exactly the right length, otherwise it might be a "\\MCHP_EP11" or other EP1x endpoint instead of EP1
		{
			if ((_tcsstr(pEP, _T("\\MCHP_EP1")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep1")) != NULL))
			{
				if (dwDir == MP_READ)
					EPNumNotRealHandle = (HANDLE)0x81;
				else if (dwDir == MP_WRITE)
					EPNumNotRealHandle = (HANDLE)0x01;
			}
		}

		if ((_tcsstr(pEP, _T("\\MCHP_EP2")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep2")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x82;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x02;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP3")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep3")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x83;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x03;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP4")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep4")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x84;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x04;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP5")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep5")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x85;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x05;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP6")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep6")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x86;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x06;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP4")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep7")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x87;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x07;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP8")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep8")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x88;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x08;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP9")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep9")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x89;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x09;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP10")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep10")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8A;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0A;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP11")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep11")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8B;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0B;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP12")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep12")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8C;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0C;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP13")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep13")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8D;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0D;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP14")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep14")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8E;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0E;
		}
		if ((_tcsstr(pEP, _T("\\MCHP_EP15")) != NULL) || (_tcsstr(pEP, _T("\\mchp_ep15")) != NULL)) {
			if (dwDir == MP_READ)
				EPNumNotRealHandle = (HANDLE)0x8F;
			else if (dwDir == MP_WRITE)
				EPNumNotRealHandle = (HANDLE)0x0F;
		}
	} //end if(pEP != NULL)


	// Check arguments first
	if ((pVID_PID != NULL) && ((dwDir == MP_WRITE) || (dwDir == MP_READ)))
	{
		if (MPUSBGetDeviceLink(instance, pVID_PID, path, MAX_PATH, &dwReqLen) == ICNCUSB_SUCCESS)
		{
			TCHAR path_io[MAX_PATH];
			//strcpy(path_io, path);
			_tcscpy_s(path_io, path);

			//We now have the proper device path, and we can finally open a device handle to the device.
		//WinUSB requires the device handle to be opened with the FILE_FLAG_OVERLAPPED attribute.
			//This MPUSBOpen function may have already been called.  In this case, the WinUSB device and interface handles may have already been opened.
			//in this case, don't want to try to open them again.
			if ((GlobalWinUSBDeviceHandle == INVALID_HANDLE_VALUE) || (WinUSBInterfaceInitialized == FALSE))
			{
				GlobalWinUSBDeviceHandle = CreateFile((LPCTSTR)(path_io), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
				if (GetLastError() == ERROR_SUCCESS)
				{
					//Now get the WinUSB interface handle by calling WinUsb_Initialize() and providing the device handle.
					BoolStatus = WinUsbDLL_Initialize(GlobalWinUSBDeviceHandle, &GlobalWinUSBInterfaceHandle);
					if (BoolStatus == TRUE)  //Successfully intialized the WinUSB interface
					{
						ApparentOpenPipes++;
						InterfacePowerPolicyValue = FALSE;      //Value for the AUTO_SUSPEND setting.  The default is enabled, but to be more compatible, it is preferrable to disable AUTO_SUSPEND by default.
						//if (WinUsbDLL_SetPowerPolicy != NULL) //Not done earlier.  Some uncertainty if this will always work since the WDK documentation (6001.18001) is incorrect describing this function.  Try to make this fail safe if it doesn't work.
						//{
						//	WinUsbDLL_SetPowerPolicy(GlobalWinUSBInterfaceHandle, AUTO_SUSPEND, ValueLength, &InterfacePowerPolicyValue);
						//	SetLastError(ERROR_SUCCESS);
						//}
						WinUSBInterfaceInitialized = TRUE;
						if (pEP == NULL)    //If MPUSBOpen() was called with pEP == NULL, then the mchpusb version of mpusbapi.dll will still return a valid interface handle, instead of returning INVALID_HANDLE_VALUE.
						{
							if (dwDir == MP_READ)
								EPNumNotRealHandle = (HANDLE)0x80;
							else if (dwDir == MP_WRITE)
								EPNumNotRealHandle = (HANDLE)0x00;
						}
						UsingWinUSBDriver = TRUE;
						return EPNumNotRealHandle;
						//If gets here, the "MyWinUSBInterfaceHandle" was initialized successfully.
					//May begin using the MyWinUSBInterfaceHandle handle in WinUsb_WritePipe() and
					//WinUsb_ReadPipe() function calls now.  Those are the functions for writing/reading to
					//the USB device's endpoints.
					}
					else
					{
						WinUSBInterfaceInitialized = FALSE;
						CloseHandle(GlobalWinUSBDeviceHandle);
						GlobalWinUSBInterfaceHandle = INVALID_HANDLE_VALUE;
						UsingWinUSBDriver = FALSE;
						return INVALID_HANDLE_VALUE;
					}
				}
				else
				{
					WinUSBInterfaceInitialized = FALSE;
					GlobalWinUSBInterfaceHandle = INVALID_HANDLE_VALUE;
					UsingWinUSBDriver = FALSE;
					return INVALID_HANDLE_VALUE;
				}
			}
			else  //else the WinUSB interface was already initialized from a prior call.  don't need to re-initialize it in this case.
			{
				ApparentOpenPipes++;
				SetLastError(ERROR_SUCCESS);
				if (pEP == NULL)    //If MPUSBOpen() was called with pEP == NULL, then the mchpusb version of mpusbapi.dll will still return a valid interface handle, instead of returning INVALID_HANDLE_VALUE.
				{
					if (dwDir == MP_READ)
						EPNumNotRealHandle = (HANDLE)0x80;
					else if (dwDir == MP_WRITE)
						EPNumNotRealHandle = (HANDLE)0x00;
				}
				return EPNumNotRealHandle;
			}
		}//end if(MPUSBGetDeviceLink ...)
		else //else couldn't even find a device that was attached!
		{
			EPNumNotRealHandle = INVALID_HANDLE_VALUE;
			if (WinUSBInterfaceInitialized == TRUE)
			{
				WinUSBInterfaceInitialized = FALSE;
			}
		}
	}//end if
	SetLastError(ERROR_INVALID_DATA);
	return EPNumNotRealHandle;
}//end WinUSB_MPUSBOpen(...)






///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetDescriptor : Returns the Requested Descriptor Data
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with MP_READ access attribute.
//  pDscParam
//          - pointer to a descriptor parameters object to be sent down to the
//            driver.  This object specifies the details about the descriptor
//            request.
//  dscLen  - the size of the pDscParam object
//  pDevDsc - pointer to where the resulting descriptor should be copied.
//  dwLen   - the available data in the pDevDsc buffer
//  pLength - a pointer to a DWORD that will be updated with the amount of data
//            actually written to the pDevDsc buffer.  This number will be
//            less than or equal to dwLen.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
///////////////////////////////////////////////////////////////////////////////
DWORD MPUSBGetDescriptor(HANDLE handle,       // Input
	PVOID pDscParam,     // Input
	DWORD dscLen,        // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetDescriptor(handle, pDscParam, dscLen, pDevDsc, dwLen, pLength);
	}

	if (pLength != NULL)*pLength = 0;
	if (pDevDsc == NULL) return ICNCUSB_FAIL;
	if (pDscParam == NULL) return ICNCUSB_FAIL;

	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_DESCRIPTOR,
		pDscParam,
		dscLen,
		pDevDsc,
		dwLen,
		pLength,
		NULL))
	{
		//printf("Get dsc error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if
	return ICNCUSB_SUCCESS;
}// MPUSBGetDescriptor()

///////////////////////////////////////////////////////////////////////////////
//Function: WinUSB_MPUSBGetDescriptor()
//Note: API equivalent of MPUSBGetDescriptor().  Used for when using the WinUSB driver instead of mchpusb.
///////////////////////////////////////////////////////////////////////////////
DWORD WinUSB_MPUSBGetDescriptor(HANDLE handle,       // Input
	PVOID pDscParam,     // Input
	DWORD dscLen,        // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	if (pLength != NULL)
	{
		*pLength = 0;
	}

	if (pDevDsc == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ICNCUSB_FAIL;
	}
	if (pDscParam == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ICNCUSB_FAIL;
	}

	UCHAR DescriptorType = ((_GET_DESCRIPTOR_PARAMETER*)pDscParam)->bType;  //This is the bDescriptorType value (see USB 2.0 specs for values)
	UCHAR Index = ((_GET_DESCRIPTOR_PARAMETER*)pDscParam)->bIndex;
	USHORT LanguageID = ((_GET_DESCRIPTOR_PARAMETER*)pDscParam)->wLangid;

	if (WinUsbDLL_GetDescriptor(GlobalWinUSBInterfaceHandle, DescriptorType, Index, LanguageID, (PUCHAR)pDevDsc, dwLen, pLength) == TRUE)
	{
		return ICNCUSB_SUCCESS;
	}
	else
	{
		return ICNCUSB_FAIL;
	}
}// WinUSB_MPUSBGetDescriptor()

///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetDeviceDescriptor : Returns the Device Descriptor Data
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with MP_READ access attribute.
//  pDevDsc - pointer to where the resulting descriptor should be copied.
//  dwLen   - the available data in the pDevDsc buffer
//  pLength - a pointer to a DWORD that will be updated with the amount of data
//            actually written to the pDevDsc buffer.  This number will be
//            less than or equal to dwLen.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBGetDeviceDescriptor(HANDLE handle,       // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetDeviceDescriptor(handle, pDevDsc, dwLen, pLength);
	}

	GET_DESCRIPTOR_PARAMETER DscParam;

	if (pLength != NULL)*pLength = 0;
	if (pDevDsc == NULL) return ICNCUSB_FAIL;

	DscParam.bType = USB_DEVICE_DESCRIPTOR_TYPE;
	DscParam.bIndex = 0;
	DscParam.wLangid = 0;

	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_DESCRIPTOR,
		&DscParam,
		sizeof(DscParam),
		pDevDsc,
		dwLen,
		pLength,
		NULL))
	{
		//printf("Get dsc error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if
	return ICNCUSB_SUCCESS;
}// MPUSBGetDeviceDescriptor

///////////////////////////////////////////////////////////////////////////////
//Function: WinUSB_MPUSBGetDeviceDescriptor()
//Note: API equivalent of MPUSBGetDeviceDescriptor().  Used for when using the WinUSB driver instead of mchpusb.
///////////////////////////////////////////////////////////////////////////////
DWORD WinUSB_MPUSBGetDeviceDescriptor(HANDLE handle,       // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	if (pLength != NULL)
	{
		*pLength = 0;
	}

	if (pDevDsc == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ICNCUSB_FAIL;
	}

	UCHAR DescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;  //This is the bDescriptorType value (see USB 2.0 specs for values, 0x01 is device descriptor)
	UCHAR Index = 0x00;
	USHORT LanguageID = 0x00;  //LangID is 0 for non-string descriptors

	if (WinUsbDLL_GetDescriptor(GlobalWinUSBInterfaceHandle, DescriptorType, Index, LanguageID, (PUCHAR)pDevDsc, dwLen, pLength) == TRUE)
	{
		return ICNCUSB_SUCCESS;
	}
	else
	{
		return ICNCUSB_FAIL;
	}
}// WinUSB_MPUSBGetDeviceDescriptor

///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetConfigurationDescriptor : Returns the Configuration Descriptor
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with MP_READ access attribute.
//  bIndex  - the index of the configuration descriptor desired.  Valid input
//            range is 1 - 255.
//  pDevDsc - pointer to where the resulting descriptor should be copied.
//  dwLen   - the available data in the pDevDsc buffer
//  pLength - a pointer to a DWORD that will be updated with the amount of data
//            actually written to the pDevDsc buffer.  This number will be
//            less than or equal to dwLen.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBGetConfigurationDescriptor(HANDLE handle,       // Input
	UCHAR bIndex,        // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetConfigurationDescriptor(handle, bIndex, pDevDsc, dwLen, pLength);
	}

	GET_DESCRIPTOR_PARAMETER DscParam;

	if (pLength != NULL)*pLength = 0;
	if (pDevDsc == NULL) return ICNCUSB_FAIL;
	if (bIndex == 0) return ICNCUSB_FAIL;

	DscParam.bType = USB_CONFIGURATION_DESCRIPTOR_TYPE;
	DscParam.bIndex = bIndex - 1;
	DscParam.wLangid = 0;

	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_DESCRIPTOR,
		&DscParam,
		sizeof(DscParam),
		pDevDsc,
		dwLen,
		pLength,
		NULL))
	{
		//printf("Get dsc error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if
	return ICNCUSB_SUCCESS;
}// MPUSBGetConfigurationDescriptor


DWORD WinUSB_MPUSBGetConfigurationDescriptor(HANDLE handle,       // Input
	UCHAR bIndex,        // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	if (pLength != NULL)
		*pLength = 0;

	if (pDevDsc == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ICNCUSB_FAIL;
	}

	UCHAR DescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE;  //This is the bDescriptorType value (see USB 2.0 specs for values)
	USHORT LanguageID = 0x00;  //LangID is 0 for non-string descriptors

	if (WinUsbDLL_GetDescriptor(GlobalWinUSBInterfaceHandle, DescriptorType, bIndex, LanguageID, (PUCHAR)pDevDsc, dwLen, pLength) == TRUE)
	{
		return ICNCUSB_SUCCESS;
	}
	else
	{
		return ICNCUSB_FAIL;
	}
}// WinUSB_MPUSBGetConfigurationDescriptor

///////////////////////////////////////////////////////////////////////////////
//	MPUSBGetStringDescriptor : Returns the requested string descriptor
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with MP_READ access attribute.
//  bIndex  - the index of the configuration descriptor desired.  Valid input
//            range is 0 - 255.
//  wLangId - the language ID of the string that needs to be read
//  pDevDsc - pointer to where the resulting descriptor should be copied.
//  dwLen   - the available data in the pDevDsc buffer
//  pLength - a pointer to a DWORD that will be updated with the amount of data
//            actually written to the pDevDsc buffer.  This number will be
//            less than or equal to dwLen.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBGetStringDescriptor(HANDLE handle,       // Input
	UCHAR bIndex,        // Input
	USHORT wLangId,      // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetStringDescriptor(handle, bIndex, wLangId, pDevDsc, dwLen, pLength);
	}

	GET_DESCRIPTOR_PARAMETER DscParam;

	if (pLength != NULL)*pLength = 0;
	if (pDevDsc == NULL) return ICNCUSB_FAIL;

	DscParam.bType = USB_STRING_DESCRIPTOR_TYPE;
	DscParam.bIndex = bIndex;
	DscParam.wLangid = wLangId;

	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_DESCRIPTOR,
		&DscParam,
		sizeof(DscParam),
		pDevDsc,
		dwLen,
		pLength,
		NULL))
	{
		//printf("Get dsc error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if
	return ICNCUSB_SUCCESS;
}// MPUSBGetConfigurationDescriptor


DWORD WinUSB_MPUSBGetStringDescriptor(HANDLE handle,       // Input
	UCHAR bIndex,        // Input
	USHORT wLangId,      // Input
	PVOID pDevDsc,       // Output
	DWORD dwLen,         // Input
	PDWORD pLength)      // Output
{
	if (pLength != NULL)
	{
		*pLength = 0;
	}

	if (pDevDsc == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ICNCUSB_FAIL;
	}


	UCHAR DescriptorType = USB_STRING_DESCRIPTOR_TYPE;  //This is the bDescriptorType value (see USB 2.0 specs for values)

	if (WinUsbDLL_GetDescriptor(GlobalWinUSBInterfaceHandle, DescriptorType, bIndex, wLangId, (PUCHAR)pDevDsc, dwLen, pLength) == TRUE)
	{
		return ICNCUSB_SUCCESS;
	}
	else
	{
		return ICNCUSB_FAIL;
	}
}// WinUSB_MPUSBGetStringDescriptor


//  **** INCOMPLETE ****
DWORD MPUSBGetConfigInfoSize(HANDLE handle)
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetConfigInfoSize(handle);
	}

	DWORD config_size;
	DWORD dwReqLen;
	DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_CONFIGURATION_INFO,
		NULL,
		0,
		&config_size,
		sizeof(DWORD),
		&dwReqLen,
		NULL);
	return config_size;
}//end MPUSBGetConfigInfoSize

DWORD WinUSB_MPUSBGetConfigInfoSize(HANDLE handle)
{
	//  **** INCOMPLETE ****
	SetLastError(ERROR_NOT_SUPPORTED);
	return ICNCUSB_FAIL;
}//end MPUSBGetConfigInfoSize


//  **** INCOMPLETE ****
DWORD MPUSBGetConfigInfo(HANDLE handle,         // Input
	PVOID pData,           // Output
	DWORD dwLen)           // Input
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBGetConfigInfo(handle, pData, dwLen);
	}

	DWORD dwReqLen;
	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_GET_CONFIGURATION_INFO,
		NULL,
		0,
		pData,
		dwLen,
		&dwReqLen,
		NULL))
	{
		//printf("Get config error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if

	return ICNCUSB_SUCCESS;
}//end MPUSBGetConfigInfo

DWORD WinUSB_MPUSBGetConfigInfo(HANDLE handle,         // Input
	PVOID pData,           // Output
	DWORD dwLen)           // Input
{
	//  **** INCOMPLETE ****
	SetLastError(ERROR_NOT_SUPPORTED);
	return ICNCUSB_FAIL;
}//end WinUSB_MPUSBGetConfigInfo



//  **** INCOMPLETE ****
DWORD MPUSBSendControl(HANDLE handle,         // Input
	PVOID pData,           // Input
	DWORD dwLen)           // Input
{
	SetLastError(ERROR_NOT_SUPPORTED);
	return ICNCUSB_FAIL;
}

//  **** INCOMPLETE ****
DWORD MPUSBGetControl(HANDLE handle,         // Input
	PVOID pData,           // Ouput
	DWORD dwLen)           // Input
{
	SetLastError(ERROR_NOT_SUPPORTED);
	return ICNCUSB_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//	MPUSBSetConfiguration : Sets the device configuration through a USB
//            SET_CONFIGURATION command.
//
//  handle  - Identifies the endpoint pipe to be written. The pipe handle must
//            have been created with MP_WRITE access attribute.
//
//  bConfigSetting
//          - Denotes the configuration number that needs to be set.  If this
//            number does not fall in the devices allowed configurations then
//            this function will return with MP_FAIL
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBSetConfiguration(HANDLE handle,         // Input
	USHORT bConfigSetting) // Input
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBSetConfiguration(handle, bConfigSetting);
	}

	SET_CONFIGURATION_PARAMETER  cfgParam;
	DWORD dwReturnData;
	DWORD dwReqLen;

	cfgParam.bConfigurationValue = (UCHAR)bConfigSetting;

	if (!DeviceIoControl(handle,
		IOCTL_MCHPUSB_SET_CONFIGURATION,
		&cfgParam,
		sizeof(cfgParam),
		&dwReturnData,
		sizeof(dwReturnData),
		&dwReqLen,
		NULL))
	{
		//printf("Set configuration error: %d"), GetLastError());
		return ICNCUSB_FAIL;
	}//end if

	return ICNCUSB_SUCCESS;
}

DWORD WinUSB_MPUSBSetConfiguration(HANDLE handle,         // Input
	USHORT bConfigSetting) // Input
{
	//Unfortunately there doesn't appear to be any directly equivalent function
	//provided by winusb.dll.  It might be possible to partially emulate
	//this request using the WinUsb_ControlTransfer() function, but this might
	//not be fully feasible...
	SetLastError(ERROR_NOT_SUPPORTED);
	return ICNCUSB_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//	MPUSBRead :
//
//  handle  - Identifies the endpoint pipe to be read. The pipe handle must
//            have been created with MP_READ access attribute.
//
//  pData   - Points to the buffer that receives the data read from the pipe.
//
//  dwLen   - Specifies the number of bytes to be read from the pipe.
//
//  pLength - Points to the number of bytes read. MPUSBRead sets this value to
//            zero before doing any work or error checking.
//
//  dwMilliseconds
//          - Specifies the time-out interval, in milliseconds. The function
//            returns if the interval elapses, even if the operation is
//            incomplete. If dwMilliseconds is zero, the function tests the
//            data pipe and returns immediately. If dwMilliseconds is INFINITE,
//            the function's time-out interval never elapses.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBRead(HANDLE handle,          // Input
	PVOID pData,            // Output
	DWORD dwLen,            // Input
	PDWORD pLength,         // Output
	DWORD dwMilliseconds)   // Input
{
	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBRead(handle, pData, dwLen, pLength, dwMilliseconds);
	}

	BOOL bResult;
	DWORD nBytesRead;
	OVERLAPPED gOverlapped;
	DWORD dwResult;

	dwResult = ICNCUSB_FAIL;

	// set up overlapped structure fields
	gOverlapped.Internal = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset = 0;
	gOverlapped.OffsetHigh = 0;
	gOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pLength != NULL)*pLength = 0;

	// attempt an asynchronous read operation
	bResult = ReadFile(handle, pData, dwLen, &nBytesRead, &gOverlapped);

	if (!bResult)
	{
		// deal with the error code
		switch (GetLastError())
		{
		case ERROR_HANDLE_EOF:
		{
			// we have reached the end of the file
			// during the call to ReadFile
			break;
		}
		case ERROR_IO_PENDING:
		{
			// asynchronous i/o is still in progress
			switch (WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
			{
			case WAIT_OBJECT_0:
				// check on the results of the asynchronous read
				// and update the nBytesRead...
				bResult = GetOverlappedResult(handle, &gOverlapped,
					&nBytesRead, FALSE);
				if (!bResult)
				{
					//printf("Error: %d"), GetLastError());
				}
				else
				{
					if (pLength != NULL)
						*pLength = nBytesRead;
					dwResult = ICNCUSB_SUCCESS;
				}//end if else
				break;
			case WAIT_TIMEOUT:
				KillEP2Thread();
				CancelIo(handle);
				break;
			default: //switch(WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
				KillEP2Thread();
				CancelIo(handle);
				break;
			}//end switch
			break;  //switch (GetLastError()) case ERROR_IO_PENDING
		}//end case
		default:    //switch (GetLastError())
			KillEP2Thread();
			CancelIo(handle);
			break;
		}//end switch
	}
	else
	{
		if (pLength != NULL)
			*pLength = nBytesRead;
		dwResult = ICNCUSB_SUCCESS;
	}//end if else

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	return dwResult;
}//end MPUSBRead


DWORD WinUSB_MPUSBRead(HANDLE pEPInfo,          // Input, NOTE: pEPInfo is the pEP that got passed to the MPUSBOpen() call.  It is not a real HANDLE, it originates as a PCHAR
	PVOID pData,            // Output
	DWORD dwLen,            // Input
	PDWORD pLength,         // Output
	DWORD dwMilliseconds)   // Input
{
	BOOL bResult;
	DWORD nBytesRead = 0;
	OVERLAPPED gOverlapped;
	DWORD dwResult;
	UCHAR PipeID;
	dwResult = ICNCUSB_FAIL;

	if (pLength != NULL)*pLength = 0;

	//Make sure pData pointer and pEPInfo "handle" are valid before doing anything
	if ((pData == NULL) || (pEPInfo == INVALID_HANDLE_VALUE))
	{
		SetLastError(ERROR_INVALID_DATA);
		return ICNCUSB_FAIL;
	}

	//First, figure out the PipeID (used by WinUsb_ReadPipe()) by using the data in the pEPInfo. (which is interpreted in the MPUSBOpen() function)
	PipeID = (UCHAR)pEPInfo;


	// set up overlapped structure fields
	gOverlapped.Internal = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset = 0;
	gOverlapped.OffsetHigh = 0;
	gOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// attempt an asynchronous read operation
	if (GlobalWinUSBInterfaceHandle!=INVALID_HANDLE_VALUE)
		bResult = WinUsbDLL_ReadPipe((WINUSB_INTERFACE_HANDLE)GlobalWinUSBInterfaceHandle, PipeID, (PUCHAR)pData, dwLen, &nBytesRead, &gOverlapped);
	else
		return ICNCUSB_FAIL;
		

	if (!bResult)
	{
		// deal with the error code
		DWORD err = GetLastError();
		switch (err)
		{
		case ERROR_HANDLE_EOF:
		{
			// we have reached the end of the file
			// during the call to ReadFile
			break;
		}
		case 0:
		case ERROR_IO_PENDING:
		{
			// asynchronous i/o is still in progress
			switch (WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
			{
			case WAIT_OBJECT_0:
				// check on the results of the asynchronous read
				// and update the nBytesRead...
				bResult = GetOverlappedResult(GlobalWinUSBInterfaceHandle, &gOverlapped,
					&nBytesRead, FALSE);
				if (!bResult)
				{
					//printf("Error: %d"), GetLastError());
				}
				else
				{
					if (pLength != NULL)
						*pLength = nBytesRead;
					dwResult = ICNCUSB_SUCCESS;
				}//end if else
				break;
			case WAIT_TIMEOUT:
				//KillEP2Thread();
				WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
				break;
			default:
				//KillEP2Thread();
				WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
				break;
			}//end switch
			break;
		}//end case
		default:
			//KillEP2Thread();
			WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
			break;
		}//end switch
	}
	else
	{
		if (pLength != NULL)
			*pLength = nBytesRead;
		dwResult = ICNCUSB_SUCCESS;
	}//end if else

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	return dwResult;
}//end WinUSB_MPUSBRead

///////////////////////////////////////////////////////////////////////////////
//	MPUSBWrite :
//
//  handle  - Identifies the endpoint pipe to be written to. The pipe handle
//            must have been created with MP_WRITE access attribute.
//
//  pData   - Points to the buffer containing the data to be written to the pipe.
//
//  dwLen   - Specifies the number of bytes to write to the pipe.
//
//  pLength - Points to the number of bytes written by this function call.
//            MPUSBWrite sets this value to zero before doing any work or
//            error checking.
//
//  dwMilliseconds
//          - Specifies the time-out interval, in milliseconds. The function
//            returns if the interval elapses, even if the operation is
//            incomplete. If dwMilliseconds is zero, the function tests the
//            data pipe and returns immediately. If dwMilliseconds is INFINITE,
//            the function's time-out interval never elapses.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
DWORD MPUSBWrite(PVOID pData,           // Input
	DWORD dwLen,           // Input
	PDWORD pLength,        // Output
	DWORD dwMilliseconds)  // Input
{

	HANDLE handle = ICNC_myOutPipe;

	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		DWORD err = WinUSB_MPUSBWrite(handle, pData, dwLen, pLength, dwMilliseconds);
		return err;
	}

	BOOL bResult;
	DWORD nBytesWritten;
	OVERLAPPED gOverlapped;
	DWORD dwResult;

	dwResult = ICNCUSB_FAIL;

	// set up overlapped structure fields
	gOverlapped.Internal = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset = 0;
	gOverlapped.OffsetHigh = 0;
	gOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pLength != NULL)*pLength = 0;

	// attempt an asynchronous write operation
	bResult = WriteFile(handle, pData, dwLen, &nBytesWritten, &gOverlapped);

	if (!bResult)
	{
		// deal with the error code
		switch (GetLastError())
		{
		case ERROR_HANDLE_EOF:
		{
			// we have reached the end of the file
			// during the call to ReadFile
			break;
		}
		case ERROR_IO_PENDING:
		{
			// asynchronous i/o is still in progress
			switch (WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
			{
			case WAIT_OBJECT_0:
				// check on the results of the asynchronous read
				// and update the nBytesWritten...
				bResult = GetOverlappedResult(handle, &gOverlapped,
					&nBytesWritten, FALSE);
				if (!bResult)
				{
					//printf("Error: %d"), GetLastError());
				}
				else
				{
					if (pLength != NULL)
						*pLength = nBytesWritten;
					dwResult = ICNCUSB_SUCCESS;
				}//end if else
				break;
			case WAIT_TIMEOUT:
				CancelIo(handle);
				break;
			default:
				CancelIo(handle);
				break;
			}//end switch
			break;
		}//end case
		default:
			CancelIo(handle);
			break;
		}//end switch
	}
	else
	{
		if (pLength != NULL)
			*pLength = nBytesWritten;
		dwResult = ICNCUSB_SUCCESS;
	}//end if else

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	FlushSendData();

	return dwResult;

}//end MPUSBWrite


DWORD WinUSB_MPUSBWrite(HANDLE pEPInfo,         // Input, NOTE: This is not a real "HANDLE", it is a uchar that contains the endpoint number returned by the MPUSBOpen() function.
	PVOID pData,           // Input
	DWORD dwLen,           // Input
	PDWORD pLength,        // Output
	DWORD dwMilliseconds)  // Input
{
	BOOL bResult;
	ULONG nBytesWritten = 0;
	OVERLAPPED gOverlapped;
	DWORD dwResult;
	dwResult = ICNCUSB_FAIL;
	UCHAR PipeID;

	if (pLength != NULL)
		*pLength = (DWORD)0;

	//Make sure pData pointer and pEPInfo "handle" are valid before doing anything
	if ((pData == NULL) || (pEPInfo == INVALID_HANDLE_VALUE))
	{
		SetLastError(ERROR_INVALID_DATA);
		return ICNCUSB_FAIL;
	}

	//First, figure out the PipeID (used by WinUsb_WritePipe()) by using the data in the pEPInfo. (which is interpreted in the MPUSBOpen() function)
	PipeID = (UCHAR)pEPInfo;
	ZeroMemory(&gOverlapped, sizeof(gOverlapped));
	// set up overlapped structure fields
	/*gOverlapped.Internal = 0;
	gOverlapped.InternalHigh = 0;
	gOverlapped.Offset = 0;
	gOverlapped.OffsetHigh = 0;*/
	gOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	if (GlobalWinUSBInterfaceHandle==INVALID_HANDLE_VALUE)
		return ICNCUSB_FAIL;

	// attempt an asynchronous write operation
	bResult = WinUsbDLL_WritePipe(GlobalWinUSBInterfaceHandle, PipeID, (PUCHAR)pData, (ULONG)dwLen, &nBytesWritten, NULL/*&gOverlapped*/);
	if (!bResult)
	{
		// deal with the error code
		DWORD err = GetLastError();
		switch (err)
		{
		case ERROR_HANDLE_EOF:
		{
			// we have reached the end of the file
			// during the call to ReadFile
			break;
		}
		case ERROR_IO_PENDING:
			//case 0:
		{
			// asynchronous i/o is still in progress
			switch (WaitForSingleObject(gOverlapped.hEvent, dwMilliseconds))
			{
			case WAIT_OBJECT_0:
				// check on the results of the asynchronous read
				// and update the nBytesWritten...
				bResult = GetOverlappedResult(GlobalWinUSBInterfaceHandle, &gOverlapped,
					&nBytesWritten, FALSE);
				if (!bResult)
				{
					//printf("Error: %d"), GetLastError());
				}
				else
				{
					if (pLength != NULL)
						*pLength = (DWORD)nBytesWritten;
					dwResult = ICNCUSB_SUCCESS;
				}//end if else
				break;
			case WAIT_TIMEOUT:
				WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
				break;
			default:
				WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
				break;
			}//end switch
			break;
		}//end case
		default:
			try {
				WinUsbDLL_AbortPipe(GlobalWinUSBInterfaceHandle, PipeID);
			}
			catch (...)
			{
			}
			break;
		}//end switch
	}
	else
	{
		if (pLength != NULL)
			*pLength = (DWORD)nBytesWritten;
		dwResult = ICNCUSB_SUCCESS;
	}//end if else

	ResetEvent(gOverlapped.hEvent);
	CloseHandle(gOverlapped.hEvent);

	FlushSendData();


	return dwResult;

}//end WinUSB_MPUSBWrite




///////////////////////////////////////////////////////////////////////////////
//	MPUSBClose : closes a given handle.
//
//	Note that "input" and "output" refer to the parameter designations in calls
//	to this function, which are the opposite of common sense from the
//	perspective of an application making the calls.
//
BOOL MPUSBClose(HANDLE handle)
{


	//If WinUSB was detected, use the WinUSB flavor of this function instead.
	if (UsingWinUSBDriver == TRUE)
	{
		return WinUSB_MPUSBClose(handle);
	}

	BOOL toReturn;
	toReturn = true;

	if (handle != INVALID_HANDLE_VALUE)
		toReturn = CloseHandle(handle);
	return toReturn;
}//end MPUSBClose

BOOL WinUSB_MPUSBClose(HANDLE handle)
{
	if ((ApparentOpenPipes == 0) || ((ApparentOpenPipes - 1) == 0))
	{
		ApparentOpenPipes = 0;
		if (WinUSBInterfaceInitialized == TRUE)
		{
			WinUSBInterfaceInitialized = FALSE;
			if (GlobalWinUSBInterfaceHandle != INVALID_HANDLE_VALUE) {
				if (WinUsbDLL_Free(GlobalWinUSBInterfaceHandle))
				{
					GlobalWinUSBInterfaceHandle = INVALID_HANDLE_VALUE;
					if (CloseHandle(GlobalWinUSBDeviceHandle))
					{
						GlobalWinUSBDeviceHandle = INVALID_HANDLE_VALUE;
						return true;    //successfully closed both device and interface handles
					}
				}
			}
		}
		return false; //For some reason couldn't properly close the WinUSB interface handle...
	}

	ApparentOpenPipes--;
	return true;

}//end WinUSB_MPUSBClose
/////////////////////////////////////////////////////////////////////////////
//
// A typical application would send a command to the target device and expect
// a response.
// SendReceivePacket is a wrapper function that facilitates the
// send command / read response paradigm
//
// SendData - pointer to data to be sent
// SendLength - length of data to be sent
// ReceiveData - Points to the buffer that receives the data read from the call
// ReceiveLength - Points to the number of bytes read
// SendDelay - time-out value for MPUSBWrite operation in milliseconds
// ReceiveDelay - time-out value for MPUSBRead operation in milliseconds
//


//---------------------------------------------------------------------------

void CheckInvalidHandle(void)
{
	if (GetLastError() == ERROR_INVALID_HANDLE)
	{
		// Most likely cause of the error is the board was disconnected.
		MPUSBClose(ICNC_myOutPipe);
		MPUSBClose(ICNC_myInPipe);
		ICNC_myOutPipe = ICNC_myInPipe = INVALID_HANDLE_VALUE;
		ICNC_myOutPipe = MPUSBOpen(0, ICNC_vid_pid, ICNC_out_pipe, MP_WRITE, 0);
		ICNC_myInPipe = MPUSBOpen(0, ICNC_vid_pid, ICNC_in_pipe, MP_READ, 0);


#if USING_EP2==1
		MPUSBClose(ICNC_myInPipeEvent);
		ICNC_myInPipeEvent = INVALID_HANDLE_VALUE;
		ICNC_myInPipeEvent = MPUSBOpen(0, ICNC_vid_pid, ICNC_inevent_pipe, MP_READ, 0);
#endif
		
		USBConnected = ((ICNC_myOutPipe != INVALID_HANDLE_VALUE) && (ICNC_myInPipe != INVALID_HANDLE_VALUE));
		return;


		//if (ICNC_myOutPipe == INVALID_HANDLE_VALUE || ICNC_myInPipe == INVALID_HANDLE_VALUE || ICNC_myInPipeEvent == INVALID_HANDLE_VALUE )
		//{
		//	return;
		//}



	}//end if

}//end CheckInvalidHandle


DWORD SendReceivePacket(BYTE* SendData, DWORD SendLength, BYTE* ReceiveData,
	DWORD* ReceiveLength, UINT SendDelay, UINT ReceiveDelay)
{
	DWORD SentDataLength;
	DWORD ExpectedReceiveLength = *ReceiveLength;

	if (ICNC_myOutPipe != INVALID_HANDLE_VALUE && ICNC_myInPipe != INVALID_HANDLE_VALUE)
	{
		if (MPUSBWrite(SendData, SendLength, &SentDataLength, SendDelay))
		{

			if (MPUSBRead(ICNC_myInPipe, ReceiveData, ExpectedReceiveLength,
				ReceiveLength, ReceiveDelay))
			{
				if (*ReceiveLength == ExpectedReceiveLength)
				{
					return 1;   // Success!
				}
				else if (*ReceiveLength < ExpectedReceiveLength)
				{
					return 2;   // Partially failed, incorrect receive length
				}//end if else
			}
			else
				CheckInvalidHandle();
		}
		else
			CheckInvalidHandle();
	}//end if

	return 0;  // Operation Failed


}//end SendReceivePacket
