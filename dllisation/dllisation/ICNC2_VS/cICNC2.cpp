#include "stdafx.h"
#include "cICNC2.h"
#include "ICNC2.h"
#include "LinkedList.h"
#include <thread>
#include <iostream>


typedef struct {
	DWORD ID;
	DWORD FlagBit;
	int NbRegisters;
	HANDLE hTimer;
	PDWORD Result;  // For return result 
	cICNC2* hICNC2;
	PeriodicReadCallback_t callback;
	PVOID arg;
} PeriodicReadCallback_param_t;


unsigned int countSetBits(DWORD n)
{
	unsigned int count = 0;
	while (n) {
		count += n & 1;
		n >>= 1;
	}
	return count;
}

DllExport cICNC2* create_ICNC2(void) {
	return new cICNC2();
}


//
//std::thread *th=NULL;
//bool KillAutoconnectThread = false;
//
//int cICNC2::AutoconnectThreadEntry(PVOID arg) {
//	cICNC2* ICNC2 =  reinterpret_cast<cICNC2*>(arg);
//	DWORD trash;
//	while (!KillAutoconnectThread) {
//		ICNC2->GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &trash);
//		if (!ICNC2->is_connected())
//			ICNC2->Connect();
//		Sleep(200);
//	}
//	return 0;
//}




cICNC2::cICNC2(bool AutoConnect) {
	m_connected = false;
	m_SysInfo = { 0 };
	m_AutoConnect = AutoConnect;
	if (m_AutoConnect)
		is_connected();	// Try to auto connect
	//if (AutoConnect) {
	//	th = new std::thread(&cICNC2::AutoconnectThreadEntry, this);
	//}
}

cICNC2::~cICNC2() {
	m_connected = false;

	//if (m_AutoConnect) {
	//	KillAutoconnectThread = true;
	//	th->join();
	//}
	DisConnect();
	if (m_hTimerQueue)
		DeleteTimerQueue(m_hTimerQueue);


	// free registered read status callback and allocated memory space
	if (m_RegisteredCallbackList != NULL) {
		List* list = static_cast<List*>(m_RegisteredCallbackList);
		Node* current = list->head;
		try {
			while (current != NULL) {
				PeriodicReadCallback_param_t* Callback_prm = static_cast<PeriodicReadCallback_param_t*>((PeriodicReadCallback_param_t*)current->data);
				delete Callback_prm->Result;	// free array's of results
				deleteNode((int)Callback_prm, list, true);
				current = list->head;
			}
		}
		catch (...) {
			
		}
	}
	destroyList((List*)m_RegisteredCallbackList, true);
}


int cICNC2::Connect(void) {

	bool InitialState = m_connected;
	m_connected = (ICNC_Connect(0) == ICNCUSB_SUCCESS);
	if (m_connected) {
		DWORD SysInfoRequest =
			ICNC_SYS_INFO_BUFFER_SIZE      \
			| ICNC_SYS_INFO_MAX_FREQUENCY    \
			| ICNC_SYS_INFO_APP_VERSION_H    \
			| ICNC_SYS_INFO_APP_VERSION_L    \
			| ICNC_SYS_INFO_LOADER_VERSION_H \
			| ICNC_SYS_INFO_LOADER_VERSION_L \
			| ICNC_SYS_INFO_BOARD_VERSION    \
			| ICNC_SYS_INFO_EEPROM_SIZE      \
			| ICNC_SYS_INFO_USR_MEM_SIZE     \
			| ICNC_SYS_INFO_AVAILABLE_AOUT   \
			| ICNC_SYS_INFO_AVAILABLE_AIN    \
			| ICNC_SYS_INFO_DAC_RESOLUTION   \
			| ICNC_SYS_INFO_ADC_RESOLUTION   \
			| ICNC_SYS_INFO_AVAILABLE_DOUT   \
			| ICNC_SYS_INFO_AVAILABLE_DIN;
		DWORD SysInfoResult[15];

		if (GetSysInfo(SysInfoRequest, SysInfoResult) == ICNCUSB_SUCCESS) {
			m_SysInfo.BufferSize = SysInfoResult[0];
			m_SysInfo.MaxPulseFrequency = SysInfoResult[1];
			m_SysInfo.FirmwareVersionH = SysInfoResult[2];
			m_SysInfo.FirmwareVersionL = SysInfoResult[3];
			m_SysInfo.BootloaderVersionH = SysInfoResult[4];
			m_SysInfo.BootloaderVersionL = SysInfoResult[5];
			m_SysInfo.BoardHWVersion = SysInfoResult[6];
			m_SysInfo.EEPROMSize = SysInfoResult[7];
			m_SysInfo.UserMemAvailable = SysInfoResult[8];
			m_SysInfo.AOUTAvailable = SysInfoResult[9];
			m_SysInfo.AINAvailable = SysInfoResult[10];
			m_SysInfo.DACResolution = SysInfoResult[11];
			m_SysInfo.ADCResolution = SysInfoResult[12];
			m_SysInfo.DOUTAvailable = SysInfoResult[13];
			m_SysInfo.DINAvailable = SysInfoResult[14];
		}
		//else
		//	Disconnected();
	}
	// Call Oncennected Event if state changed
	if (InitialState != m_connected)
		OnConnectChanged(m_connected, m_OnConnectChangedEventArg);
	return m_connected ? 1 : 0;
}



