// ICNC2.cpp : implementation file
//


#include "stdafx.h"
#include "stdlib.h"


#include <stdio.h>
#include <sysinfoapi.h>

#include <TCHAR.H> 

//#include <windows.h>
//#include <string.h> 
#include <setupapi.h>
#include <initguid.h>


//#include <winioctl.h>
//#include "ioctls.h"
#include "ICNC2.h"
#include "USBCom.h"

#include "CKEtimer.h"

#include <assert.h>
#include <math.h>

//#include <winusb.h>


#define MPUSBAPI_VERSION            0x02020000L     //revision 6.0.0.0 This version merges WinUSB + Microchip custom class driver functionality.  This version of mpusbapi.dll will work with either driver installed.

#define SEUIL_LOCAL_BUFF	50		// seuil remplissage buffer local avant envoie USB
CONSOLE_HANDLER DebugHandler = NULL;

#define MIN(A,B) (A>B?B:A)



#pragma hdrstop

// Private functions 
DWORD ICNC_SetVerboseMode(DWORD Verbose);
DWORD ICNC_SetUSBSpeedMode(BOOL EnableHighSpeed);


DWORD ICNC_Number_connected = 0;


BOOL Connected = FALSE;

HANDLE ICNC2Mutex;


// Global Vars


DWORD UserApplicationID = 0;

#define SendDataSize SendData[0]
BYTE indexSendData = 0;

#pragma pack(push, 1)
BYTE SendData[4096];
BYTE ReceiveData[64];
#pragma pack(pop)

DWORD ReceiveLength = 0;

BYTE indexReceiveData = 1;

int SimulationMode;

SSimulation Simulation;



#pragma pack(push, 1)
TCHAR DebugBuffer[1024];
#pragma pack(pop)

CKETimer aTimer;  // AccurateTimer

void FlushSendData(void);




unsigned long InitDLL()
{

	ICNC2Mutex = CreateMutex(NULL, FALSE, _T("ICNC2Mutex"));

	if (!ICNC2Mutex) {
		return 0;
	}

	USBInitialisation();


	aTimer.InitTimer(FALSE);// Initialisation Accurate timer

	Simulation.PosX = Simulation.PosY = Simulation.PosZ = Simulation.PosA = Simulation.PosB = 0;
	Simulation.Input = 0;
	Simulation.Output = 0;
	SimulationMode = 0;
	Simulation.StatusBoard = 0;


	memset(Simulation.UserMem, 0, sizeof(Simulation.UserMem));
	memset(Simulation.Parameters, 0, sizeof(Simulation.Parameters));
	Simulation.LastProbe = 0;
	FlushSendData();
	DebugHandler = NULL;

#ifdef DEBUG
	ICNC_SetVerboseMode(2);
#else
	ICNC_SetVerboseMode(0);
#endif

	return 0;
}


void DebugHandlerFctn(PTCHAR buf, PVOID Arg = NULL)
{
	TCHAR CRFF[2] = _T("\n");
	//        Application->MessageBoxA(buf, "Debug", MB_OK	);

/*
		try {
				FileDebug = new TFileStream("ICNC2DebugFile.txt", fmOpenReadWrite);
		}
		catch(...)
		{
		}

		FileDebug->Seek(0, soFromEnd );
*/
/*
if (FileDebug!=NULL) {

		CString s;
		FileDebug->Write(buf, (UINT)strlen(buf));
		FileDebug->Write(CRFF, 2);
}
*/
	OutputDebugString(buf);
	OutputDebugString(_T("\n"));
	//return 1;
	/*
			FileDebug->Free();
	*/

}


DWORD GetDiffTickCount(void) {
	return (DWORD)aTimer.GetTotalElapsedTime();
}


void FlushSendData(void) {
	SendDataSize = 0;
	indexSendData = 1;
}



int indexSendDataSubFrame[16];

void FlushSendDataSubFrame(int SubFrameNo) {
	SendData[SubFrameNo * 64] = 0;
	indexSendDataSubFrame[SubFrameNo] = SubFrameNo * 64 + 1;
}


void InitReceiveData(void)
{
	indexReceiveData = 1;
}

void Push32(int value) {
	SendData[indexSendData++] = BYTE(value);
	SendData[indexSendData++] = BYTE(value >> 8);
	SendData[indexSendData++] = BYTE(value >> 16);
	SendData[indexSendData++] = BYTE(value >> 24);
	SendDataSize += 4;
}

void Push24(int value) {
	SendData[indexSendData++] = BYTE(value);
	SendData[indexSendData++] = BYTE(value >> 8);
	SendData[indexSendData++] = BYTE(value >> 16);
	SendDataSize += 3;
}

void Push16(int value) {
	SendData[indexSendData++] = BYTE(value);
	SendData[indexSendData++] = BYTE(value >> 8);
	SendDataSize += 2;
}

void Push8(int value) {
	SendData[indexSendData++] = BYTE(value);
	SendDataSize += 1;
}


void Push16SubFrame(int value, int FrameNo) {
	SendData[indexSendDataSubFrame[FrameNo]++] = BYTE(value);
	SendData[indexSendDataSubFrame[FrameNo]++] = BYTE(value >> 8);
	SendData[FrameNo * 64] += 2;
}

void Push8SubFrame(int value, int FrameNo) {
	SendData[indexSendDataSubFrame[FrameNo]++] = BYTE(value);
	SendData[FrameNo * 64]++;
}

BYTE Pop8() {
	return (BYTE)ReceiveData[indexReceiveData++];
}

int Pop16() {
	int a, b, result;
	a = ReceiveData[indexReceiveData++];
	b = ReceiveData[indexReceiveData++];
	result = (b << 8) | a;
	return result;
	//return (int)ReceiveData[indexReceiveData++] | ((int)ReceiveData[indexReceiveData++]<<8) ;
}

int Pop32() {
	int a, b, c, d, result;
	a = ReceiveData[indexReceiveData++];
	b = ReceiveData[indexReceiveData++];
	c = ReceiveData[indexReceiveData++];
	d = ReceiveData[indexReceiveData++];

	result = (d << 24) | (c << 16) | (b << 8) | a;
	return result;

}


// Verbose = 1 => Affichage dans écran moniteur
// Verbose = 2 => Affichage dans écran moniteur et sauvegarde dans fichier
DWORD  ICNC_SetVerboseMode(DWORD Verbose)
{

	// 1 => remplacement fichier debug si existant et Activation
	if (Verbose == 1) {
		DebugHandler = &DebugHandlerFctn;
	}
	else if (Verbose == 2) {
		try {
			//FileDebug = new CFile(/*ExtractFilePath(Application ->ExeName) + */"c://mach3//ICNC2DebugFile.txt", CFile::modeWrite    | CFile::modeCreate   | CFile::shareDenyNone    );
		}
		catch (...)
		{
		}
		//FileDebug->Free();

		DebugHandler = &DebugHandlerFctn;
	}
	else DebugHandler = NULL;
	return TRUE;
}





extern "C" {
	DllExport DWORD ICNC_GetDLLVersion(void)
	{
		return MPUSBAPI_VERSION;
	} //end MPUSBGetDLLVersion
	DllExport DWORD  ICNC_SetSimulationMode(BOOL SimulationModeVal)
{
	SimulationMode = SimulationModeVal;
	//SimulationMode = 1;
	return ICNCUSB_SUCCESS;
}
	DllExport DWORD  ICNC_Connect(DWORD ICNC_Number)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 3000))
		{
			return ICNCUSB_FAIL;
		}

		if (USBConnect(ICNC_Number) == ICNCUSB_SUCCESS) {
			Connected = TRUE;
			if (DebugHandler != NULL) {
				_stprintf_s(DebugBuffer, _T("%6d Connect success"), GetDiffTickCount());
				DebugHandler(DebugBuffer, NULL);
			}
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_SUCCESS;
		}
		else {
			Connected = FALSE;
			if (DebugHandler != NULL) {
				_stprintf_s(DebugBuffer, _T("%6d Connect failed"), GetDiffTickCount());
				DebugHandler(DebugBuffer, NULL);
			}
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_FAIL;
		}
	}
	DllExport DWORD  ICNC_Disconnect(void)
	{
		// Always check to close all handles before exiting!
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d Disconnect"), GetDiffTickCount());
			DebugHandler(DebugBuffer, NULL);
		}

		if (SimulationMode) return ICNCUSB_SUCCESS;
		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 3000))
		{
			ReleaseMutex(ICNC2Mutex);
		}

		USBDisconnect();
		ReleaseMutex(ICNC2Mutex);

		Connected = FALSE;
		return ICNCUSB_SUCCESS;
	}
	DllExport DWORD  ICNC_SetUserApplicationID(DWORD lUserApplicationID)
{

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d SetUserApplication %d"), GetDiffTickCount(), lUserApplicationID);
		DebugHandler(DebugBuffer, NULL);
	}

	UserApplicationID = lUserApplicationID;
	return  ICNCUSB_SUCCESS;
}
	DllExport DWORD  ICNC_ErrorReset(DWORD ErrorBits)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d ErrorReset 0x%X"), GetDiffTickCount(), ErrorBits);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS) {
		return result;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_CMD_ERROR_RESET); //
	Push32(ErrorBits);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;


}

