#if !defined(AFX_ICNC2_H__36F00088_AEAA_488C_A619_C375238A6BA5__INCLUDED_)
#define AFX_ICNC2_H__36F00088_AEAA_488C_A619_C375238A6BA5__INCLUDED_

#pragma once

#include <windows.h>
#include <TCHAR.H>
#include "GlobalDef.h"


#pragma warning( push )
#pragma warning( disable : 4635 )

#ifdef ICNC2_VS_EXPORTS
#define DllExport    __declspec( dllexport )
#else
#define DllExport   __declspec( dllimport )
#endif


// *********************************************************************
// Following path must be adjust according to your project configuration
// *********************************************************************
//#ifndef ICNC2_VS_EXPORTS
//#if _WIN64
//#ifdef _DEBUG
//#pragma comment(lib,"../x64/debug/icnc2_vs.lib")
//#else
//#pragma comment(lib,"../x64/release/icnc2_vs.lib")
//#endif
//#endif
//#if _WIN32
//#ifdef _DEBUG
//#pragma comment(lib,"../debug/icnc2_vs.lib")
//#else
//#pragma comment(lib,"../release/icnc2_vs.lib")
//#endif
//#endif
//#endif


/// <summary>
/// Structure with InterpCNC controller technical informations.
/// Those data are updated automatically after connect success.
/// A cICNC2 class member is available in cICNC2::m_SysInfo
/// </summary>
typedef struct {
	unsigned int BufferSize;
	unsigned int MaxPulseFrequency;
	unsigned int FirmwareVersionH;
	unsigned int FirmwareVersionL;
	unsigned int BootloaderVersionH;
	unsigned int BootloaderVersionL;
	unsigned int BoardHWVersion;
	unsigned int EEPROMSize;
	unsigned int UserMemAvailable;
	unsigned int AOUTAvailable;
	unsigned int AINAvailable;
	unsigned int DACResolution;
	unsigned int ADCResolution;
	unsigned int DOUTAvailable;
	unsigned int DINAvailable;
}SysInfo_s;


//Type used for registered callback on status register reading (periodic read)
typedef void(*PeriodicReadFunc_t)(int RequestID, DWORD* data, PVOID arg);
typedef PeriodicReadFunc_t PeriodicReadCallback_t;

//Type used for registered callback on status register reading (periodic read)
typedef void(*DisconnectedFunc_t)(PVOID arg);
typedef DisconnectedFunc_t DisconnectedCallback_t;

typedef void(*DebugFunc_t)(PTCHAR msg, PVOID arg);
typedef DebugFunc_t DebugCallback_t;

typedef void(*OnConnectChangedEvent)(bool ConnectionState, PVOID arg);
typedef OnConnectChangedEvent OnConnectChangedEvent_t;

/* Class wrapper to USB communications functions with InterpCNC V2 */
class cICNC2 {
public:

	DllExport cICNC2(bool AutoConnect = false);
	//DllExport cICNC2(bool AutoConnect);

	DllExport ~cICNC2();


	/// <summary>
	/// Reset error flags define in ErrorStatusBit
	/// </summary>
	/// <param name=ErrorStatusBit">
	/// can be a combination of ICNC_STATUS_BIT_EMERGENCYSTOP, ICNC_STATUS_BIT_LOCKED, ICNC_STATUS_BIT_STROKE_LIMIT,
	/// ICNC_STATUS_BIT_EEWRITE_ERROR, ICNC_STATUS_BIT_HOMING_ERROR
	/// </params>
	/// <returns>1 if communication success</returns>
	DllExport int ErrorReset(DWORD ErrorStatusBit);

	/// <summary>
	/// Lock the controller to avoid any movement. Unlock can be done only with ErrorReset call
	/// </summary>
	/// <returns>1 if communication success</returns>
	DllExport int Lock(void);


