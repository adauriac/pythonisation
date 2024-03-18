#pragma once

//#include <wtypes.h>

#include "GlobalDef.h"
#include "Constant.h"


#pragma warning( push )
#pragma warning( disable : 4635 4638 )


#define	ICNCUSB_FAIL                  0
#define ICNCUSB_SUCCESS               1
#define ICNCUSB_COM_ERROR             3
#define ICNCUSB_TIMEOUT               4
#define ICNCUSB_PROBE_ERROR           5

/*********************************************************************
 *
 *                  SOPROLEC InterpCNC V2.0 Library
 *
 *
 ********************************************************************/
#ifdef ICNC2_VS_EXPORTS
#define DllExport    __declspec( dllexport )
#else
#define DllExport   __declspec( dllimport )
#endif

 // constantes used for simulation mode
#define SIMUL_USER_MEM             10
#define SIMUL_BUFFER_SIZE        5000
#define SIMUL_PARAMETERS          200
#define SIMUL_DAC_RESOLUTION	 1024
#define SIMUL_ADC_RESOLUTION     1024
#define SIMUL_AVAILABLE_DOUT       24   // Number of Digital output
#define SIMUL_AVAILABLE_DIN        16   // Number of Digital input

// Used for debug purpose. This is the typedef of callback call by communication functions
// The callback is call when DebugHandler is set (on valid CONSOLE_HANDLER callback function pointer
typedef void(*CONSOLE_HANDLER)(PTCHAR buf, PVOID Arg);
extern CONSOLE_HANDLER DebugHandler;

// Internal data used for simulation mode (when ICNC is not connected and ICNC_SetSimulationMode is true)
typedef struct {

	int PosX, PosY, PosZ, PosA, PosB;
	int Input, Output;
	DWORD StatusBoard;
	DWORD UserMem[SIMUL_USER_MEM];
	INT Parameters[SIMUL_PARAMETERS];
	INT LastProbe;

} SSimulation;
extern SSimulation Simulation;