// Commandes NON bufferisées
// ******************************************************************
	DllExport DWORD  ICNC_GetBoardStatus(DWORD StatusType, DWORD* Status)
	{
		int i, result, len = 0;
		int CntStatusValue = 0;
		DWORD* ptr = Status;

		if (DebugHandler != NULL) {
			len = _stprintf_s(DebugBuffer, _T("%6d GetBoardStatus 0x%8X = "), GetDiffTickCount(), StatusType);
			//DebugHandler(DebugBuffer, NULL);
		}


		if (SimulationMode) {
			if (StatusType & ICNC_STATUS_BOARD_STATUS)
				*ptr++ = Simulation.StatusBoard;
			if (StatusType & ICNC_STATUS_BUFFER_FREE)
				*ptr++ = SIMUL_BUFFER_SIZE;
			if (StatusType & ICNC_STATUS_ACTUALX)
				*ptr++ = Simulation.PosX;
			if (StatusType & ICNC_STATUS_ACTUALY)
				*ptr++ = Simulation.PosY;
			if (StatusType & ICNC_STATUS_ACTUALZ)
				*ptr++ = Simulation.PosZ;
			if (StatusType & ICNC_STATUS_ACTUALA)
				*ptr++ = Simulation.PosA;
			if (StatusType & ICNC_STATUS_ACTUALB)
				*ptr++ = Simulation.PosB;
			if (StatusType & ICNC_STATUS_INPUT)
				*ptr++ = Simulation.Input;
			if (StatusType & ICNC_STATUS_LAST_PROBE)
				*ptr++ = Simulation.LastProbe;
			if (StatusType & ICNC_STATUS_ANALOG_IN1)
				*ptr++ = 0;
			if (StatusType & ICNC_STATUS_ANALOG_IN2)
				*ptr++ = 0;
			if (StatusType & ICNC_STATUS_ANALOG_IN3)
				*ptr++ = 0;
			if (StatusType & ICNC_STATUS_ANALOG_IN4)
				*ptr++ = 0;
			if (StatusType & ICNC_STATUS_THC_TARGET)
				*ptr++ = 0;

			return ICNCUSB_SUCCESS;
		}

		if (!Connected) return ICNCUSB_FAIL;
		if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
			return result;

		for (i = 0; i < 32; i++)
		{
			if (((StatusType >> i) & 0x01) != 0) CntStatusValue++;
		}

		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
		{
			return ICNCUSB_FAIL;
		}

		ReceiveLength = 2 + CntStatusValue * 4;
		FlushSendData();
		Push8(ICNC_CMD_GET_STATUS); //
		Push32(StatusType);

		result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		if (result == ICNCUSB_SUCCESS)
		{
			InitReceiveData();
			if (Pop8() != ICNC_CMD_GET_STATUS) return ICNCUSB_COM_ERROR;
			for (i = 0; i < CntStatusValue; i++)
			{
				*ptr = Pop32();
				ptr++;
			}
			ReleaseMutex(ICNC2Mutex);
			if (DebugHandler != NULL) {
				ptr = Status;
				/* // Avec détails
				if (StatusType & ICNC_STATUS_BOARD_STATUS) {
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_BOARD_STATUS = 0x%X", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}

				if (StatusType & ICNC_STATUS_BUFFER_FREE){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_BUFFER_FREE = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ACTUALX){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ACTUALX = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ACTUALY){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ACTUALY = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ACTUALZ){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ACTUALZ = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ACTUALA){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ACTUALA = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ACTUALB){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ACTUALB = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_INPUT){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_INPUT = 0x%X", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_LAST_PROBE){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_LAST_PROBE = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN0){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ANALOG_IN0 = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN1){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ANALOG_IN1 = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN2){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ANALOG_IN2 = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN3){
						_stprintf_s(DebugBuffer, "               ICNC_STATUS_ANALOG_IN3 = %d", *(DWORD*)ptr++);
						DebugHandler(DebugBuffer, NULL);
				}
				*/
				// Sans détails

				if (StatusType & ICNC_STATUS_BOARD_STATUS) {
					DWORD NewState = *(DWORD*)ptr++;
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" STS=0x%X"), NewState);
				}

				if (StatusType & ICNC_STATUS_BUFFER_FREE) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" BUF=%d"), *(DWORD*)ptr++);
				}

				if (StatusType & ICNC_STATUS_ACTUALX) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" X=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ACTUALY) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Y=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ACTUALZ) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Z=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ACTUALA) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" A=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ACTUALB) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" B=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_INPUT) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" IN=0x%X"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_LAST_PROBE) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" PROBE=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN1) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AIN1=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN2) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AIN2=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN3) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AIN3=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_ANALOG_IN4) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AIN4=%d"), *(DWORD*)ptr++);
				}
				if (StatusType & ICNC_STATUS_THC_TARGET) {
					len += _stprintf_s(&DebugBuffer[len], 1024, _T(" THCTarget=%d"), *(DWORD*)ptr++);
				}



				DebugHandler(DebugBuffer, NULL);


			}


			return result;
		}
		else {        // Cas d'erreur
			ReleaseMutex(ICNC2Mutex);
			return result;
		}

	}
	DllExport DWORD  ICNC_SetOverrideValue(DWORD OverrideValue) // Input
{
	DWORD DataSent;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetOverrideValue %d"), GetDiffTickCount(), OverrideValue);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	int result;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS) {
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d !!!!! Erreur SetOverride->ICNC_FlushLocalBuf"), GetDiffTickCount());
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d !!!!! Erreur SetOverride->WaitForSingleObject"), GetDiffTickCount());
			DebugHandler(DebugBuffer, NULL);
		}
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_CMD_SET_OVERRIDE); //
	Push32(OverrideValue); // Max( OverrideValue, 1) );
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	if (result != ICNCUSB_SUCCESS) {
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d !!!!! Erreur SetOverride"), GetDiffTickCount());
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else {
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d +++ SetOverride success"), GetDiffTickCount());
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}


}
	DllExport DWORD  ICNC_GetOverrideValue(DWORD* OverrideValue) // Outpur
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*OverrideValue = 0;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_GET_OVERRIDE); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_OVERRIDE) return ICNCUSB_COM_ERROR;

		*OverrideValue = Pop32();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d GetOverrideValue = %d"), GetDiffTickCount(), *OverrideValue);
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}
	DllExport DWORD  ICNC_GetOutputAll(DWORD* OutputState) // Output
{
	int result;

	if (SimulationMode) {
		*OutputState = Simulation.Output;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*OutputState = 0;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_GET_OUTPUT_ALL); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_OUTPUT_ALL) return ICNCUSB_COM_ERROR;

		*OutputState = Pop32();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d GetOutputAll = 0x%X"), GetDiffTickCount(), *OutputState);
			DebugHandler(DebugBuffer, NULL);
		}

		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}
	DllExport DWORD  ICNC_GetInput(DWORD InNo, // Input
	BOOL* InputState)
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*InputState = 0;


	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 1;
	FlushSendData();
	Push8(ICNC_CMD_GET_INPUT); //
	Push8(InNo); //   No of input to read


	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_INPUT) return ICNCUSB_COM_ERROR;

		*InputState = Pop8();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d GetInput N°%d = %d"), GetDiffTickCount(), InNo, *InputState);
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}
	DllExport DWORD  ICNC_GetInputAll(DWORD* InputState) // Output
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*InputState = 0;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_GET_INPUT_ALL); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_INPUT_ALL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}

		*InputState = Pop32();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d GetInputAll = 0x%X"), GetDiffTickCount(), *InputState);
			DebugHandler(DebugBuffer, NULL);
		}

		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}
DllExport DWORD  ICNC_GetAnalog(DWORD AnalogNo, // Input
	DWORD* AnalogValue)
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*AnalogValue = 0;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 2;
	FlushSendData();
	Push8(ICNC_CMD_GET_ANALOG); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_ANALOG) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}

		*AnalogValue = Pop16();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d GetAnalog N°%d = %d"), GetDiffTickCount(), AnalogNo, *AnalogValue);
			DebugHandler(DebugBuffer, NULL);
		}

		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}


DllExport DWORD  ICNC_SetOutput(DWORD OutputNo,
	BOOL OutputState)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d SetOutput %d to %d"), GetDiffTickCount(), OutputNo, OutputState);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		if (OutputState == FALSE) Simulation.Output &= ~(1 << (OutputNo - 1));
		else Simulation.Output |= (1 << (OutputNo - 1));
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_SET_OUTPUT); //
	Push8(OutputNo); //
	Push8(OutputState); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}

DllExport DWORD  ICNC_SetAnalog(DWORD AnalogNo,
	DWORD AnalogValue)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d SetAnalog N°%d to %d"), GetDiffTickCount(), AnalogNo, AnalogValue);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_SET_ANALOG); //
	Push8(AnalogNo); //
	//Push16(AnalogValue * AnalogValue*AnalogValue/1024/1024); //
	Push16(AnalogValue);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD  ICNC_GetBufferStatus(DWORD* FreeBufferSpace) // Output
{
	int result;

	if (SimulationMode) {

		*FreeBufferSpace = SIMUL_BUFFER_SIZE;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	*FreeBufferSpace = 0;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	ReceiveLength = 2 + 2;
	FlushSendData();
	Push8(ICNC_CMD_GET_BUFFER_STATUS); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_BUFFER_STATUS) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}

		*FreeBufferSpace = Pop16();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d GetBufferStatus = %d"), GetDiffTickCount(), *FreeBufferSpace);
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}


DllExport DWORD  ICNC_Unfreeze(void)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d Unfreeze"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;


	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_UNFREEZE); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD  ICNC_WriteUserMem(DWORD MemoryNumber,
	DWORD MemoryValue)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d WriteUserMem N°%d to %d"), GetDiffTickCount(), MemoryNumber, MemoryValue);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_WRITE_USER_MEM); //
	Push8(MemoryNumber);
	Push32(MemoryValue);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}


DllExport DWORD  ICNC_ReadUserMem(DWORD MemoryNumber,
	DWORD* MemoryValue)
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*MemoryValue = 0;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_READ_USER_MEM); //
	Push8(MemoryNumber); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_READ_USER_MEM) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}

		*MemoryValue = Pop32();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d ReadUserMem N°%d to %d"), GetDiffTickCount(), MemoryNumber, *MemoryValue);
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}