	/// <summary>
	/// Reset all ICNC2 error flags and unlock if Enable input is set
	/// </summary>
	/// <returns>1 if communication success</returns>
	DllExport int ErrorResetAll() {
		return ErrorReset(ICNC_STATUS_BIT_EMERGENCYSTOP |
			ICNC_STATUS_BIT_LOCKED |
			ICNC_STATUS_BIT_STROKE_LIMIT |
			ICNC_STATUS_BIT_EEWRITE_ERROR |
			ICNC_STATUS_BIT_HOMING_ERROR);

	}

	/// <summary>
	/// Connect to the InterpCNC thrue USB chanel. Only one board must be connected. 
	/// is_connected() methode will also reflect the connection result(true or false)	
	/// On connect success, m_SysInfo members are updated from the controller
	/// </summary>
	/// <returns>1 if connect successed</returns>
	DllExport int Connect(void);

	/// <summary>
	/// Disconnect from USB
	/// If periodic status read existing, they are removed.
	/// </summary>
	DllExport void DisConnect(void);

	/// <summary>
	/// Check if USB link as been established
	/// In case of communication error, connection state will be reset automatically.
	/// </summary>
	/// <returns>true if connected</returns>
	DllExport bool is_connected(void) {
		if (m_AutoConnect) {
			if (!m_connected)
				return Connect();
			else
				return true;
		}
		else
			return m_connected;
	};

	/// <summary>Read some board system information</summary>
	/// <param name="SysInfoFlag">bits field of expected SysInfo registers (ored combination of ICNC_SYS_INFO_xxxxx bit</param>
	/// <param name="SysInfoValue">Array's pointer of SysInfo registers reads from InterpCNC (at least, one registers per selected bit). Order inside Status output array is depending of the weight of each bit</param>
	/// <returns>1 if success read</returns>
	DllExport int GetSysInfo(DWORD SysInfoFlag,
		DWORD* SysInfoValue);

	/// <summary>
	/// Read some status registers
	/// </summary>
	/// <param name="StatusType">bits field of expected status register</param>
	/// <param name="Status">Array of Status registers reads from InterpCNC (at least, one registers per selected bit). Order inside Status output array is depending of the weight of each bit</param>
	/// <returns>1 if success read</returns>
	DllExport int GetBoardStatus(DWORD StatusType,
		DWORD* Status);


	/// <summary>
	/// Read one Digital Inputs state (from DIN1 to DIN32)
	/// </summary>
	/// <param name="InputState">Pointer to return value</param>
	/// <returns>1 if success read</returns>
	DllExport int GetInput(DWORD InNo, INT* InputState);

	/// <summary>
	/// Read physical Digital Inputs state (DIN1 to DIN32)
	/// </summary>
	/// <param name="InputState">Pointer to return value</param>
	/// <returns>1 if success read</returns>
	DllExport int GetInputAll(DWORD* InputState);


	/// <summary>
	/// Read actual output state of all DOUT
	/// </summary>
	/// <param name="OutputState">Pointer to return value</param>
	/// <returns>1 if success read</returns>
	DllExport int GetAllOutputState(DWORD* OutputState);




	/// **************************
	/// Axes stuffs
	/// **************************

	/// <summary>
	/// Initialisation of position conter.
	/// Only axes with related bit seted in Axes parameter will be affected.
	/// </summary>
	/// <param name="Axes">Field bit of axes to initialize</param>
	/// <param name="PositionX">new X counter position if bit 1 of axes is set</param>
	/// <param name="PositionY">new Y counter position if bit 2 of axes is set</param>
	/// <param name="PositionZ">new Z counter position if bit 3 of axes is set</param>
	/// <param name="PositionA">new A counter position if bit 4 of axes is set</param>
	/// <param name="PositionB">new B counter position if bit 5 of axes is set</param>
	/// <returns></returns>
	DllExport int SetPosition(DWORD Axes,
		INT PositionX,
		INT PositionY,
		INT PositionZ,
		INT PositionA,
		INT PositionB);