void cICNC2::DisConnect(void) {
	if (!is_connected())
		return;
	//ICNC_Disconnect();
	Disconnected();
}




int   cICNC2::ErrorReset(DWORD ErrorStatusBit)
{
	if (!is_connected())
		return 0;
	if (ICNC_ErrorReset(ErrorStatusBit) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;

}


int   cICNC2::Lock(void)
{
	if (!is_connected())
		return 0;
	if (ICNC_LockBoard() != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;

}



int cICNC2::GetSysInfo(DWORD SysInfoFlag, DWORD* SysInfoValue) {
	if (!is_connected())
		return 0;
	if (SysInfoValue == NULL)
		return -1;
	if (ICNC_GetSysInfo(SysInfoFlag, SysInfoValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;

}


int cICNC2::GetBoardStatus(DWORD StatusType, DWORD* Status) {
	if (!is_connected())
		return 0;
	if (ICNC_GetBoardStatus(StatusType, Status) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

/// AXES stuff
/// ***********************************************************************


int cICNC2::SetPosition(DWORD Axes, INT PositionX, INT PositionY, INT PositionZ, INT PositionA, INT PositionB) {
	if (!is_connected())
		return 0;
	if (ICNC_WritePosition(Axes, PositionX, PositionY, PositionZ, PositionA, PositionB) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::MoveProfileAbsAsync(DWORD Axe, DWORD FStartStop, DWORD Accel, DWORD Speed, DWORD Decel, INT Position)
{
	if (!is_connected())
		return 0;
	if (ICNC_MoveProfileAbsAsync(Axe, FStartStop, Accel, Speed, Decel, Position) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}
int cICNC2::MachineHome(DWORD Axes, DWORD MaxStrokeX, DWORD MaxStrokeY, DWORD MaxStrokeZ, DWORD MaxStrokeA, DWORD MaxStrokeB)
{
	if (!is_connected())
		return 0;
	if (ICNC_MachineHome(Axes, MaxStrokeX, MaxStrokeY, MaxStrokeZ, MaxStrokeA, MaxStrokeB) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int cICNC2::AxeHome(DWORD AxeID,
	DWORD InputNumber,
	DWORD ExpectedInputValue,
	DWORD Acceleration_khz_per_s,
	DWORD HighSpeed_Hz,
	DWORD Deceleration_khz_per_s,
	INT   MaxStroke_step,
	DWORD LowSpeed_Hz,
	INT   InitialPosition_step)
{
	if (!is_connected())
		return 0;
	if (ICNC_AxeHome(AxeID,
		InputNumber,
		ExpectedInputValue,
		Acceleration_khz_per_s,
		HighSpeed_Hz,
		Deceleration_khz_per_s,
		MaxStroke_step,
		LowSpeed_Hz,
		InitialPosition_step) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int cICNC2::Probe(DWORD Axis, DWORD Direction, DWORD InputNo, DWORD InputValue, DWORD MaxStroke, DWORD Speed, DWORD FStart, DWORD Accel, DWORD Decel)
{
	if (!is_connected())
		return 0;
	if (ICNC_Probe(Axis, Direction, InputNo, InputValue, MaxStroke, Speed, FStart, Accel, Decel) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::SlowStopAxes(DWORD Axes)
{
	if (!is_connected())
		return 0;
	if (ICNC_SlowStopMotors(Axes) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::SlowStopAllAndClear(void)
{
	if (!is_connected())
		return 0;
	if (ICNC_SlowStopAllAndClear() != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}




int cICNC2::SetAnalog(DWORD AnalogNo, DWORD AnalogValue)
{
	if (!is_connected())
		return 0;
	if (ICNC_SetAnalog(AnalogNo, AnalogValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


/// Parameters stuff
/// ****************
int   cICNC2::WriteParameter(DWORD ParameterID, DWORD ParameterValue, bool WaitCompletion, unsigned int TimeOut)
{
	if (!is_connected())
		return 0;
	if (ICNC_WriteParameter(ParameterID, ParameterValue, WaitCompletion, TimeOut) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;


}

int   cICNC2::ReadParameter(DWORD ParameterID, INT* ParameterValue)
{
	if (!is_connected())
		return 0;
	if (ICNC_ReadParameter(ParameterID, ParameterValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int   cICNC2::GetInput(DWORD InNo, INT* InputState)
{
	if (!is_connected())
		return 0;
	if (ICNC_GetInput(InNo, InputState) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::GetInputAll(DWORD* InputState) {
	if (!is_connected())
		return 0;
	if (ICNC_GetInputAll(InputState) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::SetOutput(int No, int State)
{

	if (!is_connected())
		return 0;
	if ((No < 1) || (No > 32))
		return 0;

	if (ICNC_SetOutput(No, State) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::SetOutputAll(unsigned int State) {
	if (!is_connected())
		return 0;

	if (ICNC_SetOutputAll(State) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;

}


int cICNC2::GetAllOutputState(DWORD* OutputState) {
	if (!is_connected())
		return 0;

	if (ICNC_GetOutputAll(OutputState) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::ReadUserMem(int MemNumber, DWORD* MemValue) {

	if (!is_connected())
		return 0;
	if (ICNC_ReadUserMem(MemNumber, MemValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int cICNC2::WriteUserMem(int MemNumber, DWORD MemValue) {

	if (!is_connected())
		return 0;
	if (ICNC_WriteUserMem(MemNumber, MemValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::SetSysRegisterValue(int SysItemID, DWORD SysValue) {
	if (!is_connected())
		return 0;

	if (ICNC_CustomCommand(ICNC_CMD_CUSTOM_SETSYS, SysItemID, 1, (unsigned char*)&SysValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::GetSysRegisterValue(int SysItemID, DWORD* SysValue) {
	if (!is_connected())
		return 0;
	if (ICNC_CustomCommand(ICNC_CMD_CUSTOM_GETSYS, SysItemID, 1, (unsigned char*)SysValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


// Bufferized functions 
// **********************************************************


int cICNC2::PushMoveSpeedAbs(DWORD Axes, DWORD Speed, INT PositionX, INT PositionY, INT PositionZ, INT PositionA, INT PositionB, DWORD* BufferRequired, BOOL UsingLocalBuffer) {
	DWORD res;
	if (!is_connected())
		return 0;
	if (!UsingLocalBuffer)
		res = ICNC_MoveSpeedAbsBuf(Axes, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB, BufferRequired);
	else
		res = ICNC_MoveSpeedAbsLocalBuf(Axes, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB, BufferRequired);
	if (res != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;

}


int cICNC2::PushMoveSpeedRel(DWORD Speed,
	INT MoveX,
	INT MoveY,
	INT MoveZ,
	INT MoveA,
	INT MoveB,
	DWORD* BufferRequired,
	BOOL UsingLocalBuffer) {
	DWORD res;
	if (!is_connected())
		return 0;

	if (!UsingLocalBuffer)
		res = ICNC_MoveSpeedRelBuf(Speed, MoveX, MoveY, MoveZ, MoveA, MoveB, BufferRequired);
	else
		res = ICNC_MoveSpeedRelLocalBuf(Speed, MoveX, MoveY, MoveZ, MoveA, MoveB, BufferRequired);

	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}


int cICNC2::PushMoveToRel(DWORD Speed,
	INT MoveX,
	INT MoveY,
	INT MoveZ,
	INT MoveA,
	INT MoveB,
	DWORD* BufferRequired) {
	if (!is_connected())
		return 0;
	if (ICNC_MoveProfileRelBuf(Speed, MoveX, MoveY, MoveZ, MoveA, MoveB, BufferRequired) != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int cICNC2::PushMoveToAbs(DWORD Axis,
	DWORD Speed,
	INT PositionX,
	INT PositionY,
	INT PositionZ,
	INT PositionA,
	INT PositionB,
	DWORD* BufferRequired) {
	if (!is_connected())
		return 0;
	if (ICNC_MoveProfileAbsBuf(Axis, Speed, PositionX, PositionY, PositionZ, PositionA, PositionB, BufferRequired) != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::PushWriteUserMem(int MemNumber, DWORD MemValue) {
	if (!is_connected())
		return 0;
	if (ICNC_WriteUserMemBuf(MemNumber, MemValue) != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;
}

/// <summary>
/// Push delay in buffer execution
/// </summary>
/// <param name="TimeToWait">unit is per 100ms</param>
/// <returns></returns>
int cICNC2::PushDelay(DWORD Delay) {
	if (!is_connected())
		return 0;
	if (ICNC_WaitBuf(Delay) != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;

}

int cICNC2::PushWaitForInputState(DWORD InputNo,
	BOOL InputValue,
	DWORD TimeOut,
	BOOL LockIfError,
	BOOL ClearBufferIfError)
{
	if (!is_connected())
		return 0;
	if (ICNC_WaitInputStateBuf(InputNo, InputValue, TimeOut, LockIfError, ClearBufferIfError) != ICNCUSB_SUCCESS)
	{
		Disconnected();
		return 0;
	}
	else
		return 1;

}


int cICNC2::PushSetOutput(int No, int State)
{

	if (!is_connected())
		return 0;
	if ((No < 1) || (No > 32))
		return 0;

	if (ICNC_SetOutputBuf(No, State) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}



int cICNC2::PushSetOutputAll(unsigned int State) {
	if (!is_connected())
		return 0;

	if (ICNC_SetOutputAllBuf(State) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;

}

int cICNC2::PushSetAnalog(DWORD AnalogNo, DWORD AnalogValue)
{
	if (!is_connected())
		return 0;
	if (ICNC_SetAnalogBuf(AnalogNo, AnalogValue) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else
		return 1;
}

int cICNC2::SetMBUserCoil(unsigned int CoilAddress, unsigned int CoilNumber, bool* coilState) {
#define COIL_BASE_ADDRESS 320
#define COIL_MAX_COIL_ADDRESS  96		// Number of available coils
	if (!is_connected())
		return 0;
	int res;
	int StartAddress = CoilAddress - COIL_BASE_ADDRESS;
	if ((CoilNumber > COIL_MAX_COIL_ADDRESS) || ((StartAddress + CoilNumber) > COIL_MAX_COIL_ADDRESS)) {
		throw (_T("Invalid Coils address or to much coils number"));
	}
	unsigned char data[3];
	for (unsigned int i = 0; i < CoilNumber; i++)
	{
		int Address = i + COIL_BASE_ADDRESS;
		data[0] = (unsigned char)((Address >> 8) & 0xFF);
		data[1] = (unsigned char)(Address & 0xFF);
		data[2] = (unsigned char)(coilState[i] != 0 ? 1 : 0);
		res = ICNC_MBControl(ICNC_CMD_MB_ONE_BIT_SET, data);
		if (res != ICNCUSB_SUCCESS) {
			Disconnected();
			return res;
		}
	}
	return res;
}

int cICNC2::GetMBHoldingRegisters(unsigned int Address, unsigned int number, unsigned short Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 31) || (number == 0)) {
		throw (_T("Invalid holding registers requested in one request (Number must be 1..31)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	res = ICNC_MBControl(ICNC_CMD_MB_GET_HOLDING_REGS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}

	for (unsigned int i = 0; i < number; i++)
	{
		// Modbus register is byte swapped, then, swap byte is necessary
		Values[i] = (unsigned short)(data[0 + i * 2] << 8) & 0xff00 | data[1 + i * 2];
	}
	return res;
}

int cICNC2::SetMBHoldingRegisters(unsigned int Address, unsigned int number, unsigned short Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 30) || (number == 0)) {
		throw (_T("Invalid holding registers requested in one request (Number must be 1..30)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	for (unsigned int i = 0; i < number; i++)
	{
		// Modbus register is byte swapped, then, swap byte is necessary
		data[(i * 2) + 4] = Values[i] >> 8;
		data[(i * 2) + 5] = Values[i] & 0xff;
	}
	res = ICNC_MBControl(ICNC_CMD_MB_SET_HOLDING_REGS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}

	return res;
}


int cICNC2::GetMBInputRegisters(unsigned int Address, unsigned int number, unsigned short Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 31) || (number == 0)) {
		throw (_T("Invalid input registers requested in one request (Number must be 1..31)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	res = ICNC_MBControl(ICNC_CMD_MB_GET_INPUT_REGS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}

	for (unsigned int i = 0; i < number; i++)
	{
		// Modbus register is byte swapped, then, swap byte is necessary
		Values[i] = (unsigned short)(data[0 + i * 2] << 8) & 0xff00 | data[1 + i * 2];
	}
	return res;
}




int cICNC2::GetMBCoils(unsigned int Address, unsigned int number, bool Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 496) || (number == 0)) {
		throw (_T("Invalid coils number (Number must be 1..496)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	res = ICNC_MBControl(ICNC_CMD_MB_GET_COILS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}

	int index = 0;
	int bitNumber = 0;
	unsigned char value = data[0];
	for (unsigned int i = 0; i < number; i++)
	{

		// Modbus register is byte swapped, then, swap byte is necessary
		Values[i] = (value & 1)!=0;
		value >>=  1;
		bitNumber++;
		if (bitNumber == 8) {
			value = data[++index];
			bitNumber = 0;
		}
	}
	return res;
}


int cICNC2::SetMBCoils(unsigned int Address, unsigned int number, bool Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 480) || (number == 0)) {
		throw (_T("Invalid coils number (Number must be 1..480)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	unsigned char value=0;
	int bitNumber = 0;
	int index = 4;
	for (unsigned int i = 0; i < number; i++) {
		if (Values[i]) value |= (1 << bitNumber);
		bitNumber++;
		if (bitNumber == 8) {
			data[index++] = value;
			value = 0;
			bitNumber = 0;
		}

	}
	if(bitNumber!=0)
		data[index] = value;	// Push las byte if necessary

	res = ICNC_MBControl(ICNC_CMD_MB_SET_COILS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}
	return res;
}


int cICNC2::GetMBInputs(unsigned int Address, unsigned int number, bool Values[]) {
	if (!is_connected())
		return 0;
	int res;
	if ((number > 496) || (number == 0)) {
		throw (_T("Invalid input bit number (Number must be 1..496)"));
	}
	unsigned char data[64];
	data[0] = (unsigned char)((Address >> 8) & 0xFF);
	data[1] = (unsigned char)(Address & 0xFF);
	data[2] = (unsigned char)((number >> 8) & 0xFF);
	data[3] = (unsigned char)(number & 0xFF);

	res = ICNC_MBControl(ICNC_CMD_MB_GET_INPUTS, data);
	if (res != ICNCUSB_SUCCESS) {
		Disconnected();
		return res;
	}

	int index = 0;
	int bitNumber = 0;
	unsigned char value = data[0];
	for (unsigned int i = 0; i < number; i++)
	{

		// Modbus register is byte swapped, then, swap byte is necessary
		Values[i] = (value & 1)!=0;
		value >>= 1;
		bitNumber++;
		if (bitNumber == 8) {
			value = data[++index];
			bitNumber = 0;
		}
	}
	return res;
}




int cICNC2::SendPLCCommand(char* cmd) {
	if (!is_connected())
		return 0;
	if (ICNC_SendPLCCommand(cmd) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else {
		return 1;
	}
}

DllExport int cICNC2::ReadPLCMonitor(char* cmd) {
	if (!is_connected())
		return 0;
	if (ICNC_ReadPLCMonitor(cmd) != ICNCUSB_SUCCESS) {
		Disconnected();
		return 0;
	}
	else {
		return 1;
	}
}


/*
Management of disconnect event
*/

void cICNC2::RegisterOnDisconnected(DisconnectedCallback_t callback, PVOID arg) {
	m_DisconnectedCallback_handle = callback;
	m_DisconnectedCallback_arg = arg;
}

DllExport void cICNC2::OnConnectChanged(bool connected, PVOID arg)
{
	//TCHAR s[256];
	//sprintf(s, _T("New connected state = %d\n"), connected);
	//OutputDebugString(s);
	if (m_OnConnectChangedEventHandle != NULL)
		m_OnConnectChangedEventHandle(m_connected, arg);
}

void cICNC2::Disconnected(void) {
	bool initialState = m_connected;
	m_connected = false;
	ICNC_Disconnect();
	if (m_DisconnectedCallback_handle != NULL)
		m_DisconnectedCallback_handle(m_DisconnectedCallback_arg);
	// Notify if connected state changed
	if (initialState != m_connected)
		OnConnectChanged(m_connected, m_OnConnectChangedEventArg);
}

/*
Management of perdiodic status registers read
*/



static void CALLBACK _f_read_callback(PVOID CallbackParam, BOOLEAN TimerOrWaitFired)
{

	PeriodicReadCallback_param_t* prm = static_cast<PeriodicReadCallback_param_t*>(CallbackParam);
	if (prm->hICNC2 == NULL) return;
	if (!prm->hICNC2->is_connected()) return;

	if (prm->hICNC2->GetBoardStatus(prm->FlagBit, prm->Result) == ICNCUSB_SUCCESS) {
		prm->callback(prm->ID, prm->Result, prm->arg);
	}
}


HANDLE cICNC2::RegisterPeriodicStatusRead(DWORD RequestID, DWORD ICNC_STATUS_REGISTER_MASK, int periode, PeriodicReadCallback_t callback, PVOID arg)
{

	if (ICNC_STATUS_REGISTER_MASK == 0) return NULL;

	PeriodicReadCallback_param_t* callback_prm = new PeriodicReadCallback_param_t;
	if (callback_prm == NULL) return NULL;

	callback_prm->ID = RequestID;
	callback_prm->hICNC2 = this;
	callback_prm->arg = arg;
	callback_prm->callback = callback;

	// Store bitfield and count of seted bit
	callback_prm->FlagBit = ICNC_STATUS_REGISTER_MASK;
	callback_prm->NbRegisters = countSetBits(ICNC_STATUS_REGISTER_MASK);

	// Allocation for results array
	callback_prm->Result = new DWORD[callback_prm->NbRegisters];
	if (callback_prm->Result == NULL) {
		delete callback_prm;
		return NULL;
	}


	if (m_hTimerQueue == NULL)
		m_hTimerQueue = CreateTimerQueue();
	CreateTimerQueueTimer(&callback_prm->hTimer, m_hTimerQueue, (WAITORTIMERCALLBACK)_f_read_callback, callback_prm, periode, periode, 0);

	if (m_RegisteredCallbackList == NULL)
		m_RegisteredCallbackList = (PVOID)makelist();

	// Savbe allocated memory information for automatic clean up of memory
	addNode((int)callback_prm, (List*)m_RegisteredCallbackList);

	return callback_prm;
}

void cICNC2::UnregisterPeriodicStatusRead(HANDLE hPeriodicRead)
{

	bool res;
	PeriodicReadCallback_param_t* prm = (PeriodicReadCallback_param_t*)hPeriodicRead;

	if (prm == NULL) return;
	if (prm->hTimer != NULL)
		//DeleteTimerQueue(prm->hTimer);
		// Delete le timer de la queue mais attend la fin d'eventuels traitements en cours
		res = DeleteTimerQueueTimer(m_hTimerQueue, prm->hTimer, INVALID_HANDLE_VALUE);
	(void)res;	// Just to avoid warning for unused variable

	// free Array of results
	if (prm->Result != NULL)
		delete(prm->Result);
	// free callback informations
	deleteNode((int)prm, (List*)m_RegisteredCallbackList, true);

}


//
//void cICNC2::RegisterDebug(DebugCallback_t callback, PVOID Arg)
//{
//	DebugHandler = (CONSOLE_HANDLER)callback;
//	m_DebugArg = Arg;
//}


//typedef struct {
//	DWORD ID;
//	DWORD FlagBit;
//	int NbRegisters;
//	nfTimer* Timer;
//	PDWORD Result;  // For return result
//	cICNC2* hICNC2;
//	PeriodicReadCallback_t callback;
//	PVOID arg;
//} PeriodicReadCallback_param_t;
//
//
//class my_nfTimer : public nfTimer
//{
//public:
//	my_nfTimer(cICNC2* ICNC2, DWORD RequestID, DWORD ICNC_STATUS_REGISTER_MASK, int periode, PeriodicReadCallback_t callback, PVOID arg) : nfTimer()
//	{
//		callback_prm.ID = RequestID;
//		callback_prm.hICNC2 = ICNC2;
//		callback_prm.arg = arg;
//		callback_prm.callback = callback;
//
//		// Store bitfield and count of seted bit
//		callback_prm.FlagBit = ICNC_STATUS_REGISTER_MASK;
//		callback_prm.NbRegisters = countSetBits(ICNC_STATUS_REGISTER_MASK);
//
//		// Allocation for results array
//		callback_prm.Result = new DWORD[callback_prm.NbRegisters];
//
//		Start(periode, false);
//
//	}
//
//	void Notify()
//	{
//		if (callback_prm.hICNC2 == NULL) return;
//		if (callback_prm.hICNC2->GetBoardStatus(callback_prm.FlagBit, callback_prm.Result) == ICNCUSB_SUCCESS) {
//			callback_prm.callback(callback_prm.ID, callback_prm.Result, callback_prm.arg);
//		}
//		return;
//	}
//
//	virtual ~my_nfTimer() {
//		delete callback_prm.Result;
//	};
//
//
//
//private:
//	PeriodicReadCallback_param_t callback_prm;
//};
//
//
//
//
//static VOID CALLBACK _f_read_callback(PVOID CallbackParam, BOOLEAN TimerOrWaitFired)
//{
//
//	PeriodicReadCallback_param_t* prm = (PeriodicReadCallback_param_t*)CallbackParam;
//
//
//	if (prm->hICNC2 == NULL) return;
//	if (prm->hICNC2->GetBoardStatus(prm->FlagBit, prm->Result) == ICNCUSB_SUCCESS) {
//		prm->callback(prm->ID, prm->Result, prm->arg);
//	}
//}
//
//
//
//
//HANDLE cICNC2::RegisterPeriodicStatusRead(DWORD RequestID, DWORD ICNC_STATUS_REGISTER_MASK, int periode, PeriodicReadCallback_t callback, PVOID arg)
//{
//
//	if (ICNC_STATUS_REGISTER_MASK == 0) return NULL;
//
//	my_nfTimer* Timer = new  my_nfTimer(this, RequestID, ICNC_STATUS_REGISTER_MASK, periode, callback, arg);
//	return Timer;
//}
//
//void cICNC2::UnregisterPeriodicStatusRead(HANDLE hPeriodicRead)
//{
//	if (hPeriodicRead != NULL)
//		delete hPeriodicRead;
//}
//
//