DllExport DWORD  ICNC_ReadEEPROM(DWORD StartAdress,
	DWORD Length,
	BYTE* Value)
{
	int i, result, len = 0;
	BYTE* ptr = Value;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (Length > 62) return ICNCUSB_FAIL;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	ReceiveLength = 2 + Length;
	FlushSendData();
	Push8(ICNC_CMD_READ_EEPROM); //
	Push8(Length); //
	Push32(StartAdress);

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_READ_EEPROM) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		for (i = 0; i < (int)Length; i++)
		{
			*ptr = Pop8();
			ptr++;
		}
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			len = _stprintf_s(DebugBuffer, 1024, _T("%6d Read EEPROM from %d to %d :"), GetDiffTickCount(), StartAdress, StartAdress + Length);
			ptr = Value;
			for (i = 0; i < (int)Length; i++) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" 0x%X"), *ptr++);
			DebugHandler(DebugBuffer, NULL);
		}


		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}


DllExport DWORD  ICNC_WriteEEPROM(DWORD StartAdress,
	DWORD Length,
	BYTE* Value,
	BOOL WaitCompletion,
	DWORD TimeOut)
{
	int i, result, len = 0;
	BYTE* ptr = Value;
	DWORD DataSent;
	DWORD tick;
	DWORD PrevTickCount;
	DWORD Status;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (Length > 60) return ICNCUSB_FAIL;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_WRITE_EEPROM); //
	Push8(Length);
	Push32(StartAdress);
	for (i = 0; i < (int)Length; i++)
	{
		Push8(*ptr);
		ptr++;
	}
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);

	if (DebugHandler != NULL) {
		len = _stprintf_s(DebugBuffer, 1024, _T("%6d Write EEPROM from %d to %d :"), GetDiffTickCount(), StartAdress, StartAdress + Length);
		ptr = Value;
		for (i = 0; i < (int)Length; i++) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" 0x%X"), *ptr++);
		DebugHandler(DebugBuffer, NULL);
	}


	if (result != ICNCUSB_SUCCESS) {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}

	if (!WaitCompletion) {
		return result;
		ReleaseMutex(ICNC2Mutex);
	}


	// Wait for write completion
	tick = GetTickCount() + TimeOut;
	PrevTickCount = GetTickCount() + 1;
	Status = ICNC_STATUS_BIT_EEWRITE_INPROGRESS;

	do {
		// Vérifier toutes les 5ms la fin d'écriture
		if (GetTickCount() >= PrevTickCount)
		{
			result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
			if (result != ICNCUSB_SUCCESS) {
				return result;
				ReleaseMutex(ICNC2Mutex);
			}
			PrevTickCount = GetTickCount() + 5;
		}

	} while ((GetTickCount() < tick) && ((Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS) != 0));
	ReleaseMutex(ICNC2Mutex);

	if ((Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS) == 0) // Ecriture terminée
	{
		if ((Status & ICNC_STATUS_BIT_EEWRITE_ERROR) == 0) // et sans erreur
			return ICNCUSB_SUCCESS;
		else return ICNCUSB_FAIL;
	}
	else return ICNCUSB_TIMEOUT;
}


DllExport DWORD  ICNC_ReadParameter(DWORD ParameterID,
	INT* ParameterValue)
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	*ParameterValue = 0;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_READ_PARAMETER); //
	Push16(ParameterID); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_READ_PARAMETER) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		*ParameterValue = Pop32();
		ReleaseMutex(ICNC2Mutex);
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, _T("%6d ReadParameter N°%d = %d (0x%X)"), GetDiffTickCount(), ParameterID, *ParameterValue, *ParameterValue);
			DebugHandler(DebugBuffer, NULL);
		}

		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
}

DllExport DWORD  ICNC_WriteParameter(DWORD ParameterID,
	DWORD ParameterValue,
	BOOL WaitCompletion,
	DWORD TimeOut)
{
	int result;
	DWORD PrevTickCount;
	DWORD Status;
	int delay = 0;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d WriteParameter N°%d = %d (0x%X), WaitCompletion=%d"), GetDiffTickCount(), ParameterID, ParameterValue, ParameterValue, WaitCompletion);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (UserApplicationID != 99) {
		// Specific application ID treatement

	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 1000))
	{
		return ICNCUSB_FAIL;

	}

	ReceiveLength = 2 + 4;   // Read back parameter value
	FlushSendData();
	Push8(ICNC_CMD_WRITE_PARAMETER); //
	Push16(ParameterID);
	Push32(ParameterValue);

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_WRITE_PARAMETER) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		if ((DWORD)Pop32() != ParameterValue) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		ReleaseMutex(ICNC2Mutex);
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}

	if (!WaitCompletion) return result;



	PrevTickCount = (DWORD)aTimer.GetTotalElapsedTime() + TimeOut;
	Status = ICNC_STATUS_BIT_EEWRITE_INPROGRESS;
	aTimer.SleepNow(2);


	do {
		result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
		if (result != ICNCUSB_SUCCESS) return result;
		if ((Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS) != 0)
			aTimer.SleepNow(2);
	} while ((aTimer.GetTotalElapsedTime() < PrevTickCount) && ((Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS) != 0));


	/*       // Wait for write completion
		   tick = GetTickCount()+TimeOut;
		   PrevTickCount = GetTickCount()+2;
		   Status=ICNC_STATUS_BIT_EEWRITE_INPROGRESS;

		   do {
			  // Vérifier toutes les 1ms la fin d'écriture
			  delay++;
			  NewTick = GetTickCount();
			  if ( NewTick >= PrevTickCount)
			  {
				   result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
				   if (result != ICNCUSB_SUCCESS)
						   return result;
				   PrevTickCount = NewTick+2;
			  }
		   } while ((NewTick < tick) && !( (Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS)==0));
   */

	if ((Status & ICNC_STATUS_BIT_EEWRITE_INPROGRESS) == 0) // Ecriture terminée
	{
		if ((Status & ICNC_STATUS_BIT_EEWRITE_ERROR) == 0) // et sans erreur
			return ICNCUSB_SUCCESS;
		else {
			if (DebugHandler != NULL) {
				_stprintf_s(DebugBuffer, _T("%6d -->>ERROR during WriteParameter N°%d = %d (0x%X), WaitCompletion=%d"), GetDiffTickCount(), ParameterID, ParameterValue, ParameterValue, WaitCompletion);
				DebugHandler(DebugBuffer, NULL);
			}
			return ICNCUSB_FAIL;
		}

	}
	else return ICNCUSB_TIMEOUT;
}


DllExport DWORD  ICNC_WritePosition(DWORD Axes,
	INT PositionX,
	INT PositionY,
	INT PositionZ,
	INT PositionA,
	INT PositionB)
{
	DWORD DataSent, len;
	int result;


	if (DebugHandler != NULL) {
		len = _stprintf_s(DebugBuffer, _T("%6d WritePosition "), GetDiffTickCount());
		if (Axes & ICNC_AXE_X) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" X=%d"), PositionX);
		if (Axes & ICNC_AXE_Y) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Y=%d"), PositionY);
		if (Axes & ICNC_AXE_Z) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Z=%d"), PositionZ);
		if (Axes & ICNC_AXE_A) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" A=%d"), PositionA);
		if (Axes & ICNC_AXE_B) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" B=%d"), PositionB);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;

	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (Axes == 0) return ICNCUSB_SUCCESS;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_WRITE_POSITION); //
	Push8(Axes);

	if (Axes & ICNC_AXE_B) { Push32(PositionB); }
	if (Axes & ICNC_AXE_A) { Push32(PositionA); }
	if (Axes & ICNC_AXE_Z) { Push32(PositionZ); }
	if (Axes & ICNC_AXE_Y) { Push32(PositionY); }
	if (Axes & ICNC_AXE_X) { Push32(PositionX); }

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}

DllExport DWORD ICNC_MachineHome(DWORD Axes,
	DWORD MaxStrokeX,
	DWORD MaxStrokeY,
	DWORD MaxStrokeZ,
	DWORD MaxStrokeA,
	DWORD MaxStrokeB)
{

	DWORD Status;
	DWORD DataSent, len;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d MachineHome "), GetDiffTickCount());
		len = (DWORD)_tclen(DebugBuffer);
		if (Axes & ICNC_AXE_X) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" XMax=%d"), MaxStrokeX);
		if (Axes & ICNC_AXE_Y) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" YMax=%d"), MaxStrokeY);
		if (Axes & ICNC_AXE_Z) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" ZMax=%d"), MaxStrokeZ);
		if (Axes & ICNC_AXE_A) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AMax=%d"), MaxStrokeA);
		if (Axes & ICNC_AXE_B) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" BMax=%d"), MaxStrokeB);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	// S'assurer q'une OM n'est pas en cours et que le buffer est actuellement vide
	do {
		result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
		if (result != ICNCUSB_SUCCESS) return result;
		if (((Status & ICNC_STATUS_BIT_HOMING) != 0) || ((Status & ICNC_STATUS_BIT_BUFFER_EMPTY) == 0))
		{
			_stprintf_s(DebugBuffer, _T("X"));
			DebugHandler(DebugBuffer, NULL);
			Sleep(5);
		}
	} while (((Status & ICNC_STATUS_BIT_HOMING) != 0) || ((Status & ICNC_STATUS_BIT_BUFFER_EMPTY) == 0));

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_MACHINE_HOME); //
	Push8(Axes);
	if (Axes & ICNC_AXE_B) Push32(MaxStrokeB);
	if (Axes & ICNC_AXE_A) Push32(MaxStrokeA);
	if (Axes & ICNC_AXE_Z) Push32(MaxStrokeZ);
	if (Axes & ICNC_AXE_Y) Push32(MaxStrokeY);
	if (Axes & ICNC_AXE_X) Push32(MaxStrokeX);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);

	return result;
	//aTimer.SleepNow(50);

	// Attente fin de prise d'origine car non bufferisée
	if (result != ICNCUSB_SUCCESS) return result;        // Wait for write completion

	DWORD PrevTickCount = GetTickCount() + 1;
	Sleep(2);


	do {
		result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
		if (result != ICNCUSB_SUCCESS) return result;
		if ((Status & ICNC_STATUS_BIT_HOMING) != 0)
			Sleep(5);
	} while ((Status & ICNC_STATUS_BIT_HOMING) != 0);


	// Fin d'attente



	return result;
}