	/// <summary>
	/// Start or on the fly update independent movement on one axis
	/// Move state can be read from status register ICNC_STATUS_BOARD_STATUS, bits ICNC_STATUS_BIT_x_ASYNC_MOVING
	/// </summary>
	/// <param name="Axe">Axis number (1 to 5).</param>
	/// <param name="FStartStop">Start/Stop frequency of speed profile (Pulses/s)</param>
	/// <param name="Accel">Speed profile acceleration (KHz/s)</param>
	/// <param name="Speed">Maximum speed profile (Hz)</param>
	/// <param name="Decel">Speed profile deceleration (KHz/s)</param>
	/// <param name="Position">Absolute target position (Steps)</param>
	/// <returns>1 if success</returns>	
	DllExport int MoveProfileAbsAsync(DWORD Axe,
		DWORD FStartStop,
		DWORD Accel,
		DWORD Speed,
		DWORD Decel,
		INT Position);


	/// <summary>
	/// Start homing sequence for individual or multiple axes
	/// Homing parameters are defined in InterpCNC parameters.
	/// Only max stroke have to be define in the function call parameters
	/// If X is coupled to A or B from parameter, both master and slave will move together
	/// Homing status and error can be read from ICNC_STATUS_BOARD_STATUS2 register
	/// A Machine Home can be lauch to homing several axes together but while homing in progress,
	/// You can't start a new MachineHome call
	/// </summary>
	/// <param name="Axes">Axes bitfield selection. Exemple Axis=ICNC_AXE_Z for Z homing; Axis=ICNC_AXE_X|ICNC_AXE_Y for X and Y homing together</param>
	/// <param name="MaxStrokeX">Maximum X axe stroke before to get the homing switch. Ignored if ICNC_AXE_X is not set</param>
	/// <param name="MaxStrokeY">Maximum Y axe stroke before to get the homing switch. Ignored if ICNC_AXE_Y is not set</param>
	/// <param name="MaxStrokeZ">Maximum Z axe stroke before to get the homing switch. Ignored if ICNC_AXE_Z is not set</param>
	/// <param name="MaxStrokeA">Maximum A axe stroke before to get the homing switch. Ignored if ICNC_AXE_A is not set</param>
	/// <param name="MaxStrokeB">Maximum B axe stroke before to get the homing switch. Ignored if ICNC_AXE_B is not set</param>
	/// <returns>1 if success</returns>	
	DllExport int MachineHome(DWORD Axes,
		DWORD MaxStrokeX,
		DWORD MaxStrokeY,
		DWORD MaxStrokeZ,
		DWORD MaxStrokeA,
		DWORD MaxStrokeB);


	/// <summary>
	/// Launch homing sequence on one axe. This homing using asynchronous motion and then,
	/// you can send several command to for diffents axes. It will be executed in parallel thread.
	/// ATTENTION : this command don't care about master/slave axes setting
	/// Individual status bit are available for each axes (ICNC_STATUS2_BIT_HOMING_x and ICNC_STATUS2_BIT_HOMING_X_ERROR from ICNC_STATUS_BOARD_STATUS2)
	/// </summary>
	/// <param name="AxeID">Axe ID (from 1 to 5)</param>
	/// <param name="InputNumber">Input number used as reference point (1..32)</param>
	/// <param name="ExpectedInputValue">Input value when switch is engaged (0 for NC, 1 for NO)</param>
	/// <param name="Acceleration_khz_per_s">Acceleration of motion (KHz/s)</param>
	/// <param name="HighSpeed_Hz">Speed for going to the switch (Pulses/s)</param>
	/// <param name="Deceleration_khz_per_s">Deceleration (KHz/s)</param>
	/// <param name="MaxStroke_step">Maximum stroke to reach the switch. It also define the homing motion direction (negative value for homing in negative direction)</param>
	/// <param name="LowSpeed_Hz">Speed used for reversed motion to release the switch</param>
	/// <param name="InitialPosition_step">Position used to initialized the position conter à end of sequence</param>
	/// <returns></returns>
	DllExport int AxeHome(DWORD AxeID,
		DWORD InputNumber,
		DWORD ExpectedInputValue,
		DWORD Acceleration_khz_per_s,
		DWORD HighSpeed_Hz,
		DWORD Deceleration_khz_per_s,
		INT   MaxStroke_step,
		DWORD LowSpeed_Hz,
		INT   InitialPosition_step = 0);


