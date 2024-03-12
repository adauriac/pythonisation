"""
        This module allows to use a ICNC2 library in python 64 bits.

I got the source in a zip file archive.zip sent initially to Thierry Szindingre.
I added the compiler directive extern "C" {} to the function of the dll,
and I compiled it in Release 64 bits.

A class ICNC2 is defined, which can call some of the functions of the dll

See https://hackmd.io/-2srd5HoSuCyLYslRxgEKg aka notesacxys.anglesdauriac.fr
"""
import sys,ctypes

class ICNC2 :
    EVENT_MANUALCONTROL  = 0
    EVENT_STARTPROCESS   = 1
    EVENT_RAPIDTOXY      = 2
    EVENT_TOOLDOWN       = 3
    EVENT_DRILLING       = 4
    EVENT_FEEDING        = 5
    EVENT_LIFTUP         = 6
    EVENT_PARKTOOL       = 7
    EVENT_ENDPROCESS     = 8
    EVENT_BREAKPROCESS   = 0xFFFF
    CONST_MAX_READ_EEPROM_SIZE = 62
    CONST_MAX_WRITE_EEPROM_SIZE = 57
    AXE_NONE = 0
    AXE_X = 0x01
    AXE_Y = 0x02
    AXE_YX = 0x03
    AXE_Z = 0x04
    AXE_ZX = 0x05
    AXE_ZY = 0x06
    AXE_ZYX = 0x07
    AXE_A = 0x08
    AXE_AX = 0x09
    AXE_AY = 0x0A
    AXE_AYX = 0x0B
    AXE_AZ = 0x0C
    AXE_AZX = 0x0D
    AXE_AZY = 0x0E
    AXE_AZYX = 0x0F
    AXE_B = 0x10
    AXE_BX = 0x11
    AXE_BY = 0x12
    AXE_BYX = 0x13
    AXE_BZ = 0x14
    AXE_BZX = 0x15
    AXE_BZY = 0x16
    AXE_BZYX = 0x17
    AXE_BA = 0x18
    AXE_BAX = 0x19
    AXE_BAY = 0x1A
    AXE_BAYX = 0x1B
    AXE_BAZ = 0x1C
    AXE_BAZX = 0x1D
    AXE_BAZY = 0x1E
    AXE_BAZYX = 0x1F
    ANALOG1 = 0x01
    ANALOG2 = 0x02
    ANALOG3 = 0x03
    ANALOG4 = 0x04
    CONST_OVERRIDE_PROG = 0
    CONST_OVERRIDE_ANA1 = 1
    CONST_OVERRIDE_ANA2 = 2
    CONST_OVERRIDE_ANA3 = 3
    CONST_OVERRIDE_ANA4 = 4
    STATUS_BOARD_STATUS = 0x00000001
    STATUS_BUFFER_FREE = 0x00000002
    STATUS_ACTUALX = 0x00000004
    STATUS_ACTUALY = 0x00000008
    STATUS_ACTUALZ = 0x00000010
    STATUS_ACTUALA = 0x00000020
    STATUS_ACTUALB = 0x00000040
    STATUS_INPUT = 0x00000080
    STATUS_LAST_PROBE = 0x00000100
    STATUS_ANALOG_IN1 = 0x00000200
    STATUS_ANALOG_IN2 = 0x00000400
    STATUS_ANALOG_IN3 = 0x00000800
    STATUS_ANALOG_IN4 = 0x00001000
    STATUS_THC_TARGET = 0x00002000
    STATUS_MATRIX_KEY = 0x00004000
    STATUS_ENCODER = 0x00008000
    STATUS_A_PULSES_COUNTER = 0x00010000
    STATUS_B_PULSES_COUNTER = 0x00020000
    STATUS_A_PULSES_FREQUENCY = 0x00040000
    STATUS_B_PULSES_FREQUENCY = 0x00080000
    STATUS_ANALOG_AOUT1 = 0  # vaut 0 ou 1 suivant origine fichier.h
    STATUS_ANALOG_AOUT2 = 0x00200000
    STATUS_BOARD_STATUS2 = 0x00400000
    STATUS_BIT_XMOVING = 0x00000001
    STATUS_BIT_YMOVING = 0x00000002
    STATUS_BIT_ZMOVING = 0x00000004
    STATUS_BIT_AMOVING = 0x00000008
    STATUS_BIT_BMOVING = 0x00000010
    STATUS_BIT_BUFFER_EMPTY = 0x00000020
    STATUS_BIT_BUFFER_FREZED = 0x00000040
    STATUS_BIT_EMERGENCYSTOP = 0x00000080
    STATUS_BIT_LOCKED = 0x00000100
    STATUS_BIT_STROKE_LIMIT = 0x00000200
    STATUS_BIT_HOMING = 0x00000400
    STATUS_BIT_HOMING_ERROR = 0x00000800
    STATUS_BIT_PROBING = 0x00001000
    STATUS_BIT_PROBING_ERROR = 0x00002000
    STATUS_BIT_EEWRITE_INPROGRESS = 0x00004000
    STATUS_BIT_EEWRITE_ERROR = 0x00008000
    STATUS_BIT_PROMPT_STATE = 0x00010000
    STATUS_BIT_OVERRIDE = 0x00020000
    STATUS_BIT_OVERRIDE_ALLOWED = 0x00040000
    STATUS_BIT_WAIT_INPUT = 0x00080000
    STATUS_BIT_WAIT_INPUT_ERROR = 0x00100000
    STATUS_BIT_THC_ACTIVATED = 0x00200000
    STATUS_BIT_X_ASYNC_MOVING = 0x00400000
    STATUS_BIT_Y_ASYNC_MOVING = 0x00800000
    STATUS_BIT_Z_ASYNC_MOVING = 0x01000000
    STATUS_BIT_A_ASYNC_MOVING = 0x02000000
    STATUS_BIT_B_ASYNC_MOVING = 0x04000000
    STATUS_BIT_X_LIMIT = 0x08000000
    STATUS_BIT_Y_LIMIT = 0x10000000
    STATUS_BIT_Z_LIMIT = 0x20000000
    STATUS_BIT_A_LIMIT = 0x40000000
    STATUS_BIT_B_LIMIT = 0x80000000
    STATUS2_BIT_XDIRECTION = 0x00000001
    STATUS2_BIT_YDIRECTION = 0x00000002
    STATUS2_BIT_ZDIRECTION = 0x00000004
    STATUS2_BIT_ADIRECTION = 0x00000008
    STATUS2_BIT_BDIRECTION = 0x00000010
    STATUS2_BIT_BASIC_TX_OVERRUN = 0x00000020
    STATUS2_BIT_BASIC_RX_OVERRUN = 0x00000040
    STATUS2_BIT_BASIC_RUNNING = 0x00000080
    STATUS2_BIT_B8 = 0x00000100
    STATUS2_BIT_B9 = 0x00000200
    STATUS2_BIT_B10 = 0x00000400
    STATUS2_BIT_B11 = 0x00000800
    STATUS2_BIT_B12 = 0x00001000
    STATUS2_BIT_B13 = 0x00002000
    STATUS2_BIT_B14 = 0x00004000
    STATUS2_BIT_B15 = 0x00008000
    STATUS2_BIT_MB_RECEIVED = 0x00010000
    STATUS2_BIT_MB_USR_RPM_CHANGED = 0x00020000
    STATUS2_BIT_B18 = 0x00040000
    STATUS2_BIT_B19 = 0x00080000
    STATUS2_BIT_B20 = 0x00100000
    STATUS2_BIT_B21 = 0x00200000
    STATUS2_BIT_B22 = 0x00400000
    STATUS2_BIT_B23 = 0x00800000
    STATUS2_BIT_B24 = 0x01000000
    STATUS2_BIT_B25 = 0x02000000
    STATUS2_BIT_B26 = 0x04000000
    STATUS2_BIT_B27 = 0x08000000
    STATUS2_BIT_B28 = 0x10000000
    STATUS2_BIT_B29 = 0x20000000
    STATUS2_BIT_B30 = 0x40000000
    STATUS2_BIT_B31 = 0x80000000
    SYS_INFO_BUFFER_SIZE = 0x00000001
    SYS_INFO_MAX_FREQUENCY = 0x00000002
    SYS_INFO_APP_VERSION_H = 0x00000004
    SYS_INFO_APP_VERSION_L = 0x00000008
    SYS_INFO_LOADER_VERSION_H = 0x00000010
    SYS_INFO_LOADER_VERSION_L = 0x00000020
    SYS_INFO_BOARD_VERSION = 0x00000040
    SYS_INFO_EEPROM_SIZE = 0x00000080
    SYS_INFO_USR_MEM_SIZE = 0x00000100
    SYS_INFO_AVAILABLE_AOUT = 0x00000200
    SYS_INFO_AVAILABLE_AIN = 0x00000400
    SYS_INFO_DAC_RESOLUTION = 0x00000800
    SYS_INFO_ADC_RESOLUTION = 0x00001000
    SYS_INFO_AVAILABLE_DOUT = 0x00002000
    SYS_INFO_AVAILABLE_DIN = 0x00004000
    CMD_MOVE_CONSTANT_REL_BUF = 1
    CMD_MOVE_CONSTANT_ABS_BUF = 25
    CMD_MOVE_RAMP_BUF = 2
    CMD_RAMPUP_ABS_BUF = 3
    CMD_RAMPDOWN_ABS_BUF = 4
    CMD_MOVE_ARC_XY_BUF = 97
    CMD_PROFILE_ABS_BUF = 6
    CMD_PROFILE_REL_BUF = 7
    CMD_FREEZE_BUF = 11
    CMD_SET_OUTPUT_BUF = 13
    CMD_SET_OUTPUT_ALL_BUF = 14
    CMD_SET_ANALOG_BUF = 15
    CMD_TEMPO_BUF = 20
    CMD_WAIT_INPUT_BUF = 21
    CMD_WRITE_USER_MEM_BUF = 22
    CMD_SET_OVERRIDE_STATE_BUF = 23
    CMD_MOVE_TO_SENSOR_BUF = 24
    CMD_SET_THC_ONOFF_BUF = 26
    CMD_PROBE = 40
    CMD_BREAKE_AXES = 41
    CMD_BREAKE_AXES_AND_CLEAR = 42
    CMD_STOP = 43
    CMD_GET_OUTPUT_ALL = 44
    CMD_GET_INPUT = 45
    CMD_GET_INPUT_ALL = 46
    CMD_GET_ANALOG = 47
    CMD_SET_OUTPUT_ALL = 48
    CMD_SET_OUTPUT = 49
    CMD_SET_ANALOG = 50
    CMD_GET_BUFFER_STATUS = 55
    CMD_UNFREEZE = 56
    CMD_WAIT_UNFREEZE = 57
    CMD_WRITE_USER_MEM = 58
    CMD_READ_USER_MEM = 59
    CMD_READ_EEPROM = 62
    CMD_WRITE_EEPROM = 63
    CMD_READ_PARAMETER = 64
    CMD_WRITE_PARAMETER = 65
    CMD_ERROR_RESET = 66
    CMD_WRITE_POSITION = 67
    CMD_GET_STATUS = 70
    CMD_MACHINE_HOME = 71
    CMD_SET_OVERRIDE = 72
    CMD_GET_OVERRIDE = 73
    CMD_SET_THC_TARGET = 74
    CMD_CONFIG_PROMPT = 75
    CMD_GET_SYS_INFO = 76
    CMD_GET_KEY = 77
    CMD_MOVE_PROFILE_ABS_ASYNC = 80
    CMD_MOVE_PROFILE_REL_ASYNC = 81
    CMD_CHANGE_SPEED = 96
    CMD_BOOT_MODE = 90
    CMD_GET_EXECTIME = 91
    CMD_LOCK_BOARD = 92
    PULSE_FRAME = 93
    USB_SPEED_SELECT = 94
    CMD_SET_WATCHDOG = 95
    BASIC_CONTROL = 98
    BASIC_CONTROL_GETTX = 0
    BASIC_CONTROL_SETRX = 1

    def __init__(self):
    	self.ICNC2Lib = ctypes.CDLL("./ICNC2_VS.dll")
        
    def Connect(self, Number=0):
        """
           Initialisation de la communication avec la carte InterpCNC
           """
        ans = self.ICNC2Lib.ICNC_Connect(Number)
        self.connected = ans
        return ans
       
    def ErrorReset(self, ErrorBits):
        """
        Deverouillage carte
        Acquittemement Arret d'urgence, Fin de course
        Input : Bits de defauts a remettre a 0
                    STATUS_BIT_EMERGENCYSTOP
                    STATUS_BIT_LOCKED
                    STATUS_BIT_HOMING_ERROR
                    STATUS_BIT_PROBING_ERROR
                    STATUS_BIT_WAIT_INPUT_ERROR
                    STATUS_BIT_EEWRITE_ERROR)
        Output :
            ICNCUSB_SUCCESS si communication ok
        """
        return self.ICNC2Lib.ICNC_ErrorReset(ErrorBits)
    
    def GetInput(self, InNo):
        """
        Read one of digital input (DIN1 to DIN32)
        <param name="InNo">Input number to read (1..32)</param>
        <param name="InputState">Pointer for return result. Value is 0 if the input is inactive and 1 the input is active</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        myInt = ctypes.c_int()
        ans=self.ICNC2Lib.ICNC_GetInput(InNo,ctypes.byref(myInt))
        return ans,myInt

    def GetOutputAll(self):
        """
        Read one of digital input (DIN1 to DIN32)
        <param name="InNo">Input number to read (1..32)</param>
        <param name="InputState">Pointer for return result. Value is 0 if the input is inactive and 1 the input is active</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        myInt = ctypes.c_int()
        ans = self.ICNC2Lib.ICNC_GetOutputAll(ctypes.byref(myInt))
        return ans,myInt

    def GetBoardStatus(self, StatusType): # output param:      *Status 
        """
        d'etat interne de la carte par un seul appel a cette fonction.
        Pour eviter de surcharger la communication, faite une demande approriee en fonction
        des informations necessaire a l'instant donne.
        Input :
                StatusType : combinaison des bits d'etat a lire
                        (exemple : STATUS_BOARD_STATUS | STATUS_BUFFER_FREE
        Output :
            Status : pointeur sur une zone memoire permettant le stockage de tous les
                        etats demandes.
        Result :  ICNCUSB_SUCCESS si communication ok
            Exemple1 : Lecture simultanee du status de la carte, de la place disponible dans le buffer
                et de la position actuelle des axes X, Y et Z
                Status DWORD[5];
        GetBoardStatus( STATUS_BOARD_STATUS | STATUS_BUFFER_FREE
                            | STATUS_ACTUALX | STATUS_ACTUALY | STATUS_ACTUALZ,
                            &Status);
        """
        myInt = ctypes.c_int()
        ans=self.ICNC2Lib.ICNC_GetBoardStatus(StatusType,ctypes.byref(myInt))
        return ans,myInt

    def GetBufferStatus():
        """
        Read free space in buffer command for CNC application
        This function is optimized version of ICNC_GetBoardStatus with ICNC_STATUS_BOARD_STATUS request
        The buffer size can be read from system information ICNC_SYS_INFO_BUFFER_SIZE with ICNC_GetSysInfo function
        <param name="FreeBufferSpace">Pointer for free space return value</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        print("NOT YET IMPLEMENTED: NEED TO UNDERSATND FreeBufferSpace");
        myInt = ctypes.c_int()
        ans = self.ICNC2Lib.ICNC_GetBufferStatus(ctypes.byref(myInt));
        return ans,myInt
    
    def MoveSpeedAbsBuf(self,Axis,Speed,MoveX,MoveY,MoveZ,MoveA,MoveB):
        """
        COMMENTAIRE DU ICNC2.h DE PLASMAGUI
        Absolute movement without speed profile
        Déplacement à une position absolue avec profile (trapézoïdale) utilisant un buffer local
        sur le PC afin d'envoyer des trames complètes sur la cartee et optimiser le flux de communication USB
        """
        return self.ICNC2Lib.ICNC_MoveSpeedAbsBuf(Axis,Speed,MoveX,MoveY,MoveZ,MoveA,MoveB)

    def MoveProfileAbsBuf(self,Axis,Speed,PositionX,PositionY,PositionZ,PositionA,PositionB):#WORD* BufferRequired = NULL);
        """
        Absolute position movement with speed profile
        """
        return ICNC_MoveProfileAbsBuf(Axis,Speed,PositionX,PositionY,PositionZ,PositionA,PositionB) # last parameter BufferRequired will have the default value zero 

    def MachineHome(self,Axes,MaxStrokeX=0xFFFFFFFF,MaxStrokeY=0xFFFFFFFF,MaxStrokeZ=0xFFFFFFFF,MaxStrokeA=0xFFFFFFFF,MaxStrokeB=0xFFFFFFFF):
        """
        Start Homing sequence on one or several axes.
        Homing parameters are define in InterpCNC parameters (see EE_ORIGINE_xxxxx for details)
        When homing start, STATUS_BIT_HOMING of STATUS_BOARD_STATUS register is set.
        When homing is completed, this bit is clear and you can test the STATUS_BIT_HOMING_ERROR bit
        to control the result.
        Homing will fail if homing sensor of one axis is not reach while moving up to maximum stroke define.
 If A or B axis are define as X slave, homing sequence of this slave will be launched automaticaly if AXE_X is set
        </summary>
        <param name="Axes">Axis combination of counter to write AXE_X to AXE_B</param>
        <param name="MaxStrokeX">MaxStroke for X  if AXE_X bit is set in Axes. Ignored if not</param>
        <param name="MaxStrokeY">MaxStroke for Y  if AXE_Y bit is set in Axes. Ignored if not</param>
        <param name="MaxStrokeZ">MaxStroke for Z  if AXE_Z bit is set in Axes. Ignored if not</param>
        <param name="MaxStrokeA">MaxStroke for A  if AXE_A bit is set in Axes. Ignored if not</param>
        <param name="MaxStrokeB">MaxStroke for B  if AXE_B bit is set in Axes. Ignored if not</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        <exemple>Homing sequence
        <code>
        int main()
        {
		DWORD Result;
		if (Connect(0)!=ICNCUSB_SUCCESS)
			return -1;
		if (Home(AXE_Z,
						0,			// ignored
						0,			// ignored
						50000,		// Max stroke before homing switch
						0,			// ignored
						0);			// ignored
			!= ICNCUSB_SUCCESS)
				return -1;
		
		// wait for Z homing completion
		do {
			GetBoardStatus(STATUS_BOARD_STATUS, &Result);
			Sleep(10);	// Leave time for other process
		while ( (Result&STATUS_BIT_HOMING) != 0);	
		
		// Check for Error status bit	
		if ( (Result&STATUS_BIT_HOMING_ERROR) != 0) {
			printf ("Z homing error");
			return -1;
		}
        
		if (Home(AXE_X | AXE_Y,
						100000,			// Max stroke X before homing switch
						100000,			// Max stroke Y before homing switch
						0,			// ignored
						0,			// ignored
						0);			// ignored
			!= ICNCUSB_SUCCESS)
				return -1;
		
		// wait for X en Y homing completion
		do {
			GetBoardStatus(STATUS_BOARD_STATUS, &Result);
			Sleep(10);	// Leave time for other process
		while ( (Result&STATUS_BIT_HOMING) != 0);	
		
		// Check for Error status bit	
		if ( (Result&STATUS_BIT_HOMING_ERROR) != 0) {
			printf ("X or Y homing error");
			return -1;
		}
		printf ("Home completed");
        
		Disconnect();
		return 0;
        """
        return self.ICNC2Lib.ICNC_MachineHome(Axes, MaxStrokeX,MaxStrokeY,MaxStrokeZ,MaxStrokeA,MaxStrokeB)

    def SlowStopAllAndClear(self):
        """
        Stop all interpolated mouvement (CNC use case) or asynchrone mouvement and flush the command buffer.
        Stop will be done with deceleration
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        return self.ICNC2Lib.ICNC_SlowStopAllAndClear()

    def SetOutput(self,OutputNo, OutputState):
        """
        Set or Reset one digital output (DOUT1..DOUT32)
        <param name="OutputNo">Analog input number to read (1..32)</param>
        <param name="OutputState">New output state</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        return self.ICNC2Lib.ICNC_SetOutput(OutputNo,OutputState)


    def SetOutputBuf(self,OutputNo,OutputValue):
        """
        """
        return self.ICNC2Lib.ICNC_SetOutputBuf(OutputNo,OutputValue)

    def SetSimulationMode(self,SimulationModeVal):
        """
        Permet de passer la DLL en mode simulation sans communication physique
        avec la carte. Il est ainsi possible de tester le programme pour ce qui est
        des acces aux fonctions de la DLL
        Input : True pour passer en mode simulation
        <returns>always ICNCUSB_SUCCESS</returns>
        """
        return self.ICNC2Lib.ICNC_SetSimulationMode(SimulationModeVal)

    def WriteParameter(self,ParameterID,ParameterValue,WaitCompletion=1,TimeOut=1000):
        """
        Write one of InterpCNC parameter based on parameter's ID
        Parameters ID are define in ParametersID.h
        <param name="ParameterID">Parameter's ID to write</param>
        <param name="ParameterValue">pointer for return value</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        return self.ICNC2Lib.ICNC_WriteParameter(ParameterID,ParameterValue,WaitCompletion,TimeOut);

    def WriteUserMem(self,MemoryNumber,MemoryValue):
        """
        Write in User memory registers
        The number of available user memory register can be obtain by reading the ICNC_SYS_INFO_USR_MEM_SIZE register.
        Each one can store a 32bits value and are located in RAM. Then, values are lost when power of.
        Two type of write to user memory are available. One is immediate, one is bufferized.
        The bufferized one can help you to follow the execution of bufferized command.
For this purpose, you can make a bufferized write with a counter for some particular points of your process.
        Reading this user mem will inform you about the process excecution.
        </summary>
        <param name="MemoryNumber">User memory register number (1..16)</param>
        <param name="MemoryValue">Value to write</param>
        <returns>ICNCUSB_SUCCESS if success. ICNCUSB_FAIL in case of communication error</returns>
        """
        return self.ICNC2Lib.ICNC_WriteUserMem(MemoryNumber,MemoryValue)

    def WaitBuf(self,TimeToWait):
        """
        TimeToWait en 100ms
        """
        return self.ICNC2Lib.ICNC_WaitBuf(TimeToWait)

    def GetDLLVersion(self):
        """
        Retourne le numero de la dll
        """
        return self.ICNC2Lib.ICNC_GetDLLVersion()

    def Disconnect(self):
        """
        Arret de la communication avec la carte InterpCNC
        """
        self.connected = 0
        return self.ICNC2Lib.ICNC_Disconnect()

    def SetUserApplicationID(self, lUserApplicationID):
        """
        Definition de l'ID application
        """
        self.notImplemented()
        return -1


# *****************************************************************************************
# *                                                                                       *
# *****************************************************************************************
if __name__ == "__main__":
        print("hy")
        i = ICNC2()
        print("connection : ",i.Connect())
        status = i.STATUS_BOARD_STATUS | i.STATUS_BUFFER_FREE | i.STATUS_ACTUALX | i.STATUS_ACTUALY | i.STATUS_ACTUALZ
        ret,status = i.GetBoardStatus(status)
        print("board status : ",status)
        print("you can use the class i, for example dir(i)")
        print("       i.MachineHome(i.AXE_X)")
        print("       i.MoveSpeedAbsBuf(i.AXE_Y,Speed,MoveX,MoveY,MoveZ,MoveA,MoveB)")
        print("       i.ErrorReset(i.STATUS_BIT_LOCKED | i.STATUS_BIT_EEWRITE_ERROR | i.STATUS_BIT_EMERGENCYSTOP)")
        print("       i.GetBoardStatus(i.STATUS_BOARD_STATUS | i.STATUS_ACTUALX | i.STATUS_ANALOG_IN1)")
        print("       i.GetInput(5)")
        print("       i.GetOutputAll()")
        print("       i.MoveProfileAbsBuf(i.AXE_Y,Speed,PositionX,PositionY,PositionZ,PositionA,PositionB)")
        print("       i.SlowStopAllAndClear()")
        print("       i.SetOutput(6, 1)")
        print("       i.SetOutputBuf(3,4)")
        print("       i.WriteParameter(ParameterID,ParameterValue,WaitCompletion,TimeOut)")
        print("       i.WriteUserMem(6,9)")
        print("       i.WaitBuf(60)")
        print("       i.MoveSpeedAbsBuf(Axis,Speed,MoveX,MoveY,MoveZ,MoveA,MoveB)")