// Launch independent and asynchrone homing sequence in backgournd for one axe
DllExport DWORD ICNC_AxeHome(DWORD AxeID,
	DWORD InputNumber,
	DWORD ExpectedInputValue,
	DWORD Acceleration_khz_per_s,
	DWORD HighSpeed_Hz,
	DWORD Deceleration_khz_per_s,
	INT   MaxStroke_step,
	DWORD LowSpeed_Hz,
	INT   InitialPosition_step)
{

	DWORD DataSent, len;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d AxeHome "), GetDiffTickCount());
		len = (DWORD)_tclen(DebugBuffer);
		if (AxeID == 1) len += _stprintf_s(&DebugBuffer[len], 1024, _T("  XMax=%d"), MaxStroke_step);
		if (AxeID == 2)  len += _stprintf_s(&DebugBuffer[len], 1024, _T(" YMax=%d"), MaxStroke_step);
		if (AxeID == 3)  len += _stprintf_s(&DebugBuffer[len], 1024, _T(" ZMax=%d"), MaxStroke_step);
		if (AxeID == 4)  len += _stprintf_s(&DebugBuffer[len], 1024, _T(" AMax=%d"), MaxStroke_step);
		if (AxeID == 5)  len += _stprintf_s(&DebugBuffer[len], 1024, _T(" BMax=%d"), MaxStroke_step);
		len += _stprintf_s(&DebugBuffer[len], 1024, _T(" InputNumber=%d Type=%s"), InputNumber,
			ExpectedInputValue == 0 ? "NC" : "NO");
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_HOME_AXE); //
	Push8(AxeID);
	Push8(InputNumber);
	Push8(ExpectedInputValue);
	Push16(Acceleration_khz_per_s);
	Push32(HighSpeed_Hz);
	Push16(Deceleration_khz_per_s);
	Push32(MaxStroke_step);
	Push16(LowSpeed_Hz);
	Push32(InitialPosition_step);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD ICNC_SetTHCTarget(DWORD THCTargetValue)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d SetTHCTarget to %d"), GetDiffTickCount(), THCTargetValue);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	// Ne pas autoriser GALAAD à mettre 0 ou 1024 dans le THC
	// car la commande SetTHC(0) est envoyée en fin d'objet et prise en compte immédiatement
	// et SetTHC(1024) est envoyée en début d'objet


	if (UserApplicationID != 99) {
		if (THCTargetValue == 0) {
			if (DebugHandler != NULL) {
				_stprintf_s(DebugBuffer, 1024, _T("%6d SetTHCTarget Skipped because target is 0"), GetDiffTickCount());
				DebugHandler(DebugBuffer, NULL);
			}
			return ICNCUSB_SUCCESS;
		}
		if (THCTargetValue == 1024) {
			if (DebugHandler != NULL) {
				_stprintf_s(DebugBuffer, 1024, _T("%6d SetTHCTarget Skipped because target is 1024"), GetDiffTickCount());
				DebugHandler(DebugBuffer, NULL);
			}
			return ICNCUSB_SUCCESS;
		}
	}


	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_SET_THC_TARGET); //
	Push32(THCTargetValue); //
	//Push32(400); //

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


// Activation/desactivation logiciel du THC
// La commande peut être bufferisée ou directe
// Note : Si le logciel doit prendre en charge l'activationdésactivation du THC, mettre
// le parammètre EE_THC_ACTIVATION_OUTPUT à 0 pour éviter les conflicts
DllExport DWORD ICNC_SetTHCStateBuf(BOOL EnableTHC, BOOL BufferizedCommand)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetTHCStateBuf %d"), GetDiffTickCount(), EnableTHC);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_SET_THC_ONOFF_BUF); //
	Push8(EnableTHC);
	Push8(BufferizedCommand);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD ICNC_ConfigPrompt(BOOL PromptState)
{
	DWORD DataSent;
	int result;
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	FlushSendData();
	Push8(ICNC_CMD_CONFIG_PROMPT); //
	Push8(PromptState); //
	return MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
}

DWORD ICNC_SetUSBSpeedMode(BOOL EnableHighSpeed)
{
	DWORD DataSent;
	int result;
	if (SimulationMode)
		return ICNCUSB_SUCCESS;
	if (!Connected)
		return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	FlushSendData();
	Push8(ICNC_USB_SPEED_SELECT); //
	if (EnableHighSpeed)  Push8(1); //
	else  Push8(0);
	return MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
}



DllExport DWORD ICNC_GetSysInfo(DWORD SysInfoType,
	DWORD* SysInfo)
{
	int i, result;
	int CntSysInfo = 0;
	DWORD* ptr = SysInfo;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d GetSYSInfo 0x%X"), GetDiffTickCount(), SysInfoType);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (SysInfoType & ICNC_SYS_INFO_BUFFER_SIZE) *ptr++ = SIMUL_BUFFER_SIZE;
		if (SysInfoType & ICNC_SYS_INFO_MAX_FREQUENCY) *ptr++ = 100000;
		if (SysInfoType & ICNC_SYS_INFO_APP_VERSION_H) *ptr++ = 4;
		if (SysInfoType & ICNC_SYS_INFO_APP_VERSION_L) *ptr++ = 1;
		if (SysInfoType & ICNC_SYS_INFO_LOADER_VERSION_H) *ptr++ = 1;
		if (SysInfoType & ICNC_SYS_INFO_LOADER_VERSION_L) *ptr++ = 0;
		if (SysInfoType & ICNC_SYS_INFO_BOARD_VERSION) *ptr++ = 200;
		if (SysInfoType & ICNC_SYS_INFO_EEPROM_SIZE) *ptr++ = 32000;
		if (SysInfoType & ICNC_SYS_INFO_USR_MEM_SIZE) *ptr++ = SIMUL_USER_MEM;
		if (SysInfoType & ICNC_SYS_INFO_AVAILABLE_AOUT) *ptr++ = 2;
		if (SysInfoType & ICNC_SYS_INFO_AVAILABLE_AIN) *ptr++ = 2;
		if (SysInfoType & ICNC_SYS_INFO_DAC_RESOLUTION) *ptr++ = SIMUL_DAC_RESOLUTION;
		if (SysInfoType & ICNC_SYS_INFO_ADC_RESOLUTION) *ptr++ = SIMUL_ADC_RESOLUTION;
		if (SysInfoType & ICNC_SYS_INFO_AVAILABLE_DOUT) *ptr++ = SIMUL_AVAILABLE_DOUT;
		if (SysInfoType & ICNC_SYS_INFO_AVAILABLE_DIN) *ptr++ = SIMUL_AVAILABLE_DIN;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	for (i = 0; i < 32; i++)
	{
		if (((SysInfoType >> i) & 0x01) != 0) CntSysInfo++;
	}

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}



	ReceiveLength = 2 + CntSysInfo * 4;
	FlushSendData();
	Push8(ICNC_CMD_GET_SYS_INFO); //
	Push32(SysInfoType);

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_SYS_INFO) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		for (i = 0; i < CntSysInfo; i++)
		{
			*ptr = Pop32();
			ptr++;
		}
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
	else {
		ReleaseMutex(ICNC2Mutex);
		return result;
	}

}




DllExport DWORD ICNC_GetSoftwareKeyValue(DWORD* SoftwareKeyValue) // Output
{
	int result;

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	*SoftwareKeyValue = 0;

	ReceiveLength = 2 + 4;
	FlushSendData();
	Push8(ICNC_CMD_GET_KEY); //

	result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
	if (result == ICNCUSB_SUCCESS)
	{
		InitReceiveData();
		if (Pop8() != ICNC_CMD_GET_KEY) return ICNCUSB_COM_ERROR;

		*SoftwareKeyValue = Pop32();
		if (DebugHandler != NULL) {
			_stprintf_s(DebugBuffer, 1024, _T("%6d GetSoftwareKeyValue = %d"), GetDiffTickCount(), *SoftwareKeyValue);
			DebugHandler(DebugBuffer, NULL);
		}
		return result;
	}
	else return result;
}

DllExport DWORD ICNC_LockBoard(void)
{
	DWORD DataSent;
	int result;


	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d XXXXXXXXXXXXXXXXXXXX   Verouillage InterpCNC"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;


	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_LOCK_BOARD); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}




DllExport DWORD ICNC_MoveProfileAbsAsync(DWORD Axis,
	DWORD FStartStop,
	DWORD Accel,
	DWORD Speed,
	DWORD Decel,
	INT   Position)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveProfileAbsAsync AXE=0x%2X SPEED=%6d Pos=%d FSTART=%d ACCEL=%d DECEL=%d"), GetDiffTickCount(), Axis, Speed, Position, FStartStop, Accel, Decel);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = Position;
		if (Axis & ICNC_AXE_Y) Simulation.PosY = Position;
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = Position;
		if (Axis & ICNC_AXE_A) Simulation.PosA = Position;
		if (Axis & ICNC_AXE_B) Simulation.PosB = Position;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_MOVE_PROFILE_ABS_ASYNC); //
	Push8(Axis); //
	Push32(FStartStop);
	Push32(Accel);
	Push32(Speed);
	Push32(Decel);
	Push32(Position);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}