	/// <summary>
	/// Start sensor probe based on one axis movement
	/// Probing status and error can be read from ICNC_STATUS_BOARD_STATUS  register (ICNC_STATUS_BIT_PROBING and ICNC_STATUS_BIT_PROBING)
	/// ICNC_STATUS_BIT_PROBING will remain actif while probing is runing (motion on selected axis),
	/// When ICNC_STATUS_BIT_PROBING is reseted, you can confirm the success with ICNC_STATUS_BIT_PROBING (should not be set)
	/// If all right, read the switching position from status register ICNC_STATUS_LAST_PROBE
	/// </summary>
	/// <param name="Axis">Axe bit indicator  (from ICNC_AXE_X to ICNC_AXE_B)</param>
	/// <param name="Direction">0 for negative move direction, 1 for positive move direction</param>
	/// <param name="InputNo">Input number of probe sensor (1..32)</param>
	/// <param name="InputValue">Expected input state when sensor is active</param>
	/// <param name="MaxStroke">Max stroke to move before sensor detection. If move over this stroke, Probe will be stopped and ICNC_STATUS_BIT_PROBING_ERROR while be set</param>
	/// <param name="Speed">Axis speed (pulses/s)</param>
	/// <param name="FStart">Start/Stop frequency of motion speed (Hz)</param>
	/// <param name="Accel">Accel of movement (KHz/s)</param>
	/// <param name="Decel">Deceleration of mouvement. Should be high in case of rigid sensor (KHz/s)</param>
	/// <returns>1 if success</returns>	
	DllExport int Probe(DWORD Axis,
		DWORD Direction,
		DWORD InputNo,
		DWORD InputValue,
		DWORD MaxStroke,
		DWORD Speed,
		DWORD FStart,
		DWORD Accel,
		DWORD Decel);




	/// <summary>
	/// Stop one or several axes with deceleration value based on last move command
	/// This command can be used to stop an Asynchrone motion before excpepted target (or homing sequence or probing sequence)
	/// </summary>
	/// <param name="Axes">Axes ID combination bit (ICNC_AXE_X | ICNC_AXE_Y | ICNC_AXE_Z ...)</param>
	/// <returns>1 if success</returns>	
	DllExport int SlowStopAxes(DWORD Axes);


	/// <summary>
	/// Stop all axes with decelaration and purge the motion/command buffer
	/// </summary>
	DllExport int SlowStopAllAndClear(void);




	/// <summary>
	/// Set Analog output value
	/// The resolution of analog output can be read from GetSysInfo with ICNC_SYS_INFO_DAC_RESOLUTION bit request
	/// The Number of DAC chanel can be obtain GetSysInfo with ICNC_SYS_INFO_AVAILABLE_AOUT bit request
	/// </summary>
	/// <remarks> On ICNC2 4 axes version, the third analog aoutput chanel is linked to the PWM output</remarks>
	/// <param name="AnalogNo">Analog output number (1 to max analog output number)</param>
	/// <param name="AnalogValue">Analog output value (based on DAC resolution = 1024 points on ICNC2</param>
	/// <param name="PositionX">Target position of X axis. Ignored if bit ICNC_AXE_X is not set in Axes</param>
	DllExport int SetAnalog(DWORD AnalogNo,
		DWORD AnalogValue);



	// Parameters stuff
	// ********************************************************************************

	/// <summary>
	/// Write ICNC2 parameters (stored in EEPROM)
	/// Write operation can be synchronous or asynchronous
	/// <returns><strong>We strongly recommand to use synchronized call (with WaitCompletion = true and 250ms timeout)</strong></returns>
	/// </summary>
	/// <param name="ParameterID">Parameter ID</param>
	/// <param name="ParameterValue">New parameter value</param>
	/// <param name="WaitCompletion">wait inside function for write completed on the board</param>
	/// <param name="TimeOut">Timeout of wait completion (ms)</param>
	/// <returns>1 if success</returns>	
	DllExport int WriteParameter(DWORD ParameterID,
		DWORD ParameterValue,
		bool WaitCompletion = TRUE,
		unsigned int TimeOut = 250);