/// <summary>
/// Get the DLL version
/// </summary>
/// <returns>DLL Version</returns>	
extern "C" {
	DllExport DWORD ICNC_GetDLLVersion(void);

	/// <summary>
	/// Set the DLL in simulation mode.
	/// This mode allow to make software developpement without real connexion
	/// It's not real time simulation 
	/// </summary>
	/// <param name="SimulationModeVal">Set to thrue to enter in simulation mode</param>
	/// <returns>always ICNCUSB_SUCCESS</returns>	
	DllExport DWORD ICNC_SetSimulationMode(BOOL  SimulationModeVal);

	DWORD ICNC_SetUSBSpeedMode(BOOL EnableHighSpeed);
	DWORD  ICNC_SetVerboseMode(DWORD Verbose);
	/// <summary>
	/// Establish the link with connected InterpCNC
	/// </summary>
	/// <param name="ICNC_Number">For futur use.</param>
	/// <returns>ICNCUSB_SUCCESS if success</returns>	

	DllExport DWORD ICNC_Connect(DWORD ICNC_Number = 0);


	/// <summary>
	/// Disconnect from InterpCNC. The USB link is release for other application.
	/// </summary>
	/// <returns>always ICNCUSB_SUCCESS</returns>	
	DllExport DWORD ICNC_Disconnect(void);


	/// <summary>
	/// Unlock the board and/or reset some error flags
	/// Unlock is necessary after Emergency stop or limit switch
	/// </summary>
	/// <param name="ErrorBits">Bits field combination of 
	///     ICNC_STATUS_BIT_EMERGENCYSTOP
	///     ICNC_STATUS_BIT_LOCKED
	///     ICNC_STATUS_BIT_HOMING_ERROR
	///     ICNC_STATUS_BIT_PROBING_ERROR
	///     ICNC_STATUS_BIT_WAIT_INPUT_ERROR
	///     ICNC_STATUS_BIT_EEWRITE_ERROR.
	/// </param>
	/// <returns>ICNCUSB_SUCCESS if success. Success mean no communication error</returns>	
	DllExport DWORD ICNC_ErrorReset(DWORD ErrorBits);




/*
* ************************************************************************************************
* Not bufferized commands
* ************************************************************************************************
*/


/// <summary>
/// Read one or several status register
/// The content and size of returned data is depending of the StatusType parameter
/// You have to provide a pointer to an Array of DWORD with enougth space for return values required.
/// The size of Status array (from 1 to 32) is equal to the number of bits set in StatusType
/// </summary>
/// <param name="StatusType">Bits field combination of required status register.
///   ICNC_STATUS_BOARD_STATUS       		0x00000001  // get internal board status bits (bits details define with ICNC_STATUS_BIT_xxx constants)
///   ICNC_STATUS_BUFFER_FREE        		0x00000002	// get actual command buffer free space for CNC application
///   ICNC_STATUS_ACTUALX            		0x00000004	// get actual pos X
///   ICNC_STATUS_ACTUALY            		0x00000008	// get actual pos Y
///   ICNC_STATUS_ACTUALZ            		0x00000010	// get actual pos Z
///   ICNC_STATUS_ACTUALA            		0x00000020	// get actual pos A
///   ICNC_STATUS_ACTUALB            		0x00000040	// get actual pos B
///   ICNC_STATUS_INPUT              		0x00000080	// get inputs state
///   ICNC_STATUS_LAST_PROBE         		0x00000100  // Get last probe position result
///   ICNC_STATUS_ANALOG_IN1			  	0x00000200	// Get analog 1 input value (based on ADC range)
///   ICNC_STATUS_ANALOG_IN2			  	0x00000400	// Get analog 2 input value (based on ADC range)
///   ICNC_STATUS_ANALOG_IN3			  	0x00000800	// Get analog 3 input value (based on ADC range)
///   ICNC_STATUS_ANALOG_IN4			  	0x00001000	// Get analog 4 input value (based on ADC range)
///   ICNC_STATUS_THC_TARGET				0x00002000	// Get actual THC target
///   ICNC_STATUS_MATRIX_KEY				0x00004000	// Get actual matrix key state
///   ICNC_STATUS_ENCODER    				0x00008000	// Get actual encoder counter
///   ICNC_STATUS_A_PULSES_COUNTER		0x00010000	// Get actual input A pulses counter
///   ICNC_STATUS_B_PULSES_COUNTER		0x00020000	// Get actual input B pulses counter
///   ICNC_STATUS_A_PULSES_FREQUENCY		0x00040000	// Get actual input A pulses frquency (unit is 1/100Hz)
///   ICNC_STATUS_B_PULSES_FREQUENCY		0x00080000	// Get actual input B pulses counter (unit is 1/100Hz)
///   ICNC_STATUS_ANALOG_AOUT1			0x00100000	// Get actual analog output 1 state
///   ICNC_STATUS_ANALOG_AOUT2      		0x00200000	// Get actual analog output 2 state
///   ICNC_STATUS_BOARD_STATUS2	        0x00400000	// Get extended board status (Status2. Bits details are define in ICNC_STATUS2_BIT_xxxxxxxxxx)
/// </param>
/// <param name="Status">Pointer on DWORD[] where read result will be send
///     Inside this array, the order depend of the weight of related StatusType bit.
///     If <c>StatusType = ICNC_STATUS_BOARD_STATUS | ICNC_STATUS_ACTUALX | ICNC_STATUS_ANALOG_IN1 </c>,
///     Status[0] will be the actual board status
///     Status[1] will be the actual X motor position
///     Status[2] will be the analog 1 input value
/// </param>
/// <returns>ICNCUSB_SUCCESS if success. Success mean no communication error</returns>	
/// <exemple>Exemple of GetBoardStatus use
/// <code>
/// int main()
/// {
///    DWORD Results[3];
///	   if (ICNC_Connect(0)!=ICNCUSB_SUCCESS)
///			return -1;
///    if (ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS | ICNC_STATUS_ACTUALX | ICNC_STATUS_ANALOG_IN1, Results) != ICNCUSB_SUCCESS)
///			return -1;
///    DWORD Status = Results[0];
///    if ( (Status & ICNC_STATUS_BIT_EMERGENCYSTOP) != 0)
///			printf("Emergency stop activated");
///    if ( (Status & ICNC_STATUS_BIT_HOMING) != 0)
///			printf("Homing in progress");
///    ICNC_Disconnect();
///    return 0;
/// }
/// </code>
/// </exemple>
	DllExport DWORD ICNC_GetBoardStatus(DWORD StatusType, DWORD* Status);


	///  <summary>
	/// Set DOUT1 to DOUT31 state all together
	/// </summary>
	/// <param name="OutputState">New DOUT output states for µDOUT1 to DOUT32</param>
	/// <returns>ICNCUSB_SUCCESS if success. Success mean no communication error</returns>	
	DllExport DWORD ICNC_SetOutputAll(DWORD OutputState);


	///  <summary>
	/// Reserve for future use. No action
	/// </summary>
	DllExport DWORD ICNC_SetOverrideValue(DWORD OverrideValue); // Input

	///  <summary>
	/// Reserve for future use. No action
	/// </summary>
	DllExport DWORD ICNC_GetOverrideValue(DWORD* OverrideValue); // Output


	/// 
	/// <summary>
	/// Move one axis up to event linked to an input state changed.
	/// When event occurs, the axis will be stoped with Decel parameter.
	/// The position when event occurs is save and then, event if time is needed to stop because of deceleration,
	/// you will be able to read the exact switch position (from ICNC_STATUS_LAST_PROBE status register).
	/// </summary>
	/// <param name="Axis">The axis ID to move from ICNC_AXE_X to ICNC_AXE_Y</param>
	/// <param name="Direction">Can be 1 for probe in positive direction, -1 for negative direction</param>
	/// <param name="InNo">Input number used for probe event (1..32)</param>
	/// <param name="InValue">1 for rising edge event, 0 for falling edge event</param>
	/// <param name="MaxStroke">
	/// Maximum stroke on axis (from initial position) before to stop even if event didn't occurs.
	/// In this case, the ICNC_STATUS_BIT_PROBING will be reset but the bit ICNC_STATUS_BIT_PROBING_ERROR will be set to signal abnormal completion</param>
	/// </param>
	/// <param name="Speed">nominal speed of the axis (steps/s)</param>
	/// <param name="FStart">Initial speed of speed profile (steps/s)</param>
	/// <param name="Accel">Acceleration (K steps/s)</param>
	/// <param name="Accel">Deceleration (K steps/s). Should be high in case of rigid sensor</param>
	/// <returns>ICNCUSB_SUCCESS if success. Success mean no communication error</returns>	
	/// <remarks>
	/// As soon as the Probe command is sent, the ICNC_STATUS_BIT_PROBING from ICNC_STATUS_BOARD_STATUS register will be set.
	/// When Probe sequence is completed, this bit will be clear.
	/// You have then to test the ICNC_STATUS_BIT_PROBING_ERROR bit to know if probe occurs successully.
	/// ICNC_STATUS_BIT_PROBING_ERROR will be set if :
	///		Call parameters are wrong,
	///		Event doesn't appen within the limited stroke,
	///		The input is in Expected input state when cammand is launched (switch already active).
	/// In case of error (ICNC_STATUS_BIT_PROBING_ERROR is set), you should not consider ICNC_STATUS_LAST_PROBE status register as valid value.
	/// The ICNC_STATUS_BIT_PROBING will be clear at the end of axis move (including deceleration).
	/// </remarks>
	/// <exemple>Probe exemple
	/// <code>
	/// int main()
	/// {
	///		DWORD Result;
	///		if (ICNC_Connect(0)!=ICNCUSB_SUCCESS)
	///			return -1;
	///		if (ICNC_Probe(ICNC_AXE_Z,
	///						1,			// Positive direction 
	///						5,			// Input DIN5 number
	///						0,			// Expected input value (normaly close contact)
	///						100000,		// 100000 steps maximum stroke
	///						2000,		// Velocity 2000 steps/s
	///						500,		// Start velocity 500 steps/s
	///						25,			// 25000 steps/s² for acceleration
	///						250)		// 250000 steps/s² for deceleration
	///			!= ICNCUSB_SUCCESS)
	///				return -1;
	///		
	///		// wait for Probe completion
	///		do {
	///			ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Result);
	///			Sleep(10);	// Leave time for other process
	///		while ( (Result&ICNC_STATUS_BIT_PROBING) != 0);	
	///		
	///		// Check for Error status bit	
	///		if ( (Result&ICNC_STATUS_BIT_PROBING_ERROR) != 0) {
	///			printf ("Probe Error");
	///			return -1;
	///		}
	/// 
	///		// No error then, read Probe result
	///		ICNC_GetBoardStatus(ICNC_STATUS_LAST_PROBE, &Result);
	///		printf("Probe position = %d", (int)Result);
	///		ICNC_Disconnect();
	///		return 0;
	/// }
	/// </code>
	/// </exemple>
	DllExport DWORD ICNC_Probe(DWORD Axis, DWORD Direction, DWORD InNo, BOOL InValue, DWORD MaxStroke, DWORD Speed, DWORD FStart, DWORD Accel, DWORD Decel);


	/// <summary>
	/// This function is using the ICNC_Probe function but including the wait of end of probe sequence.
	/// Then, your proccess will be locked while probing is not completed.
	/// </summary>
	/// <param name="Axis">The axis ID to move from ICNC_AXE_X to ICNC_AXE_Y</param>
	/// <param name="Direction">Can be 1 for probe in positive direction, -1 for negative direction</param>
	/// <param name="InNo">Input number used for probe event (1..32)</param>
	/// <param name="InValue">1 for rising edge event, 0 for falling edge event</param>
	/// <param name="MaxStroke">
	/// Maximum stroke on axis (from initial position) before to stop even if event didn't occurs.
	/// In this case, the ICNC_STATUS_BIT_PROBING will be reset but the bit ICNC_STATUS_BIT_PROBING_ERROR will be set to signal abnormal completion</param>
	/// </param>
	/// <param name="Speed">nominal speed of the axis (steps/s)</param>
	/// <param name="FStart">Initial speed of speed profile (steps/s)</param>
	/// <param name="Accel">Acceleration (K steps/s)</param>
	/// <param name="Accel">Deceleration (K steps/s). Should be high in case of rigid sensor</param>
	/// <param name="ProbePosition">Pointer where Probe position will be return</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_PROBE_ERROR if probe error, ICNCUSB_FAIL in case of communication error</returns>	
	DllExport DWORD ICNC_ProbeAndReadBack(DWORD Axis, DWORD Direction, DWORD InNo, BOOL InValue, DWORD MaxStroke, DWORD Speed, DWORD FStart, DWORD Accel, DWORD Decel, INT* ProbePosition);


	/// <summary>
	/// Stop one or several axes mouvement with deceleration.
	/// This command should be use in relation with Asynchronous movement and not for interpolated mouvement.
	/// </summary>
	/// <param name="Axes">Combination of ICNC_AXE_x bit for axes selection</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>	
	DllExport DWORD ICNC_SlowStopMotors(DWORD Axes);


	/// <summary>
	/// Stop all interpolated mouvement (CNC use case) or asynchrone mouvement and flush the command buffer.
	/// Stop will be done with deceleration
	/// </summary>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>	
	DllExport DWORD ICNC_SlowStopAllAndClear(void);

	/// <summary>
	/// Stop all interpolated mouvement (CNC use case) or asynchrone mouvement and flush the command buffer.
	/// Stop will be done without deceleration
	/// </summary>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>	
	DllExport DWORD ICNC_StopMotorsAllAndClear(void);


	/// <summary>
	/// Read the actual output state (DOUT1 to DOUT32)
	/// </summary>
	/// <param name="OutputState">Pointer for return result</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>	
	/// <summary>
	/// Read one of digital input (DIN1 to DIN32)
	/// </summary>
	/// <param name="InNo">Input number to read (1..32)</param>
	/// <param name="InputState">Pointer for return result. Value is 0 if the input is inactive and 1 the input is active</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_GetOutputAll(DWORD* OutputState); // Output

	DllExport DWORD ICNC_GetInput(DWORD InNo, // Input
		BOOL* InputState);

	/// <summary>
	/// Read all digital input together (DIN1 to DIN32)
	/// </summary>
	/// <param name="InputState">Pointer for return result.</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_GetInputAll(DWORD* InputState); // Output


	/// <summary>
	/// Read one analog input (AIN1 to AIN4)
	/// </summary>
	/// <param name="AnalogNo">Analog input number to read (1..4)</param>
	/// <param name="AnalogValue">
	/// Pointer for return result. Value is in ADC points and then, real value is depending of ADC resolution
	/// The resolution can be read from system information ICNC_SYS_INFO_ADC_RESOLUTION with ICNC_GetSysInfo function
	/// </param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_GetAnalog(DWORD AnalogNo, // Input
		DWORD* AnalogValue);


	/// <summary>
	/// Set or Reset one digital output (DOUT1..DOUT32)
	/// </summary>
	/// <param name="OutputNo">Analog input number to read (1..32)</param>
	/// <param name="OutputState">New output state</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_SetOutput(DWORD OutputNo,
		BOOL OutputState);



	/// 
	/// <summary>
	/// Set one analog output value (AOUT1..AOUT4)
	/// </summary>
	/// <param name="AnalogNo">Analog output number to write (1..4)</param>
	/// <param name="AnalogValue">
	/// New analog output value
	/// The resolution can be read from system information ICNC_SYS_INFO_DAC_RESOLUTION with ICNC_GetSysInfo function
	/// </param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_SetAnalog(DWORD AnalogNo,
		DWORD AnalogValue);


	/// 
	/// <summary>
	/// Read free space in buffer command for CNC application
	/// This function is optimized version of ICNC_GetBoardStatus with ICNC_STATUS_BOARD_STATUS request
	/// The buffer size can be read from system information ICNC_SYS_INFO_BUFFER_SIZE with ICNC_GetSysInfo function
	/// </summary>
	/// <param name="FreeBufferSpace">Pointer for free space return value</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_GetBufferStatus(DWORD* FreeBufferSpace);


	/// 
	/// <summary>
	/// Unfreeze the InterpCNC buffer use.
	/// This fonction can be use in relation with ICNC_FreezeBuf function.
	/// </summary>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	/// <seealso cref="ICNC_FreezeBuf"/>
	DllExport DWORD ICNC_Unfreeze(void);


	/// <summary>
	/// Write in User memory registers
	/// The number of available user memory register can be obtain by reading the ICNC_SYS_INFO_USR_MEM_SIZE register.
	/// Each one can store a 32bits value and are located in RAM. Then, values are lost when power of.
	/// Two type of write to user memory are available. One is immediate, one is bufferized.
	/// The bufferized one can help you to follow the execution of bufferized command.
	/// For this purpose, you can make a bufferized write with a counter for some particular points of your process.
	/// Reading this user mem will inform you about the process excecution.
	/// </summary>
	/// <param name="MemoryNumber">User memory register number (1..16)</param>
	/// <param name="MemoryValue">Value to write</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_WriteUserMem(DWORD MemoryNumber,
		DWORD MemoryValue);

	/// <summary>
	/// Read a user memory
	/// </summary>
	/// <param name="MemoryNumber">User memory register number (1..16)</param>
	/// <param name="MemoryValue">pointer for return value</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_ReadUserMem(DWORD MemoryNumber,
		DWORD* MemoryValue);


	/// <summary>
	/// Read EEProm memory.
	/// SOPROLEC internal use
	/// </summary>
	DllExport DWORD ICNC_ReadEEPROM(DWORD StartAdress,
		DWORD Length,
		BYTE* Value);

	/// <summary>
	/// Write EEProm memory.
	/// SOPROLEC internal use
	/// Using this function can make the InterpCNC unusable
	/// </summary>
	DllExport DWORD ICNC_WriteEEPROM(DWORD StartAdress,
		DWORD Length,
		BYTE* Value,
		BOOL WaitCompletion = TRUE,
		DWORD TimeOut = 1000);

	/// <summary>
	/// Read one of InterpCNC parameter based on parameter's ID
	/// Parameters ID are define in ParametersID.h
	/// </summary>
	/// <param name="ParameterID">Parameter's ID to read</param>
	/// <param name="ParameterValue">pointer for return value</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_ReadParameter(DWORD ParameterID,
		INT* ParameterValue);

	/// <summary>
	/// Write one of InterpCNC parameter based on parameter's ID
	/// Parameters ID are define in ParametersID.h
	/// </summary>
	/// <param name="ParameterID">Parameter's ID to write</param>
	/// <param name="ParameterValue">pointer for return value</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_WriteParameter(DWORD ParameterID,
		DWORD ParameterValue,
		BOOL WaitCompletion = TRUE,
		DWORD TimeOut = 1000);


	/// <summary>
	/// Write internal position counter for one or several axes
	/// !!!!! Never write those counters when axis is moving !!!!
	/// </summary>
	/// <param name="Axes">Axis combination of counter to write ICNC_AXE_X to ICNC_AXE_B</param>
	/// <param name="PositionX">Value to write on X axis counter if ICNC_AXE_X bit is set in Axes. Ignored if not</param>
	/// <param name="PositionY">Value to write on Y axis counter if ICNC_AXE_Y bit is set in Axes. Ignored if not</param>
	/// <param name="PositionZ">Value to write on Z axis counter if ICNC_AXE_Z bit is set in Axes. Ignored if not</param>
	/// <param name="PositionA">Value to write on A axis counter if ICNC_AXE_A bit is set in Axes. Ignored if not</param>
	/// <param name="PositionB">Value to write on B axis counter if ICNC_AXE_B bit is set in Axes. Ignored if not</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_WritePosition(DWORD Axes,
		INT PositionX = 0,
		INT PositionY = 0,
		INT PositionZ = 0,
		INT PositionA = 0,
		INT PositionB = 0);

	/// <summary>
	/// Start Homing sequence on one or several axes.
	/// Homing parameters are define in InterpCNC parameters (see EE_ORIGINE_xxxxx for details)
	/// When homing start, ICNC_STATUS_BIT_HOMING of ICNC_STATUS_BOARD_STATUS register is set.
	/// When homing is completed, this bit is clear and you can test the ICNC_STATUS_BIT_HOMING_ERROR bit
	/// to control the result.
	/// Homing will fail if homing sensor of one axis is not reach while moving up to maximum stroke define.
	/// If A or B axis are define as X slave, homing sequence of this slave will be launched automaticaly if ICNC_AXE_X is set
	/// </summary>
	/// <param name="Axes">Axis combination of counter to write ICNC_AXE_X to ICNC_AXE_B</param>
	/// <param name="MaxStrokeX">MaxStroke for X  if ICNC_AXE_X bit is set in Axes. Ignored if not</param>
	/// <param name="MaxStrokeY">MaxStroke for Y  if ICNC_AXE_Y bit is set in Axes. Ignored if not</param>
	/// <param name="MaxStrokeZ">MaxStroke for Z  if ICNC_AXE_Z bit is set in Axes. Ignored if not</param>
	/// <param name="MaxStrokeA">MaxStroke for A  if ICNC_AXE_A bit is set in Axes. Ignored if not</param>
	/// <param name="MaxStrokeB">MaxStroke for B  if ICNC_AXE_B bit is set in Axes. Ignored if not</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	/// <exemple>Homing sequence
	/// <code>
	/// int main()
	/// {
	///		DWORD Result;
	///		if (ICNC_Connect(0)!=ICNCUSB_SUCCESS)
	///			return -1;
	///		if (ICNC_Home(ICNC_AXE_Z,
	///						0,			// ignored
	///						0,			// ignored
	///						50000,		// Max stroke before homing switch
	///						0,			// ignored
	///						0);			// ignored
	///			!= ICNCUSB_SUCCESS)
	///				return -1;
	///		
	///		// wait for Z homing completion
	///		do {
	///			ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Result);
	///			Sleep(10);	// Leave time for other process
	///		while ( (Result&ICNC_STATUS_BIT_HOMING) != 0);	
	///		
	///		// Check for Error status bit	
	///		if ( (Result&ICNC_STATUS_BIT_HOMING_ERROR) != 0) {
	///			printf ("Z homing error");
	///			return -1;
	///		}
	/// 
	///		if (ICNC_Home(ICNC_AXE_X | ICNC_AXE_Y,
	///						100000,			// Max stroke X before homing switch
	///						100000,			// Max stroke Y before homing switch
	///						0,			// ignored
	///						0,			// ignored
	///						0);			// ignored
	///			!= ICNCUSB_SUCCESS)
	///				return -1;
	///		
	///		// wait for X en Y homing completion
	///		do {
	///			ICNC_GetBoardStatus(ICNC_STATUS_BOARD_STATUS, &Result);
	///			Sleep(10);	// Leave time for other process
	///		while ( (Result&ICNC_STATUS_BIT_HOMING) != 0);	
	///		
	///		// Check for Error status bit	
	///		if ( (Result&ICNC_STATUS_BIT_HOMING_ERROR) != 0) {
	///			printf ("X or Y homing error");
	///			return -1;
	///		}
	///		printf ("Home completed");
	/// 
	///		ICNC_Disconnect();
	///		return 0;
	/// }
	/// </code>
	/// </exemple>
	DllExport DWORD ICNC_MachineHome(DWORD Axes,
		DWORD MaxStrokeX = 0xFFFFFFFF,
		DWORD MaxStrokeY = 0xFFFFFFFF,
		DWORD MaxStrokeZ = 0xFFFFFFFF,
		DWORD MaxStrokeA = 0xFFFFFFFF,
		DWORD MaxStrokeB = 0xFFFFFFFF);

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
	DllExport DWORD ICNC_AxeHome(DWORD AxeID,
		DWORD InputNumber,
		DWORD ExpectedInputValue,
		DWORD Acceleration_khz_per_s,
		DWORD HighSpeed_Hz,
		DWORD Deceleration_khz_per_s,
		INT   MaxStroke_step,
		DWORD LowSpeed_Hz,
		INT   InitialPosition_step = 0);



	/// <summary>
	/// Set the target plasma voltage in case of using the plasma Torch Hight Control
	/// </summary>
	/// <param name="THCTargetValue">Plasma arc voltage target in ADC points</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_SetTHCTarget(DWORD THCTargetValue);


	/// <summary>
	/// read some technical informations from the controller.
	/// </summary>
	/// <param name="SysInfoType">CNC_SYS_INFO_xxxxx Combination bit of information to read</param>
	/// <param name="SysInfo">
	/// Pointer on array's of results. You should provide a pointer to an array with enougth space
	/// depending of the number of bits seted in SysInfoType parameter
	/// </param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	/// 
	DllExport DWORD ICNC_GetSysInfo(DWORD SysInfoType,
		DWORD* SysInfo);

	/// <summary>
	/// Reserved for SOPROLEC internal used
	/// </summary>
	DllExport DWORD ICNC_GetSoftwareKeyValue(DWORD* SoftwareKeyValue);



	/// <summary>
	/// This function will stop all axes (without rampe), flush all buffer and no more axis mouvement command will be accepted
	/// To exit from this state, you have to use the ICNC_ErrorReset command with ICNC_STATUS_BIT_LOCKED bit set.
	/// </summary>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_LockBoard(void);




	/// <summary>
	/// Start independant move of one axis with absolute target position.
	/// If the axis is already moving, the profile will be update on the fly with new parameters.
	/// Each axis use it's own profile generator and then, can be move indepentently from otehr one.
	/// Mouvement status can be read in ICNC_STATUS_BIT_x_ASYNC_MOVING from ICNC_STATUS_BOARD_STATUS
	/// </summary>
	/// <param name="Axis">Axis ID (ICNC_AXE_X to ICNC_AXE_B)</param>
	/// <param name="FStartStop">Initial speed (steps/s)</param>
	/// <param name="Accel">speed profile acceleration (steps/s²)</param>
	/// <param name="Speed">Maximum speed (steps/s)</param>
	/// <param name="Decel">speed profile deceleration (steps/s²)</param>
	/// <param name="Position">Absolute target position (in steps)</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_MoveProfileAbsAsync(DWORD Axis,
		DWORD FStartStop,
		DWORD Accel,
		DWORD Speed,
		DWORD Decel,
		INT   Position);


	/// <summary>
	/// Start independant move of one axis with relative target position.
	/// start point will be the actual position. Event if the axis already move.
	/// Then, Target will be Actual position + Distance
	/// </summary>
	/// <param name="Axis">Axis ID (ICNC_AXE_X to ICNC_AXE_B)</param>
	/// <param name="FStartStop">Initial speed (steps/s)</param>
	/// <param name="Accel">speed profile acceleration (steps/s²)</param>
	/// <param name="Speed">Maximum speed (steps/s)</param>
	/// <param name="Decel">speed profile deceleration (steps/s²)</param>
	/// <param name="Position">Distance to move from actual position (in steps)</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_MoveProfileRelAsync(DWORD Axe,
		DWORD FStartStop,
		DWORD Accel,
		DWORD Speed,
		DWORD Decel,
		INT   Distance);

	/// <summary>
	/// Reserved for futur used
	/// </summary>
	DllExport DWORD ICNC_RampUpAbsBuf(DWORD Axis,
		INT Position[5],
		DWORD FStart[5],
		DWORD FEnd[5],
		DWORD FInc[5]);


	/// <summary>
	/// Reserved for futur used
	/// </summary>
	DllExport DWORD ICNC_RampDownAbsBuf(DWORD Axis,
		INT Position[5],
		DWORD FStart[5],
		DWORD FEnd[5],
		DWORD FInc[5]);





	// Activation/desactivation logiciel du THC
	// La commande peut être bufferisée ou directe
	// Note : Si le logciel doit prendre en charge l'activationdésactivation du THC, mettre
	// le parammètre EE_THC_ACTIVATION_OUTPUT à 0 pour éviter les conflicts
	DllExport DWORD ICNC_SetTHCStateBuf(BOOL EnableTHC,
		BOOL BufferizedCommand);



	// Commande bufferisées
	// ************************************************

	/// <summary>
	/// Freeze InterpCNC buffer internal use treatement until buffer unfreezed or buffer full
	/// This command can be useful to fill the buffer before real mouvement for CNC application
	/// </summary>
	DllExport DWORD ICNC_FreezeBuf(void);

	/// <summary>
	/// Reserved for futur used
	/// </summary>
	DllExport DWORD ICNC_SetOverrideStateBuf(BOOL OverrideState); // Input


	DllExport DWORD ICNC_SetOutputBuf(DWORD OutputNo,
		BOOL OutputValue);

	DllExport DWORD ICNC_SetOutputAllBuf(DWORD OutputValue);

	DllExport DWORD ICNC_SetAnalogBuf(DWORD OutputNo,
		DWORD OutputValue);

	DllExport DWORD ICNC_WaitBuf(DWORD TimeToWait);

	DllExport DWORD ICNC_WaitInputStateBuf(DWORD InputNo,
		BOOL InputValue,
		DWORD TimeOut,
		BOOL LockIfError = FALSE,
		BOOL ClearBufferIfError = FALSE);

	DllExport DWORD ICNC_WriteUserMemBuf(DWORD MemNumber,
		DWORD Value);


	DllExport DWORD ICNC_MoveSpeedRelBuf(DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired = NULL);

	DllExport DWORD ICNC_MoveSpeedAbsBuf(DWORD Axis,
		DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired = NULL);


	/// <summary>
	/// Relative movement without speed profile (constant speed)
	/// Use local buffer for full frame transmission
	/// !!! Don't forget to use ICNC_FlushLocalBuf at the end
	/// </summary>
	DllExport DWORD ICNC_MoveSpeedRelLocalBuf(DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired = NULL);


	/// <summary>
	/// Similar to ICNC_MoveSpeedAbsBuf but using a buffer on your PC for improvement of transmit flow on USB bus
	/// When sending a command Send Absolute movement without speed profile
	/// Use local buffer for full frame transmission
	/// !!! Don't forget to use ICNC_FlushLocalBuf at the end
	/// <!summary>
	DllExport DWORD ICNC_MoveSpeedAbsLocalBuf(DWORD Axis,
		DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired = NULL);


	/// <summary>
	/// Send to the controller remaining data located in local PC buffer
	/// Remaining data can come from ICNC_MoveSpeedRelLocalBuf or ICNC_MoveSpeedRelLocalBuf for communication optimisation purpose
	/// Anyway, all commands other than the two listed above will flush existing local data
	/// </summary>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_FlushLocalBuf(void);


	DllExport DWORD ICNC_MoveProfileRelBuf(DWORD Speed,
		INT MoveX,
		INT MoveY,
		INT MoveZ,
		INT MoveA,
		INT MoveB,
		DWORD* BufferRequired = NULL);

	DllExport DWORD ICNC_MoveProfileAbsBuf(DWORD Axis,
		DWORD Speed,
		INT PositionX,
		INT PositionY,
		INT PositionZ,
		INT PositionA,
		INT PositionB,
		DWORD* BufferRequired = NULL);


	DllExport DWORD ICNC_PushPulsesData(unsigned char* ptBuffer,
		int nbBytes,
		DWORD Frequency,
		DWORD* BufferRequired = NULL);

	/// <summary>
	/// reserved for SOPROLEC use
	/// </summary>
	DllExport DWORD ICNC_SwitchToBootLoaderMode(DWORD BootKey);




	/// <summary>
	/// Read or write some internal system registers
	/// </summary>
	/// <param name="SubCommandID">
	///    ICNC_CMD_CUSTOM_SETSYS
	/// or ICNC_CMD_CUSTOM_GETSYS
	/// or ICNC_CMD_CUSTOM_GETSYS_MULTI
	/// </param>
	/// <param name="NumRegistre">
	/// 1..5 : Internal profile generator  status for axes 1 to 5
	/// 11..15 : Actual velocity (signed value, steps/s)
	/// 40 : Inputs forced to 0
	/// 41 : Inputs forced to 1
	/// 200 : Modbus RX bytes count
	/// 201 : Modbus TX bytes count
	/// 202 : Last modbus command type received
	/// 203 : Count of modbus brodcast frame received
	/// 204 : Count of modbus received for this slave
	/// 300 : Modbus coils 0 to 31
	/// 301 : Modbus coils 32 to 63
	/// 302 : Modbus coils 64 to 95
	/// </param>
	/// <param name="NbRegister"></param>
	/// <param name="ptBuffer"></param>
	/// <returns></returns>
	DllExport DWORD ICNC_CustomCommand(int SubCommandID,
		int NumRegistre,
		int NbRegister,
		unsigned char* ptBuffer);



	/// <summary>
	/// 
	/// </summary>
	/// <param name="SubCommandID">
	///	   ICNC_CMD_MB_BITS_SET : write 96 existing coils
	/// or ICNC_CMD_MB_BITS_GET : Read 96 existing coils
	/// or ICNC_CMD_MB_ONE_BIT_SET : Write one on 96 user coils
	/// or ICNC_CMD_MB_SET_HOLDING_REG : Write one holding registe
	/// or ICNC_CMD_MB_GET_HOLDING_REG : read one holding registre
	/// </param>
	/// <param name="ptBuffer">Depending of SubCommandID</param>
	/// <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
	DllExport DWORD ICNC_MBControl(int SubCommandID,
		unsigned char* ptBuffer);



	/// <summary>
	/// Interaction with the PLC Basic function
	/// </summary>
	/// <param name="SubCommandID">
	///    ICNC_BASIC_CONTROL_GETTX 
	/// or ICNC_BASIC_CONTROL_SETRX</param>
	/// <param name="ptBuffer">buffer for send or receive data</param>
	/// <param name="SizeToSend">Size of data to send. !!!! Must be <= 60 characters</param>
	/// <param name="BufferFree">for return of available PLC buffer</param>
	/// <returns></returns>
	DllExport DWORD ICNC_BasicControl(int SubCommandID,
		char* ptBuffer,
		int SizeToSend,
		int* BufferFree);


	/// <summary>
	/// Wrapper function to send a PLC command by using the ICNC_BasicControl function
	/// </summary>
	/// <param name="Command">
	/// PLC Basic command. Should be ended by '\n' to be considered
	/// If the string is too long, it will be splitted for correct transmission
	/// </param>
	/// <returns></returns>
	DllExport DWORD ICNC_SendPLCCommand(char* Command);


	DllExport DWORD ICNC_ReadPLCMonitor(char* MonitorTxt);

}
#pragma warning( pop )