DllExport DWORD ICNC_MoveProfileRelAsync(DWORD Axe,
	DWORD FStartStop,
	DWORD Accel,
	DWORD Speed,
	DWORD Decel,
	INT   Distance)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveProfileRelAsync AXE=0x%2X SPEED=%6d DIST=%d FSTART=%d ACCEL=%d DECEL=%d"), GetTickCount(), Axe, Speed, Distance, FStartStop, Accel, Decel);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (Axe & ICNC_AXE_X) Simulation.PosX += Distance;
		if (Axe & ICNC_AXE_Y) Simulation.PosY += Distance;
		if (Axe & ICNC_AXE_Z) Simulation.PosZ += Distance;
		if (Axe & ICNC_AXE_A) Simulation.PosA += Distance;
		if (Axe & ICNC_AXE_B) Simulation.PosB += Distance;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_MOVE_PROFILE_REL_ASYNC); //
	Push8(Axe); //
	Push32(FStartStop);
	Push32(Accel);
	Push32(Speed);
	Push32(Decel);
	Push32(Distance);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD ICNC_RampUpAbsBuf(DWORD Axis,
	INT Position[5],
	DWORD FStart[5],
	DWORD FEnd[5],
	DWORD Accel[5]) // Hz/s
{
	DWORD DataSent;
	int result;
	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = Position[0];
		if (Axis & ICNC_AXE_Y) Simulation.PosY = Position[1];
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = Position[2];
		if (Axis & ICNC_AXE_A) Simulation.PosA = Position[3];
		if (Axis & ICNC_AXE_B) Simulation.PosB = Position[4];
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	//        if (Speed == 0) {
	//                if (BufferRequired!=NULL) *BufferRequired = 0;
	//                return ICNCUSB_FAIL;
	//        }
			// Si pas de mouvement
	if (Axis == 0) {
		return ICNCUSB_SUCCESS;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_RAMPUP_ABS_BUF); //
	Push8(Axis);

	if (Axis & ICNC_AXE_B) { Push32(Position[4]); Push24(FStart[4]); Push24(FEnd[4]); Push24(Accel[4] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_A) { Push32(Position[3]); Push24(FStart[3]); Push24(FEnd[3]); Push24(Accel[3] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_Z) { Push32(Position[2]); Push24(FStart[2]); Push24(FEnd[2]); Push24(Accel[2] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_Y) { Push32(Position[1]); Push24(FStart[1]); Push24(FEnd[1]); Push24(Accel[1] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_X) { Push32(Position[0]); Push24(FStart[0]); Push24(FEnd[0]); Push24(Accel[0] * RAMPE_UPDATE_PERIODE / 1000); }

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;



}


DllExport DWORD ICNC_RampDownAbsBuf(DWORD Axis,
	INT Position[5],
	DWORD FStart[5],
	DWORD FEnd[5],
	DWORD Decel[5]) // HZ/s
{
	DWORD DataSent;
	int result;
	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = Position[0];
		if (Axis & ICNC_AXE_Y) Simulation.PosY = Position[1];
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = Position[2];
		if (Axis & ICNC_AXE_A) Simulation.PosA = Position[3];
		if (Axis & ICNC_AXE_B) Simulation.PosB = Position[4];
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	//        if (Speed == 0) {
	//                if (BufferRequired!=NULL) *BufferRequired = 0;
	//                return ICNCUSB_FAIL;
	//        }
			// Si pas de mouvement
	if (Axis == 0) {
		return ICNCUSB_SUCCESS;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_RAMPDOWN_ABS_BUF); //
	Push8(Axis);

	if (Axis & ICNC_AXE_B) { Push32(Position[4]); Push24(FStart[4]); Push24(FEnd[4]); Push24(Decel[4] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_A) { Push32(Position[3]); Push24(FStart[3]); Push24(FEnd[3]); Push24(Decel[3] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_Z) { Push32(Position[2]); Push24(FStart[2]); Push24(FEnd[2]); Push24(Decel[2] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_Y) { Push32(Position[1]); Push24(FStart[1]); Push24(FEnd[1]); Push24(Decel[1] * RAMPE_UPDATE_PERIODE / 1000); }
	if (Axis & ICNC_AXE_X) { Push32(Position[0]); Push24(FStart[0]); Push24(FEnd[0]); Push24(Decel[0] * RAMPE_UPDATE_PERIODE / 1000); }

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}



// ***********************************************************************************************************

DllExport DWORD ICNC_SetOutputAll(DWORD OutputState)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d SetOutputAll %8X"), GetDiffTickCount(), OutputState);
		DebugHandler(DebugBuffer, NULL);
	}


	if (SimulationMode) {
		Simulation.Output = OutputState;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;


	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;




	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_SET_OUTPUT_ALL); //
	Push32(OutputState);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD ICNC_Probe(DWORD Axis, DWORD Direction, DWORD InNo, BOOL InValue, DWORD MaxStroke, DWORD Speed, DWORD FStart, DWORD Accel, DWORD Decel)
{
	DWORD DataSent, len;
	int result;


	if (DebugHandler != NULL) {
		len = _stprintf_s(DebugBuffer, 1024, _T("%6d Probe "), GetDiffTickCount());
		if (Axis & ICNC_AXE_X) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" X"));
		if (Axis & ICNC_AXE_Y) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Y"));
		if (Axis & ICNC_AXE_Z) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Z"));
		if (Axis & ICNC_AXE_A) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" A"));
		if (Axis & ICNC_AXE_B) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" B"));
		if (Direction == 0) len += _stprintf_s(&DebugBuffer[len], 1024, _T("-"));
		else len += _stprintf_s(&DebugBuffer[len], 1024, _T("+"));
		len += _stprintf_s(&DebugBuffer[len], 1024, _T(" IN=%d"), InNo);
		if (InValue == 0) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" NC"));
		else len += _stprintf_s(&DebugBuffer[len], 1024, _T(" NO"));
		len += _stprintf_s(&DebugBuffer[len], 1024, _T(" MAX=%d SPEED=%d FSTART=%d ACCEL=%d DECEL=%d"), MaxStroke, Speed, FStart, Accel, Decel);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.LastProbe = Simulation.PosX;
		if (Axis & ICNC_AXE_Y) Simulation.LastProbe = Simulation.PosY;
		if (Axis & ICNC_AXE_Z) Simulation.LastProbe = Simulation.PosZ;
		if (Axis & ICNC_AXE_A) Simulation.LastProbe = Simulation.PosA;
		if (Axis & ICNC_AXE_B) Simulation.LastProbe = Simulation.PosB;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_PROBE); //
	Push8(Axis); //
	Push8(Direction); //
	Push8(InNo); //
	Push8(InValue); //
	Push32(MaxStroke);
	Push32(Speed);
	Push32(FStart);

	// Lors du Probe, galaad passe les accel/decel en Hz/s au lieu de KHz/s
	if (UserApplicationID != 99) {
		// Specific application ID treatement
		Push32(Accel / 100);
		Push32(Decel / 100);
	}
	else {
		Push32(Accel);
		Push32(Decel);
	}



	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}

// Function :
//      Probe sensor on specified axes and return the probe position
//
// Input :
//      Axe = Axes to probe (AXE_X or AXE_Y or AXE_Z or AXE_A or AXE_B)
//      Direction : +1 for positive direction, -1 for negative direction
//      InNo : Sensor input number
//      InValue : Input state when sensor is pressed (0 for NC, 1 fo NO)
//      MaxtStroke : Maximum number of pulse movement
//      Speed : Movement speed (Hz)
//      FStart : Movement start frequency (Hz)
//      Accel : Movement acceleration (KZ/s).
//      Decel : Movement deceleration (KHz/s).
//
// Output :
//      ProbePosition : Probe Axe position
//
// Result :
//      ICNCUSB_SUCCES if success
//      ICNC_FAIL in case of communication error
//      ICNC_PROBE_ERROR in case of error detection during probe operation (ie, MaxStroke reatched befor probe detection)


DllExport DWORD ICNC_ProbeAndReadBack(DWORD Axis, DWORD Direction, DWORD InNo, BOOL InValue, DWORD MaxStroke, DWORD Speed, DWORD FStart, DWORD Accel, DWORD Decel, INT* ProbePosition)
{
	DWORD len;
	//DWORD StatusBoard;
	INT ResultProbePosition;
	int result;

	if (DebugHandler != NULL) {
		len = _stprintf_s(DebugBuffer, _T("%6d Probe "), GetDiffTickCount());
		if (Axis & ICNC_AXE_X) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" X"));
		if (Axis & ICNC_AXE_Y) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Y"));
		if (Axis & ICNC_AXE_Z) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" Z"));
		if (Axis & ICNC_AXE_A) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" A"));
		if (Axis & ICNC_AXE_B) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" B"));
		if (Direction == 0) len += _stprintf_s(&DebugBuffer[len], 1024, _T("-"));
		else len += _stprintf_s(&DebugBuffer[len], 1024, _T("+"));
		len += _stprintf_s(&DebugBuffer[len], 1024, _T(" IN=%d"), InNo);
		if (InValue == 0) len += _stprintf_s(&DebugBuffer[len], 1024, _T(" NC"));
		else len += _stprintf_s(&DebugBuffer[len], 1024, _T(" NO"));
		len += _stprintf_s(&DebugBuffer[len], 1024, _T(" MAX=%d SPEED=%d FSTART=%d ACCEL=%d DECEL=%d"), MaxStroke, Speed, FStart, Accel, Decel);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.LastProbe = Simulation.PosX;
		if (Axis & ICNC_AXE_Y) Simulation.LastProbe = Simulation.PosY;
		if (Axis & ICNC_AXE_Z) Simulation.LastProbe = Simulation.PosZ;
		if (Axis & ICNC_AXE_A) Simulation.LastProbe = Simulation.PosA;
		if (Axis & ICNC_AXE_B) Simulation.LastProbe = Simulation.PosB;
		if (ProbePosition != NULL) *ProbePosition = Simulation.LastProbe;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if (ProbePosition == NULL) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	// Move until sensor detection
	result = ICNC_Probe(Axis, Direction, InNo, InValue, MaxStroke, Speed, FStart, Accel, Decel);
	if (result != ICNCUSB_SUCCESS) return result;
	// Check for command completion

	// Wait for write completion
	//DWORD tick = GetTickCount()+TimeOut;
	DWORD PrevTickCount = GetTickCount() + 1;
	DWORD Status = ICNC_STATUS_BIT_PROBING;
	Sleep(2);


	do {
		result = ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Status);
		if (result != ICNCUSB_SUCCESS) return result;
		if ((Status & ICNC_STATUS_BIT_PROBING) != 0)
			Sleep(5);
	} while ((Status & ICNC_STATUS_BIT_PROBING) != 0);

	// Check for probe command success
	if ((/*StatusBoard*/Status & ICNC_STATUS_BIT_PROBING_ERROR) != 0)
		return ICNCUSB_PROBE_ERROR;

	// Read Probe position result
	result = ICNC_GetBoardStatus(ICNC_STATUS_LAST_PROBE, (DWORD*)&ResultProbePosition);
	if (result != ICNCUSB_SUCCESS) return result;
	*ProbePosition = ResultProbePosition;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d        Probe result = %d"), GetDiffTickCount(), *ProbePosition);
		DebugHandler(DebugBuffer, NULL);
	}

	return ICNCUSB_SUCCESS;
}