	/// <summary>
	/// Read ICNC2 parameters (stored in EEPROM)
	/// </summary>
	/// <param name="ParameterID">Parameter ID</param>
	/// <param name="ParameterValue">Pointer to return value</param>
	/// <returns>1 if success</returns>	
	DllExport int ReadParameter(DWORD ParameterID,
		INT* ParameterValue);

	/// <summary>
	/// Set individual Digital output state
	/// </summary>
	/// <param name="No">DOUT number from 1 to 32</param>
	/// <param name="State">New output state (0 or 1)</param>
	/// <returns>1 if success</returns>	
	DllExport int SetOutput(int No,
		int State);

	/// <summary>
	/// Set all Digital output state together (32 outputs states)
	/// </summary>
	/// <param name="State">New output state</param>
	/// <returns>1 if success</returns>	
	DllExport int SetOutputAll(unsigned int State);



	/// <summary>
	/// Read from one of available RAM user memory (m_SysInfo.UserMemAvailable are avaialble)
	/// </summary>
	/// <param name="MemNumber">Memory ID from 0 to m_SysInfo.UserMemAvailable-1</param>
	/// <param name="MemValue">Pointer to return value</param>
	/// <returns>1 if success</returns>	
	DllExport int ReadUserMem(int MemNumber,
		DWORD* MemValue);
	/// <summary>
	/// Immediate write to one of available RAM user memory (m_SysInfo.UserMemAvailable are avaialble)
	/// </summary>
	/// <param name="MemNumber">Memory ID from 0 to m_SysInfo.UserMemAvailable-1</param>
	/// <param name="MemValue">New value to write</param>
	/// <returns>1 if success</returns>	
	DllExport int WriteUserMem(int MemNumber,
		DWORD MemValue);


	DllExport int SetSysRegisterValue(int SysItemID,
		DWORD SysValue);

	DllExport int GetSysRegisterValue(int SysItemID,
		DWORD* SysValue);






	/// Bufferized fonctions
	///**************************************************************
	///**************************************************************


	/// <summary>
	/// Move with constant speed from actual position to target position
	/// The speed is related to the major axis speed. All other axes speed will be adjust inside the firmware
	/// according to the major axis speed to get the linear interpolation.
	/// </summary>
	/// <param name="Axes">Axes flags bit to consider (or combiation of ICNC_AXE_x bits)</param>
	/// <param name="Speed">Axis velocity of major axis (Hz or pulses/s)</param>
	/// <param name="PositionX">Target position of X axis. Ignored if bit ICNC_AXE_X is not set in Axes</param>
	/// <param name="PositionY">Target position of Y axis. Ignored if bit ICNC_AXE_Y is not set in Axes</param>
	/// <param name="PositionZ">Target position of Z axis. Ignored if bit ICNC_AXE_Z is not set in Axes</param>
	/// <param name="PositionA">Target position of A axis. Ignored if bit ICNC_AXE_A is not set in Axes</param>
	/// <param name="PositionB">Target position of B axis. Ignored if bit ICNC_AXE_B is not set in Axes</param>
	/// <param name="BufferRequired">Estimation of required space in motion/command buffer for this command</param>
	/// <param name="UsingLocalBuffer">To merge several command before real transmission (for USB communication improvement)</param>
	/// <returns>1 if command successfull</returns>	
	DllExport int PushMoveSpeedAbs(DWORD Axes,
		DWORD Speed,
		INT PositionX,
		INT PositionY,
		INT PositionZ,
		INT PositionA,
		INT PositionB,
		DWORD* BufferRequired,
		BOOL UsingLocalBuffer = FALSE);

	/// <summary>
	/// Relative move from actual position with constant speed (no speed profile) 
	/// The speed is related to the major axis speed. All other axes speed will be adjust inside the firmware
	/// according to the major axis speed to get the linear interpolation.
	/// </summary>
	/// <param name="Speed">Axis velocity of major axis (Hz or pulses/s)</param>
	/// <param name="MoveX">Increment of X axis. Ignored if value is 0</param>
	/// <param name="MoveY">Increment Y axis.  Ignored if value is 0</param>
	/// <param name="MoveZ">Increment Z axis.  Ignored if value is 0</param>
	/// <param name="MoveA">Increment A axis.  Ignored if value is 0</param>
	/// <param name="MoveB">Increment B axis.  Ignored if value is 0</param>
	/// <param name="BufferRequired">Estimation of required space in motion/command buffer for this command</param>
	/// <param name="UsingLocalBuffer">To merge several command before real transmission (for USB communication improvement)</param>
	/// <returns>1 if command successfull</returns>	
	DllExport int PushMoveSpeedRel(DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired,
		BOOL UsingLocalBuffer = FALSE);


	/// <summary>
	/// Relative move from actual position with speed profile 
	/// The speed is related to the major axis speed. All other axes speed will be adjust inside the firmware
	/// according to the major axis speed to get the linear interpolation.
	/// </summary>
	/// <param name="Speed">Axis velocity of major axis (Hz or pulses/s)</param>
	/// <param name="MoveX">Increment of X axis. Ignored if value is 0</param>
	/// <param name="MoveY">Increment Y axis.  Ignored if value is 0</param>
	/// <param name="MoveZ">Increment Z axis.  Ignored if value is 0</param>
	/// <param name="MoveA">Increment A axis.  Ignored if value is 0</param>
	/// <param name="MoveB">Increment B axis.  Ignored if value is 0</param>
	/// <param name="BufferRequired">Estimation of required space in motion/command buffer for this command</param>
	/// <returns>1 if command successfull</returns>	
	DllExport int PushMoveToRel(DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired);


	/// <summary>
	/// Move to absolute position from actual position with speed profile 
	/// The speed is related to the major axis speed. All other axes speed will be adjust inside the firmware
	/// according to the major axis speed to get the linear interpolation.
	/// </summary>
	/// <param name="Axes">Axes flags bit to consider (or combiation of ICNC_AXE_x bits)</param>
	/// <param name="Speed">Axis velocity of major axis (Hz or pulses/s)</param>
	/// <param name="PositionX">Target position of X axis. Ignored if bit ICNC_AXE_X is not set in Axes</param>
	/// <param name="PositionY">Target position of Y axis. Ignored if bit ICNC_AXE_Y is not set in Axes</param>
	/// <param name="PositionZ">Target position of Z axis. Ignored if bit ICNC_AXE_Z is not set in Axes</param>
	/// <param name="PositionA">Target position of A axis. Ignored if bit ICNC_AXE_A is not set in Axes</param>
	/// <param name="PositionB">Target position of B axis. Ignored if bit ICNC_AXE_B is not set in Axes</param>
	/// <param name="BufferRequired">Estimation of required space in motion/command buffer for this command</param>
	/// <returns>1 if command successfull</returns>	
	DllExport int PushMoveToAbs(DWORD Axis,
		DWORD Speed,
		INT PositionX,
		INT PositionY,
		INT PositionZ,
		INT PositionA,
		INT PositionB,
		DWORD* BufferRequired);



	/// <summary>
	/// Bufferized write to one of available RAM user memory (m_SysInfo.UserMemAvailable are avaialble)
	/// Could be used to control the buffer treatement
	/// </summary>
	/// <param name="MemNumber">Memory ID from 0 to m_SysInfo.UserMemAvailable-1</param>
	/// <param name="MemValue">New value to write</param>
	/// <returns>1 if success</returns>	
	DllExport int PushWriteUserMem(int MemNumber,
		DWORD MemValue);