DllExport DWORD ICNC_SlowStopMotors(DWORD Axes)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SlowStopMotors %X"), GetDiffTickCount(), Axes);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_BREAKE_AXES); //
	Push8(Axes); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}
DllExport DWORD ICNC_SlowStopAllAndClear(void)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SlowStopAllAndClear"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_BREAKE_AXES_AND_CLEAR); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);


	return result;
}

DllExport DWORD ICNC_StopMotorsAllAndClear(void)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d StopMotorsAndClear"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_STOP); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


// Commandes buferisées
// ******************************************************************

DllExport DWORD ICNC_FreezeBuf(void)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d FreezBuffer"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_FREEZE_BUF); //
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}

DllExport DWORD ICNC_SetOverrideStateBuf(BOOL OverrideState)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetOverrideStateBuf %d"), GetDiffTickCount(), OverrideState);
		DebugHandler(DebugBuffer, NULL);
	}


	if (SimulationMode) {

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_SET_OVERRIDE_STATE_BUF); //
	Push8(OverrideState);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}



DllExport DWORD ICNC_SetOutputBuf(DWORD OutputNo,
	BOOL OutputValue)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetOutputBuf N°%d to %d"), GetDiffTickCount(), OutputNo, OutputValue);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		if (OutputValue == FALSE) Simulation.Output &= ~(1 << (OutputNo - 1));
		else Simulation.Output |= (1 << (OutputNo - 1));
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_SET_OUTPUT_BUF); //
	Push8(OutputNo);
	Push8(OutputValue);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}
DllExport DWORD ICNC_SetOutputAllBuf(DWORD OutputValue)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetOutputAllBuf %X"), GetDiffTickCount(), OutputValue);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		Simulation.Output = OutputValue;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;



	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_SET_OUTPUT_ALL_BUF); //
	Push32(OutputValue);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}
DllExport DWORD ICNC_SetAnalogBuf(DWORD OutputNo,
	DWORD OutputValue)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d SetAnalogBuf N°%d to %d"), GetDiffTickCount(), OutputNo, OutputValue);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_SET_ANALOG_BUF); //
	Push8(OutputNo);
	//Push16((int)((double)OutputValue * OutputValue*OutputValue*OutputValue/1024.0/1024.0/1024.0)); //
	//Push16((int)((double)OutputValue * OutputValue*OutputValue/1024.0/1024.0)); //
	Push16(OutputValue);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


/// <summary>
/// Push delay in buffer execution
/// </summary>
/// <param name="TimeToWait">unit is per 100ms</param>
/// <returns></returns>
DllExport DWORD ICNC_WaitBuf(DWORD TimeToWait)
{
	DWORD DataSent;
	int result;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d Bufferized timer %dms"), GetDiffTickCount(), TimeToWait * 100);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	FlushSendData();
	Push8(ICNC_CMD_TEMPO_BUF); //
	Push32(TimeToWait);
	return MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
}

DllExport DWORD ICNC_WaitInputStateBuf(DWORD InputNo,
	BOOL InputValue,
	DWORD TimeOut,
	BOOL LockIfError,
	BOOL ClearBufferIfError)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d Wait for specific input state N°%d N° STATE=%d Timeout=%d"), GetDiffTickCount(), InputNo, InputValue, TimeOut);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_WAIT_INPUT_BUF); //
	Push8(InputNo);
	Push8(InputValue);
	Push16(TimeOut);
	Push8(LockIfError);
	Push8(ClearBufferIfError);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;

}

DllExport DWORD ICNC_WriteUserMemBuf(DWORD MemNumber,
	DWORD Value)
{
	DWORD DataSent;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d WriteUserMemBuf N°%d=%d"), GetDiffTickCount(), MemNumber, Value);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_WRITE_USER_MEM_BUF); //
	Push8(MemNumber);
	Push32(Value);
	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);

	return result;


}

// Relative movement without speed profile
DllExport DWORD ICNC_MoveSpeedRelBuf(DWORD Speed,
	INT MoveX,
	INT MoveY,
	INT MoveZ,
	INT MoveA,
	INT MoveB,
	DWORD* BufferRequired)
{
	DWORD DataSent;
	int result;
	int axis = 0, nbAxis = 0;



	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveSpeedRelBuf SPEED=%6d X=%d Y=%d Z=%d A=%d B=%d"), GetDiffTickCount(), Speed, MoveX, MoveY, MoveZ, MoveA, MoveB);
		DebugHandler(DebugBuffer, NULL);
	}



	if (SimulationMode) {
		Simulation.PosX += MoveX;
		Simulation.PosY += MoveY;
		Simulation.PosZ += MoveZ;
		Simulation.PosA += MoveA;
		Simulation.PosB += MoveB;
		if (BufferRequired != NULL) *BufferRequired = 10;
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;

	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return result;
	}

	if (MoveX != 0) { axis |= ICNC_AXE_X; nbAxis++; }
	if (MoveY != 0) { axis |= ICNC_AXE_Y; nbAxis++; }
	if (MoveZ != 0) { axis |= ICNC_AXE_Z; nbAxis++; }
	if (MoveA != 0) { axis |= ICNC_AXE_A; nbAxis++; }
	if (MoveB != 0) { axis |= ICNC_AXE_B; nbAxis++; }


	// Si pas de mouvement
	if (axis == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}

	if (BufferRequired != NULL) *BufferRequired = 2 + 2 * nbAxis;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_MOVE_CONSTANT_REL_BUF); //
	Push8(axis);
	if (MoveB != 0) Push32(MoveB);
	if (MoveA != 0) Push32(MoveA);
	if (MoveZ != 0) Push32(MoveZ);
	if (MoveY != 0) Push32(MoveY);
	if (MoveX != 0) Push32(MoveX);
	Push32(Speed);


	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);

	ReleaseMutex(ICNC2Mutex);
	return result;
}



DllExport DWORD ICNC_MoveSpeedRelLocalBuf(DWORD Speed,
	INT MoveX,
	INT MoveY,
	INT MoveZ,
	INT MoveA,
	INT MoveB,
	DWORD* BufferRequired)
{
	DWORD result;
	DWORD lBufferRequired;
	int axis = 0, nbAxis = 0;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveSpeedRelLocalBuf SPEED=%6d X=%d Y=%d Z=%d A=%d B=%d"), GetDiffTickCount(), Speed, MoveX, MoveY, MoveZ, MoveA, MoveB);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		Simulation.PosX += MoveX;
		Simulation.PosY += MoveY;
		Simulation.PosZ += MoveZ;
		Simulation.PosA += MoveA;
		Simulation.PosB += MoveB;
		if (BufferRequired != NULL) *BufferRequired = 10;

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;

	if (MoveX != 0) { axis |= ICNC_AXE_X; nbAxis++; }
	if (MoveY != 0) { axis |= ICNC_AXE_Y; nbAxis++; }
	if (MoveZ != 0) { axis |= ICNC_AXE_Z; nbAxis++; }
	if (MoveA != 0) { axis |= ICNC_AXE_A; nbAxis++; }
	if (MoveB != 0) { axis |= ICNC_AXE_B; nbAxis++; }

	// Si pas de mouvement
	if (axis == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}

	lBufferRequired = 2 + 2 * nbAxis;
	if (BufferRequired != NULL) *BufferRequired = lBufferRequired;

	if (SendDataSize + lBufferRequired >= SEUIL_LOCAL_BUFF)
	{
		result = ICNC_FlushLocalBuf();
		if (result != ICNCUSB_SUCCESS) return result;
		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
		{
			return ICNCUSB_FAIL;
		}
		FlushSendData();
	}
	else {
		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
		{
			return ICNCUSB_FAIL;
		}
	}



	Push8(ICNC_CMD_MOVE_CONSTANT_REL_BUF); //
	Push8(axis);
	if (MoveB != 0) Push32(MoveB);
	if (MoveA != 0) Push32(MoveA);
	if (MoveZ != 0) Push32(MoveZ);
	if (MoveY != 0) Push32(MoveY);
	if (MoveX != 0) Push32(MoveX);
	Push32(Speed);
	ReleaseMutex(ICNC2Mutex);
	return ICNCUSB_SUCCESS;

}



DllExport DWORD ICNC_FlushLocalBuf(void)
{
	DWORD DataSent;
	if (SendDataSize > 0) {
		if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 3000))
			return ICNCUSB_FAIL;

		int result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		ReleaseMutex(ICNC2Mutex);
		return result;
	}
	else
		return ICNCUSB_SUCCESS;
}



DllExport DWORD ICNC_MoveProfileRelBuf(DWORD Speed,
	INT MoveX,
	INT MoveY,
	INT MoveZ,
	INT MoveA,
	INT MoveB,
	DWORD* BufferRequired)
{
	DWORD DataSent;
	int result;
	int axis = 0, nbAxis = 0;

	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveProfileRelBuf SPEED=%6d X=%d Y=%d Z=%d A=%d B=%d"), GetDiffTickCount(), Speed, MoveX, MoveY, MoveZ, MoveA, MoveB);
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		Simulation.PosX += MoveX;
		Simulation.PosY += MoveY;
		Simulation.PosZ += MoveZ;
		Simulation.PosA += MoveA;
		Simulation.PosB += MoveB;
		if (BufferRequired != NULL) *BufferRequired = 10;

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;

	if (MoveX != 0) { axis |= ICNC_AXE_X; nbAxis++; }
	if (MoveY != 0) { axis |= ICNC_AXE_Y; nbAxis++; }
	if (MoveZ != 0) { axis |= ICNC_AXE_Z; nbAxis++; }
	if (MoveA != 0) { axis |= ICNC_AXE_A; nbAxis++; }
	if (MoveB != 0) { axis |= ICNC_AXE_B; nbAxis++; }


	// Si pas de mouvement
	if (axis == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}

	if (BufferRequired != NULL) *BufferRequired = 2 + axis + 1;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_CMD_PROFILE_REL_BUF); //
	Push8(axis);
	if (MoveB != 0) Push32(MoveB);
	if (MoveA != 0) Push32(MoveA);
	if (MoveZ != 0) Push32(MoveZ);
	if (MoveY != 0) Push32(MoveY);
	if (MoveX != 0) Push32(MoveX);
	Push32(Speed);

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}