	/// <summary>
	/// Bufferized pause in motion/command buffer excecution
	/// </summary>
	/// <param name="Delay">Delay duration (per 100ms)</param>
	/// <returns>1 if success</returns>	
	DllExport int PushDelay(DWORD Delay);

	/// <summary>
	/// Bufferized wait for input state. While Input state is different from Expected status, the buffer will be freezed
	/// If expected state don't occur before timeout, the board can be locked and/or the buffer can be flush
	/// </summary>
	/// <param name="InputNo">Input number to control (1..32)</param>
	/// <param name="InputValue">Expected value on the input to continue</param>
	/// <param name="TimeOut">Time allowed to get expected value</param>
	/// <param name="LockIfError">If set, the controlle will be locked to avoid any action in case of timeout</param>
	/// <param name="ClearBufferIfError">If set, the controlle will flush the buffer in case of timeout</param>
	/// <returns>1 if success</returns>	
	DllExport int PushWaitForInputState(DWORD InputNo,
		BOOL InputValue,
		DWORD TimeOut,
		BOOL LockIfError,
		BOOL ClearBufferIfError);

	/// <summary>
	/// Bufferized Set individual Digital output state
	/// </summary>
	/// <param name="No">DOUT number from 1 to 32</param>
	/// <param name="State">New output state (0 or 1)</param>
	/// <returns>1 if success</returns>	
	DllExport int PushSetOutput(int No,
		int State);

	/// <summary>
	/// Bufferized Set all Digital output state together (32 outputs states)
	/// </summary>
	/// <param name="State">New output state</param>
	/// <returns>1 if success</returns>	
	DllExport int PushSetOutputAll(unsigned int State);


	/// <summary>
	/// Set Analog output value
	/// The resolution of analog output can be read from GetSysInfo with ICNC_SYS_INFO_DAC_RESOLUTION bit request
	/// The Number of DAC chanel can be obtain GetSysInfo with ICNC_SYS_INFO_AVAILABLE_AOUT bit request
	/// </summary>
	/// <remarks> On ICNC2 4 axes version, the third analog aoutput chanel is linked to the PWM output</remarks>
	/// <param name="AnalogNo">Analog output number (1 to max analog output number)</param>
	/// <param name="AnalogValue">Analog output value (based on DAC resolution = 1024 points on ICNC2</param>
	/// <param name="PositionX">Target position of X axis. Ignored if bit ICNC_AXE_X is not set in Axes</param>
	DllExport int PushSetAnalog(DWORD AnalogNo,
		DWORD AnalogValue);


	// End of bufferized commands
	// ****************************************************************************************************************************

	//*****************************************************************************************************************************
	// Modbus related commands
	DllExport int SetMBUserCoil(unsigned int CoilAddress, unsigned int CoilNumber, bool* coilState);
		
	DllExport int GetMBHoldingRegisters(unsigned int Address, unsigned int number, unsigned short Values[]);
	DllExport int SetMBHoldingRegisters(unsigned int Address, unsigned int number, unsigned short Values[]);
	DllExport int GetMBInputRegisters(unsigned int Address, unsigned int number, unsigned short Values[]);
	DllExport int GetMBCoils(unsigned int Address, unsigned int number, bool Values[]);
	DllExport int GetMBInputs(unsigned int Address, unsigned int number, bool Values[]);
	DllExport int SetMBCoils(unsigned int Address, unsigned int number, bool Values[]);



	// End of modbus commands
// ****************************************************************************************************************************


//*****************************************************************************************************************************
// PLC related commands

/// <summary>
/// Send a command to the PLCBasic interpreter.
/// if the command begin with a line number, it will be save in RAM but not executed.
/// if the command don't start with a line number, it will be execute immediatly.
/// A command must be terminated with \n caracter
/// Long command will be splitted to respect the maximum of 64 caracters per frame on USB bus
/// </summary>
/// <param name="cmd">string of command to send</param>
/// <returns></returns>
	DllExport int SendPLCCommand(char* cmd);



	/// <summary>
	/// Send STOP PLC command
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	DllExport int StopPLC(void) {
		return SendPLCCommand("\3");
	}

	/// <summary>
	/// Read char from PLC monitor. Should be call with short sampling time to avoid caracter lost from Monitor buffer
	/// </summary>
	/// <param name="cmd">Pointer to a buffer with enough space (at least, 64 char) for return values</param>
	/// <returns></returns>
	DllExport int ReadPLCMonitor(char* cmd);

	// End of PLC commands
	// ****************************************************************************************************************************

	/// <summary>
	/// Create a periodic timer to read one or several status registers
	/// After each read, a callback is call
	/// </summary>
	/// <param name="RequestID">useful user identifier to distinguish request callback if needed</param>
	/// <param name="ICNC_STATUS_REGISTER_MASK">bits field of required status register to read</param>
	/// <param name="periode">refresh period (ms)</param>
	/// <param name="callback">PeriodicReadCallback_t type callback function handle </param>
	/// <param name="arg">handle for your arguments passed to the callback</param>
	/// <returns>A handle on registred event</returns>
	DllExport HANDLE RegisterPeriodicStatusRead(DWORD RequestID, DWORD ICNC_STATUS_REGISTER_MASK, int periode, PeriodicReadCallback_t callback, PVOID arg);

	/// <summary>
	/// Remove registred periodic timer (previously allocated with RegisterPeriodicStatusRead) 
	/// </summary>
	/// <param name="hPeriodicRead"><Handle to registred event to clear/param>
	/// <returns>none</returns>
	DllExport void UnregisterPeriodicStatusRead(HANDLE hPeriodicRead);

	DllExport void RegisterOnDisconnected(DisconnectedCallback_t callback, PVOID arg);


	/// <summary>
	/// set a callback for debug. The callback will receive a message realted to the function called, arg and results
	/// </summary>
	/// <param name="callback">DebugCallback_t typed callbnack function in your code</param>
	/// <param name="Arg">your argument if needed. Can be NULL</param>
	/// <returns></returns>
	DllExport void RegisterDebug(DebugCallback_t callback, PVOID Arg = NULL) {
		extern DebugCallback_t DebugHandler;
		DebugHandler = (DebugCallback_t)callback;
		//m_DebugArg = Arg;
	}

	/// <summary>
	/// Attach a callback on event callback to monitor the connection state changed
	/// ATTENTION, the callback will be call only if you try to communicate with the ICNC
	/// Then, if the controller is disconnected, you will get a callbac only when next communication try.
	/// </summary>
	/// <param name="callback"></param>
	/// <returns></returns>
	DllExport void HookOnConnectChanged(OnConnectChangedEvent_t callback, PVOID arg) {
		m_OnConnectChangedEventHandle = (OnConnectChangedEvent_t)callback;
		m_OnConnectChangedEventArg = arg;
		if (m_AutoConnect)
			OnConnectChanged(m_connected, arg); // For initial state callback
	}


	// Event called when connected success or when disconnected from call to Disconnect or commu_nicaion error
	DllExport virtual void OnConnectChanged(bool connected, PVOID arg);

	SysInfo_s m_SysInfo;



private:
	void Disconnected(void);
	bool m_connected = FALSE;

	HANDLE m_hTimerQueue = NULL;  // Internal use for registered status read callback
	PVOID m_RegisteredCallbackList = NULL; // Internal use for registered status read callback

	DisconnectedCallback_t m_DisconnectedCallback_handle = NULL;	// Handle called if communication error
	PVOID m_DisconnectedCallback_arg = NULL;	// User arg for Disconnected handle
	//PVOID m_DebugArg = NULL;		// User handle argument for debug handle

	bool m_AutoConnect = false;
	OnConnectChangedEvent_t m_OnConnectChangedEventHandle = NULL;
	// argument passed to the OnConnectedChangedEvent handle.
	// it can be set from constructor argument 
	PVOID m_OnConnectChangedEventArg = NULL;

};


/// Create and allocate a handle to cICNC2 class
DllExport cICNC2* create_ICNC2(void);


#pragma warning( pop )


#endif