DllExport DWORD ICNC_MoveProfileAbsBuf(DWORD Axis,
	DWORD Speed,
	INT PositionX,
	INT PositionY,
	INT PositionZ,
	INT PositionA,
	INT PositionB,
	DWORD* BufferRequired)
{
	DWORD DataSent;
	DWORD lBufferRequired = 2 + 1;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveProfileAbsBuf AXES=0x%2X SPEED=%6d X=%7d Y=%7d Z=%7d A=%7d B=%7d"), GetDiffTickCount(), Axis, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB);
		DebugHandler(DebugBuffer, NULL);
	}


	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = PositionX;
		if (Axis & ICNC_AXE_Y) Simulation.PosY = PositionY;
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = PositionZ;
		if (Axis & ICNC_AXE_A) Simulation.PosA = PositionA;
		if (Axis & ICNC_AXE_B) Simulation.PosB = PositionB;
		if (BufferRequired != NULL) *BufferRequired = 10;

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (Speed == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_FAIL;
	}
	// Si pas de mouvement
	if (Axis == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}



	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}


	FlushSendData();
	Push8(ICNC_CMD_PROFILE_ABS_BUF); //
	Push8(Axis);

	if (Axis & ICNC_AXE_B) { Push32(PositionB); lBufferRequired++; }
	if (Axis & ICNC_AXE_A) { Push32(PositionA); lBufferRequired++; }
	if (Axis & ICNC_AXE_Z) { Push32(PositionZ); lBufferRequired++; }
	if (Axis & ICNC_AXE_Y) { Push32(PositionY); lBufferRequired++; }
	if (Axis & ICNC_AXE_X) { Push32(PositionX); lBufferRequired++; }

	Push32(Speed);

	if (BufferRequired != NULL) *BufferRequired = lBufferRequired;

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;



}



DllExport DWORD ICNC_MoveSpeedAbsBuf(DWORD Axis,
	DWORD Speed,
	INT PositionX,
	INT PositionY,
	INT PositionZ,
	INT PositionA,
	INT PositionB,
	DWORD* BufferRequired)
{
	DWORD DataSent;
	DWORD lBufferRequired;
	int result;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, 1024, _T("%6d MoveSpeedAbsBuf AXES=0x%2X SPEED=%6d X=%7d Y=%7d Z=%7d A=%7d B=%7d"), GetDiffTickCount(), Axis, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB);
		DebugHandler(DebugBuffer, NULL);
	}

	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = PositionX;
		if (Axis & ICNC_AXE_Y) Simulation.PosY = PositionY;
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = PositionZ;
		if (Axis & ICNC_AXE_A) Simulation.PosA = PositionA;
		if (Axis & ICNC_AXE_B) Simulation.PosB = PositionB;
		if (BufferRequired != NULL) *BufferRequired = 10;

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (Speed == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_FAIL;
	}

	// Si pas de mouvement
	if (Axis == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}


	lBufferRequired = 3;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	FlushSendData();
	Push8(ICNC_CMD_MOVE_CONSTANT_ABS_BUF); //
	Push8(Axis);

	if (Axis & ICNC_AXE_B) { Push32(PositionB); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_A) { Push32(PositionA); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_Z) { Push32(PositionZ); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_Y) { Push32(PositionY); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_X) { Push32(PositionX); lBufferRequired += 1; }

	Push32(Speed);

	if (BufferRequired != NULL) *BufferRequired = lBufferRequired;

	result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);

	return result;
}




DllExport DWORD ICNC_MoveSpeedAbsLocalBuf(DWORD Axis,
	DWORD Speed,
	INT PositionX,
	INT PositionY,
	INT PositionZ,
	INT PositionA,
	INT PositionB,
	DWORD* BufferRequired)
{
	DWORD lBufferRequired;
	int result;
	if (DebugHandler != NULL) {

		_stprintf_s(DebugBuffer, _T("%6d MoveSpeedAbsLocalBuf AXES=0x%2X SPEED=%6d X=%7d Y=%7d Z=%7d A=%7d B=%7d"), GetDiffTickCount(), Axis, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB);
		DebugHandler(DebugBuffer, NULL);
		//}
	}

	if (SimulationMode) {
		if (Axis & ICNC_AXE_X) Simulation.PosX = PositionX;
		if (Axis & ICNC_AXE_Y) Simulation.PosY = PositionY;
		if (Axis & ICNC_AXE_Z) Simulation.PosZ = PositionZ;
		if (Axis & ICNC_AXE_A) Simulation.PosA = PositionA;
		if (Axis & ICNC_AXE_B) Simulation.PosB = PositionB;
		if (BufferRequired != NULL) *BufferRequired = 10;

		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;

	if (Speed == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_FAIL;
	}
	// Si pas de mouvement
	if ((Axis & 0x1F) == 0) {
		if (BufferRequired != NULL) *BufferRequired = 0;
		return ICNCUSB_SUCCESS;
	}


	lBufferRequired = 3;	// Pour fctn, axis et speed

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	Push8(ICNC_CMD_MOVE_CONSTANT_ABS_BUF); //
	Push8(Axis);

	if (Axis & ICNC_AXE_B) { Push32(PositionB); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_A) { Push32(PositionA); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_Z) { Push32(PositionZ); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_Y) { Push32(PositionY); lBufferRequired += 1; }
	if (Axis & ICNC_AXE_X) { Push32(PositionX); lBufferRequired += 1; }

	//CheckSpeedLimitationAbs(Axis, &Speed, PositionX+Offset.X, PositionY+Offset.Y, PositionZ+Offset.Z, PositionA+Offset.A, PositionB+Offset.B );

	Push32(Speed);

	if (SendDataSize + lBufferRequired >= SEUIL_LOCAL_BUFF)
	{
		result = ICNC_FlushLocalBuf();
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		FlushSendData();
	}


	if (BufferRequired != NULL) *BufferRequired = lBufferRequired;
	ReleaseMutex(ICNC2Mutex);
	return ICNCUSB_SUCCESS;
}


DllExport DWORD ICNC_SwitchToBootLoaderMode(DWORD BootKey)
{
	DWORD DataSent;
	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d Switch to boot loader mode"), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;


	FlushSendData();
	Push8(ICNC_CMD_BOOT_MODE); //
	Push32(BootKey);

	Connected = FALSE;

	return MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);

}





// Le nombre d'octets envoyés doit être un multiple de 4.
// Si nbBytesdoit n'est pas un multiple de 4, remplir avec des 0
DllExport DWORD ICNC_PushPulsesData(unsigned char* ptBuffer, int nbBytes, DWORD Frequency, DWORD* BufferRequired)
{
	DWORD DataSent;
	int result;
	int count;


	if (DebugHandler != NULL) {
		_stprintf_s(DebugBuffer, _T("%6d Send Pulses data "), GetDiffTickCount());
		DebugHandler(DebugBuffer, NULL);
	}
	if (SimulationMode) {
		return ICNCUSB_SUCCESS;
	}
	if (!Connected) return ICNCUSB_FAIL;

	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}

	count = 0;
	FlushSendData();


	int NbSubFrame = (nbBytes + 58) / 59;
	int NoSubFrame = 0;
	while (count < nbBytes) {


		if (count % 59 == 0) {
			NoSubFrame = (count) / 59;
			FlushSendDataSubFrame(NoSubFrame);
			Push8SubFrame(ICNC_PULSE_FRAME, NoSubFrame); //
			Push8SubFrame(MIN(nbBytes - count, 59), NoSubFrame); //
			Push16SubFrame((int)(80E6 / Frequency), NoSubFrame); //  Periode timer generateur de pulse

		}
		Push8SubFrame(ptBuffer[count], NoSubFrame);
		count++;
	}


	if (BufferRequired != NULL) {
		*BufferRequired = NbSubFrame * 18;
	}

	result = MPUSBWrite(SendData, NbSubFrame * 5 + nbBytes, &DataSent, 1000);
	ReleaseMutex(ICNC2Mutex);
	return result;
}


DllExport DWORD ICNC_CustomCommand(int SubCommandID,
	int NumRegistre,
	int NbRegister,
	unsigned char* ptBuffer)
{
	int i, j, result;
	DWORD DataSent;
	//PrevCommande =  ICNC_CMD_GET_ANALOG;
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_CMD_CUSTOM);  //
	Push8(SubCommandID);        //

	switch (SubCommandID) {
	case ICNC_CMD_CUSTOM_SETSYS:
		Push16(NumRegistre);
		Push32(*(unsigned int*)ptBuffer);
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;

	case ICNC_CMD_CUSTOM_GETSYS:
		Push16(NumRegistre);
		ReceiveLength = 2 + 4;
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_CUSTOM) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		// On récupère un int32
		ptBuffer[0] = Pop8();
		ptBuffer[1] = Pop8();
		ptBuffer[2] = Pop8();
		ptBuffer[3] = Pop8();

		ReleaseMutex(ICNC2Mutex);
		break;
	case ICNC_CMD_CUSTOM_GETSYS_MULTI:
		Push16(NumRegistre);
		Push16(NbRegister);
		ReceiveLength = 2 + 4 * NbRegister;
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_CUSTOM) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		j = 0;
		for (i = 0; i < NbRegister; i++) {
			//i1 = Pop32();
			ptBuffer[0] = Pop8();
			ptBuffer[1] = Pop8();
			ptBuffer[2] = Pop8();
			ptBuffer[3] = Pop8();
			ptBuffer += 4;
		}
		ReleaseMutex(ICNC2Mutex);
		break;
	}

	return result;
}



DllExport DWORD ICNC_MBControl(int SubCommandID,
	unsigned char* ptBuffer)
{
	int i, j, i1, i2, i3, result;
	unsigned char c1;
	DWORD DataSent;
	//PrevCommande =  ICNC_CMD_GET_ANALOG;
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_CMD_MB_CONTROL);  //
	Push8(SubCommandID);        //

	switch (SubCommandID) {
	case ICNC_CMD_MB_BITS_SET:
		for (i = 0; i < 12; i++)    // 12 octets pour les 96 bits
			c1 = ptBuffer[i + 1];
		c1 = (unsigned char)((c1 * 0x0202020202ULL & 0x010884422010ULL) % 1023);
		Push8(c1);
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;

	case ICNC_CMD_MB_BITS_GET:
		ReceiveLength = 14; // Taille+CMD+3*4
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_MB_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		j = 0;
		for (i = 0; i < 4; i++) {
			i1 = Pop32();
			ptBuffer[j++] = (unsigned char)((i1 >> 24) & 0xFF);
			ptBuffer[j++] = (unsigned char)((i1 >> 16) & 0xFF);
			ptBuffer[j++] = (unsigned char)((i1 >> 8) & 0xFF);
			ptBuffer[j++] = (unsigned char)(i1 & 0xFF);
		}
		ReleaseMutex(ICNC2Mutex);
		break;
	case ICNC_CMD_MB_ONE_BIT_SET:
		// le buffer contient :
		// Le numéro du bit à écrire sur 16 bits,
		// La valeur du bit (0 ou autre) sur 8 bits
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);
		Push16(i1);
		Push8(ptBuffer[2]);
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;
	case ICNC_CMD_MB_GET_HOLDING_REG:
		// le buffer contient :
		// Le numéro du registre à lire sur 16 bits,
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);
		Push16(i1);

		ReceiveLength = 4; // Taille+CMD+4*4
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_MB_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		i1 = Pop16();
		ptBuffer[0] = (unsigned char)((i1 >> 8) & 0xFF);
		ptBuffer[1] = (unsigned char)((i1) & 0xFF);

		ReleaseMutex(ICNC2Mutex);

		break;
	case ICNC_CMD_MB_SET_HOLDING_REG:
		// le buffer contient :
		// Le numéro du registre à écrire sur 16 bits,
		// La valeur du registre sur 16 bits
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);
		i2 = ((ptBuffer[2] << 8) & 0xFF00) | (ptBuffer[3] & 0xFF);
		Push16(i1);
		Push16(i2);
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;


	case ICNC_CMD_MB_GET_HOLDING_REGS:	// read one or several holding registers
	case ICNC_CMD_MB_GET_INPUT_REGS:	// read one or several holding registers

		// le buffer contient :
		// Le numéro du registre à lire sur 16 bits,
		// le nombre de registre à lire sur 16 bits
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);	// Adresse modbus
		Push16(i1);

		i1 = ((ptBuffer[2] << 8) & 0xFF00) | (ptBuffer[3] & 0xFF);	// Nombre de registres
		i1 = min(i1, 31);	// 31 registers maxi
		Push16(i1);

		ReceiveLength = 2 + i1 * 2; // Taille+CMD+2*nbRegister
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_MB_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		for (i = 0; i < i1; i++) {
			i2 = Pop16();
			ptBuffer[0 + i * 2] = (unsigned char)((i2 >> 8) & 0xFF);
			ptBuffer[1 + i * 2] = (unsigned char)((i2) & 0xFF);

		}
		ReleaseMutex(ICNC2Mutex);
		break;


	case ICNC_CMD_MB_SET_HOLDING_REGS:
		// le buffer contient :
		// Le numéro du registre à écrire sur 16 bits,
		// Le nombre de registre (16 bits)
		// Les valeurs à écrire
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);
		i2 = ((ptBuffer[2] << 8) & 0xFF00) | (ptBuffer[3] & 0xFF);
		Push16(i1);
		Push16(i2);
		for (int i = 0; i < i2; i++) {
			int value = ((ptBuffer[(i*2)+4] << 8) & 0xFF00) | (ptBuffer[(i*2)+5] & 0xFF);
			Push16(value);
		}
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;


	case ICNC_CMD_MB_GET_COILS:
	case ICNC_CMD_MB_GET_INPUTS:
		// le buffer contient :
		// Le numéro du registre à lire sur 16 bits,
		// le nombre de registre à lire sur 16 bits
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);	// Adresse modbus
		Push16(i1);

		i1 = ((ptBuffer[2] << 8) & 0xFF00) | (ptBuffer[3] & 0xFF);	// Nombre de input bits ou coils
		i1 = min(i1, 496);	// maximum number limitation
		Push16(i1);

		i2 = (i1+7) / 8;

		ReceiveLength = 2 +  i2; // Taille+CMD+ bytes used for bit receiving
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_CMD_MB_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		for (i = 0; i < i2; i++) {
			ptBuffer[i] = (unsigned char)(Pop8());

		}
		ReleaseMutex(ICNC2Mutex);
		break;

	case ICNC_CMD_MB_SET_COILS:
		// le buffer contient :
		// Le numéro du registre à écrire sur 16 bits,
		// Le nombre de cois (16 bits)
		// Les valeurs à écrire
		i1 = ((ptBuffer[0] << 8) & 0xFF00) | (ptBuffer[1] & 0xFF);
		i2 = ((ptBuffer[2] << 8) & 0xFF00) | (ptBuffer[3] & 0xFF);
		Push16(i1);
		Push16(i2);
		i3 = (i2 + 7) / 8;
		for (int i = 0; i < i3; i++) {
			int value = ptBuffer[i + 4];
			Push8(value);
		}
		try {

			result = MPUSBWrite(SendData, SendDataSize + 1, &DataSent, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		ReleaseMutex(ICNC2Mutex);
		return result;
		break;


	default:
		throw "Unknwn Modbus USB sub function code";


	}
	return result;
}


DllExport DWORD ICNC_BasicControl(int SubCommandID,
	char* ptBuffer,
	int SizeToSend,
	int* BufferFree)
{
	int i, i1, result;
	//PrevCommande =  ICNC_CMD_GET_ANALOG;
	if (SimulationMode) return ICNCUSB_SUCCESS;
	if (!Connected) return ICNCUSB_FAIL;
	if ((result = ICNC_FlushLocalBuf()) != ICNCUSB_SUCCESS)
		return result;
	if (WAIT_OBJECT_0 != WaitForSingleObject(ICNC2Mutex, 100))
	{
		return ICNCUSB_FAIL;
	}
	FlushSendData();
	Push8(ICNC_BASIC_CONTROL);  //
	Push8(SubCommandID);        //

	switch (SubCommandID) {
	case ICNC_BASIC_CONTROL_GETTX:
		ReceiveLength = 64;
		try {

			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}

		ptBuffer[0] = '\0';    // Fin de chaine
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_BASIC_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		i1 = Pop8(); // Nb d'octets à traiter (longueur de chaine)
		ptBuffer[0] = i1;
		for (i = 0; i < i1; i++) {
			ptBuffer[i] = Pop8();
		}
		ptBuffer[i1] = '\0';


		ReleaseMutex(ICNC2Mutex);
		break;
	case ICNC_BASIC_CONTROL_SETRX:
		i1 = SizeToSend;
		Push8(i1);
		for (i = 0; i < i1; i++)
			Push8(ptBuffer[i]);
		ReceiveLength = 2 + 2;
		try {
			result = SendReceivePacket(SendData, SendDataSize + 1, ReceiveData, &ReceiveLength, 1000, 1000);
		}
		catch (...) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;

		}
		if (result != ICNCUSB_SUCCESS) {
			ReleaseMutex(ICNC2Mutex);
			return result;
		}
		InitReceiveData();
		if (Pop8() != ICNC_BASIC_CONTROL) {
			ReleaseMutex(ICNC2Mutex);
			return ICNCUSB_COM_ERROR;
		}
		*BufferFree = Pop16();
		ReleaseMutex(ICNC2Mutex);
		break;

	}
	return result;
}



DllExport DWORD ICNC_SendPLCCommand(char* Command) {
#define MAX_SIZE 60	// Nombre maxi de caractère envoyés dans une seule trame
	char DataBuffer[64];
	int BufferFree = 0;
	int size = strlen(Command);
	int fromindex = 0;

	// Read actual PLC free buffer space
	DataBuffer[0] = '\0';	// Chaine vide
	if (ICNC_BasicControl(ICNC_BASIC_CONTROL_SETRX, DataBuffer, 0, &BufferFree) != ICNCUSB_SUCCESS) {
		return ICNCUSB_FAIL;
	}

	while (size > 0) {
		// Check for maximum datasize to send in this frame
		int tosend = size;
		if (tosend > MAX_SIZE) tosend = MAX_SIZE;
		if (tosend > BufferFree) tosend = BufferFree;
		memcpy(DataBuffer, (void*)&Command[fromindex], tosend);
		if (ICNC_BasicControl(ICNC_BASIC_CONTROL_SETRX, DataBuffer, tosend, &BufferFree) != ICNCUSB_SUCCESS) {
			return ICNCUSB_FAIL;
		}
		size -= tosend;
		fromindex += tosend;
	}
	return ICNCUSB_SUCCESS;
}




DllExport DWORD ICNC_ReadPLCMonitor(char* MonitorTxt) {
	int res;
	res = ICNC_BasicControl(ICNC_BASIC_CONTROL_GETTX,
		MonitorTxt,
		0,
		NULL);
	return res;
}
}