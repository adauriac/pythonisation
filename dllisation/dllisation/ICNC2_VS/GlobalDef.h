/* *******************************************************************
 * COPYRIGHT -- SOPROLEC
 ********************************************************************
 * Program: -
 * File: GlobalDef.h
 * Author: Eric DARRET
 * Created: January 16, 2009
 ********************************************************************
 * InterpCNC V2 definition
 ******************************************************************* */


#ifndef _GLOBAL_DEF_H_
#define _GLOBAL_DEF_H_

#define ICNC_CONST_MAX_READ_EEPROM_SIZE  62	// Maximum number of byte to read with CMD_READ_EEPROM commande
#define ICNC_CONST_MAX_WRITE_EEPROM_SIZE 57  // Maximum number of byte to write with CMD_WRITE_EEPROM commande


//b0 0 b4 : axes to move bits

#define ICNC_AXE_NONE    0
#define ICNC_AXE_X      0x01
#define ICNC_AXE_Y      0x02
#define ICNC_AXE_YX      0x03
#define ICNC_AXE_Z      0x04
#define ICNC_AXE_ZX      0x05
#define ICNC_AXE_ZY      0x06
#define ICNC_AXE_ZYX      0x07
#define ICNC_AXE_A      0x08
#define ICNC_AXE_AX      0x09
#define ICNC_AXE_AY      0x0A
#define ICNC_AXE_AYX      0x0B
#define ICNC_AXE_AZ      0x0C
#define ICNC_AXE_AZX      0x0D
#define ICNC_AXE_AZY      0x0E
#define ICNC_AXE_AZYX   0x0F
#define ICNC_AXE_B      0x10
#define ICNC_AXE_BX      0x11
#define ICNC_AXE_BY      0x12
#define ICNC_AXE_BYX      0x13
#define ICNC_AXE_BZ      0x14
#define ICNC_AXE_BZX      0x15
#define ICNC_AXE_BZY      0x16
#define ICNC_AXE_BZYX   0x17
#define ICNC_AXE_BA      0x18
#define ICNC_AXE_BAX      0x19
#define ICNC_AXE_BAY      0x1A
#define ICNC_AXE_BAYX   0x1B
#define ICNC_AXE_BAZ      0x1C
#define ICNC_AXE_BAZX   0x1D
#define ICNC_AXE_BAZY   0x1E
#define ICNC_AXE_BAZYX   0x1F


// Numéro de voie analogique
#define ICNC_ANALOG1      0x01
#define ICNC_ANALOG2      0x02
#define ICNC_ANALOG3      0x03
#define ICNC_ANALOG4      0x04


// Source de réglage override
#define ICNC_CONST_OVERRIDE_PROG		   	0  // Override is set from PC application
#define ICNC_CONST_OVERRIDE_ANA1		   	1  // Override is set from analog input 1	
#define ICNC_CONST_OVERRIDE_ANA2        	2  // Override is set from Analog Input 2
#define ICNC_CONST_OVERRIDE_ANA3        	3  // Override is set from Analog Input 3
#define ICNC_CONST_OVERRIDE_ANA4        	4  // Override is set from Analog Input 4

											
// Bit pour interogation de status par commande CMD_GET_STATUS
#define ICNC_STATUS_BOARD_STATUS       		0x00000001  // get internal board status bits (bits details define with ICNC_STATUS_BIT_xxx constants)
#define ICNC_STATUS_BUFFER_FREE        		0x00000002	// get actual command buffer free space for CNC application
#define ICNC_STATUS_ACTUALX            		0x00000004	// get actual pos X
#define ICNC_STATUS_ACTUALY            		0x00000008	// get actual pos Y
#define ICNC_STATUS_ACTUALZ            		0x00000010	// get actual pos Z
#define ICNC_STATUS_ACTUALA            		0x00000020	// get actual pos A
#define ICNC_STATUS_ACTUALB            		0x00000040	// get actual pos B
#define ICNC_STATUS_INPUT              		0x00000080	// get inputs state
#define ICNC_STATUS_LAST_PROBE         		0x00000100  // Get last probe position result
#define ICNC_STATUS_ANALOG_IN1			  	0x00000200	// Get analog 1 input value (based on ADC range)
#define ICNC_STATUS_ANALOG_IN2			  	0x00000400	// Get analog 2 input value (based on ADC range)
#define ICNC_STATUS_ANALOG_IN3			  	0x00000800	// Get analog 3 input value (based on ADC range)
#define ICNC_STATUS_ANALOG_IN4			  	0x00001000	// Get analog 4 input value (based on ADC range)
#define ICNC_STATUS_THC_TARGET				0x00002000	// Get actual THC target
#define ICNC_STATUS_MATRIX_KEY				0x00004000	// Get actual matrix key state
#define ICNC_STATUS_ENCODER    				0x00008000	// Get actual encoder counter
#define ICNC_STATUS_A_PULSES_COUNTER		0x00010000	// Get actual input A pulses counter
#define ICNC_STATUS_B_PULSES_COUNTER		0x00020000	// Get actual input B pulses counter
#define ICNC_STATUS_A_PULSES_FREQUENCY		0x00040000	// Get actual input A pulses frquency (unit is 1/100Hz)
#define ICNC_STATUS_B_PULSES_FREQUENCY		0x00080000	// Get actual input B pulses counter (unit is 1/100Hz)
#define ICNC_STATUS_ANALOG_AOUT1			0x00100000	// Get actual analog output 1 state
#define ICNC_STATUS_ANALOG_AOUT2      		0x00200000	// Get actual analog output 2 state
#define ICNC_STATUS_BOARD_STATUS2	        0x00400000	// Get extended board status (Status2)
#define ICNC_STATUS_DOUT_STATE  	        0x00800000	// Get the actual state of DOUT1 to DOUT32

// BOARD_STATUS bits signification
#define ICNC_STATUS_BIT_XMOVING				0x00000001	// X axis interpolation moving
#define ICNC_STATUS_BIT_YMOVING				0x00000002	// Y axis interpolation moving
#define ICNC_STATUS_BIT_ZMOVING				0x00000004	// Z axis interpolation moving
#define ICNC_STATUS_BIT_AMOVING				0x00000008	// A axis interpolation moving
#define ICNC_STATUS_BIT_BMOVING				0x00000010	// B axis interpolation moving
#define ICNC_STATUS_BIT_BUFFER_EMPTY		0x00000020	// Commande buffer is empty
#define ICNC_STATUS_BIT_BUFFER_FREZED		0x00000040	// Freez buffer is set (buffer commands paused for pre-fill)
#define ICNC_STATUS_BIT_EMERGENCYSTOP		0x00000080	// Emergency stop has been pressed
#define ICNC_STATUS_BIT_LOCKED				0x00000100	// Board locked (startup, emmergency pressed or stoke limit)
#define ICNC_STATUS_BIT_STROKE_LIMIT		0x00000200	// One limit switch has been activated
#define ICNC_STATUS_BIT_HOMING				0x00000400	// Homing in progress
#define ICNC_STATUS_BIT_HOMING_ERROR		0x00000800	// Homing failed (overstrock)
#define ICNC_STATUS_BIT_PROBING				0x00001000	// Probe in progress
#define ICNC_STATUS_BIT_PROBING_ERROR		0x00002000	// Probe failed (overstrock or wrong parameters)
#define ICNC_STATUS_BIT_EEWRITE_INPROGRESS	0x00004000	// Write in internal EEPROM (parameters) in progress)
#define ICNC_STATUS_BIT_EEWRITE_ERROR		0x00008000  // error when read back control after write operation 
#define ICNC_STATUS_BIT_PROMPT_STATE		0x00010000	// Get actual prompt management status  
#define ICNC_STATUS_BIT_OVERRIDE			0x00020000	// Set if actual override value is <>100%
#define ICNC_STATUS_BIT_OVERRIDE_ALLOWED	0x00040000	// Indication of the override status (autorized=0 or not if 1)
#define ICNC_STATUS_BIT_WAIT_INPUT			0x00080000	// Timeout occured diring wait input state function0x00010000
#define ICNC_STATUS_BIT_WAIT_INPUT_ERROR	0x00100000	// Timeout occured diring wait input state function or wrong data input
#define ICNC_STATUS_BIT_THC_ACTIVATED	    0x00200000	// THC in ON
#define ICNC_STATUS_BIT_X_ASYNC_MOVING		0x00400000	// X asynchonous move in progress
#define ICNC_STATUS_BIT_Y_ASYNC_MOVING		0x00800000	// Y asynchonous move in progress
#define ICNC_STATUS_BIT_Z_ASYNC_MOVING		0x01000000	// Z asynchonous move in progress
#define ICNC_STATUS_BIT_A_ASYNC_MOVING		0x02000000	// A asynchonous move in progress
#define ICNC_STATUS_BIT_B_ASYNC_MOVING		0x04000000	// B asynchonous move in progress
#define ICNC_STATUS_BIT_X_LIMIT				0x08000000	// X axis limit switch active
#define ICNC_STATUS_BIT_Y_LIMIT				0x10000000	// Y axis limit switch active
#define ICNC_STATUS_BIT_Z_LIMIT				0x20000000	// Z axis limit switch active
#define ICNC_STATUS_BIT_A_LIMIT				0x40000000	// A axis limit switch active
#define ICNC_STATUS_BIT_B_LIMIT				0x80000000	// B axis limit switch active

// BOARD_STATUS 2 bits signification
#define ICNC_STATUS2_BIT_XDIRECTION			0x00000001	// Actual X axis moving direction
#define ICNC_STATUS2_BIT_YDIRECTION			0x00000002	// Actual Y axis moving direction
#define ICNC_STATUS2_BIT_ZDIRECTION			0x00000004	// Actual Z axis moving direction
#define ICNC_STATUS2_BIT_ADIRECTION			0x00000008	// Actual A axis moving direction
#define ICNC_STATUS2_BIT_BDIRECTION			0x00000010	// Actual B axis moving direction
#define ICNC_STATUS2_BIT_BASIC_TX_OVERRUN	0x00000020	// TX buffer used by Print function overrun (when using internal Basic interpreter))
#define ICNC_STATUS2_BIT_BASIC_RX_OVERRUN	0x00000040	// RX buffer used for transmit program overrun (when using internal Basic interpreter))
#define ICNC_STATUS2_BIT_BASIC_RUNNING      0x00000080 	// Internal basic program is runing
#define ICNC_STATUS2_BIT_DMX_RECEIVED   	0x00000100	 
#define ICNC_STATUS2_BIT_RECEIPE_CHANGED	0x00000200	
#define ICNC_STATUS2_BIT_B10				0x00000400
#define ICNC_STATUS2_BIT_B11				0x00000800
#define ICNC_STATUS2_BIT_B12				0x00001000
#define ICNC_STATUS2_BIT_B13				0x00002000
#define ICNC_STATUS2_BIT_B14				0x00004000
#define ICNC_STATUS2_BIT_B15				0x00008000  	
#define ICNC_STATUS2_BIT_MB_RECEIVED		0x00010000	// Data received on Modbus chanel (automatically reset after read)
#define ICNC_STATUS2_BIT_MB_USR_RPM_CHANGED	0x00020000	// User EEPROM parameter changed (automatically reset after read)
#define ICNC_STATUS2_BIT_HOMING_X			0x00040000		
#define ICNC_STATUS2_BIT_HOMING_X_ERROR		0x00080000	
#define ICNC_STATUS2_BIT_HOMING_Y			0x00100000	
#define ICNC_STATUS2_BIT_HOMING_Y_ERROR	    0x00200000	
#define ICNC_STATUS2_BIT_HOMING_Z			0x00400000
#define ICNC_STATUS2_BIT_HOMING_Z_ERROR		0x00800000
#define ICNC_STATUS2_BIT_HOMING_A			0x01000000
#define ICNC_STATUS2_BIT_HOMING_A_ERROR		0x02000000
#define ICNC_STATUS2_BIT_HOMING_B			0x04000000
#define ICNC_STATUS2_BIT_HOMING_B_ERROR		0x08000000
#define ICNC_STATUS2_BIT_B28				0x10000000
#define ICNC_STATUS2_BIT_B29				0x20000000
#define ICNC_STATUS2_BIT_B30				0x40000000
#define ICNC_STATUS2_BIT_B31				0x80000000



// Bits pour interogation des informations systeme status par commande CMD_GET_SYS_INFO
#define ICNC_SYS_INFO_BUFFER_SIZE      0x00000001   // Taille du buffer de commande (en int32)
#define ICNC_SYS_INFO_MAX_FREQUENCY      0x00000002  // Frequence maximum des déplacements
#define ICNC_SYS_INFO_APP_VERSION_H      0x00000004  // Numéro majeur de version d'application
#define ICNC_SYS_INFO_APP_VERSION_L      0x00000008  // Numéro mineur de version d'application
#define ICNC_SYS_INFO_LOADER_VERSION_H   0x00000010  // Numéro majeur de version du bootloader
#define ICNC_SYS_INFO_LOADER_VERSION_L   0x00000020  // Numéro mineur de version du bootloader
#define ICNC_SYS_INFO_BOARD_VERSION      0x00000040  // Numero de version de carte
#define ICNC_SYS_INFO_EEPROM_SIZE        0x00000080  // Taille EEPROM
#define ICNC_SYS_INFO_USR_MEM_SIZE       0x00000100  // Nombre d'INT32 disponible pour application client (mémoire utilisateur)
#define ICNC_SYS_INFO_AVAILABLE_AOUT    0x00000200  // Nombre de sorties analogiques
#define ICNC_SYS_INFO_AVAILABLE_AIN     0x00000400  // Nombre d'entrées analogiques 
#define ICNC_SYS_INFO_DAC_RESOLUTION	0x00000800  // Analog output converter resolution
#define ICNC_SYS_INFO_ADC_RESOLUTION	0x00001000	// Analog input converter resolution
#define ICNC_SYS_INFO_AVAILABLE_DOUT	0x00002000  // number of available digital output
#define ICNC_SYS_INFO_AVAILABLE_DIN		0x00004000  // Number of available digital input

// ****
// **** Bufferized commandes
// ****
#define ICNC_CMD_MOVE_CONSTANT_REL_BUF               1   // Relatif move without rampe (constant speed)
#define ICNC_CMD_MOVE_CONSTANT_ABS_BUF               25   // Absolute move without rampe (constant speed)
#define ICNC_CMD_MOVE_RAMP_BUF                  2     // Relatif move with rampe (up or down) // not yet implemented

#define ICNC_CMD_RAMPUP_ABS_BUF 			3	// Ramp up move
#define ICNC_CMD_RAMPDOWN_ABS_BUF 			4	// Ramp down move

#define ICNC_CMD_MOVE_ARC_XY_BUF						97  // Circular XY arc


#define ICNC_CMD_PROFILE_ABS_BUF               6   // Absolute move with accel and decel rampe

#define ICNC_CMD_PROFILE_REL_BUF               7   // Relatif move with accel and decel rampe

#define ICNC_CMD_FREEZE_BUF                  11   // Freeze buffer until buffer is full or unfreeze commande
#define ICNC_CMD_SET_OUTPUT_BUF               13   // Set/Reset one output
#define ICNC_CMD_SET_OUTPUT_ALL_BUF            14   // Set/Reset of outputs
#define ICNC_CMD_SET_ANALOG_BUF               15   // 
#define ICNC_CMD_TEMPO_BUF                  20
#define ICNC_CMD_WAIT_INPUT_BUF               21
#define ICNC_CMD_WRITE_USER_MEM_BUF            22
#define ICNC_CMD_SET_OVERRIDE_STATE_BUF			23	// Allow or disallow the use of override
#define ICNC_CMD_MOVE_TO_SENSOR_BUF				24   // Move axis until switch detection
#define ICNC_CMD_SET_THC_ONOFF_BUF					26	// Activation/desactivation THC bufferisé
// ****
// **** Commandes non bufferisées
// ****
#define ICNC_CMD_PROBE                  40   
#define ICNC_CMD_BREAKE_AXES               41  // Breake (rampe down) specified axis
#define ICNC_CMD_BREAKE_AXES_AND_CLEAR      42  // Breake (rampe down) axis in movement and clear all buffer
#define ICNC_CMD_STOP                  43   // Immediate stop and clear all buffer
#define ICNC_CMD_GET_OUTPUT_ALL            44   // Read the actual outputs state
#define ICNC_CMD_GET_INPUT               45   // Read of one input state
#define ICNC_CMD_GET_INPUT_ALL            46   // Read inputs states
#define ICNC_CMD_GET_ANALOG               47
#define ICNC_CMD_SET_OUTPUT_ALL            48
#define ICNC_CMD_SET_OUTPUT                49
#define ICNC_CMD_SET_ANALOG               50


#define ICNC_CMD_GET_BUFFER_STATUS         55
#define ICNC_CMD_UNFREEZE               56
#define ICNC_CMD_WAIT_UNFREEZE              57   // Used for internal CMD status management. No USB function associated.
#define ICNC_CMD_WRITE_USER_MEM            58
#define ICNC_CMD_READ_USER_MEM            59

#define ICNC_CMD_READ_EEPROM               62
#define ICNC_CMD_WRITE_EEPROM            63
#define ICNC_CMD_READ_PARAMETER            64
#define ICNC_CMD_WRITE_PARAMETER            65
#define ICNC_CMD_ERROR_RESET				66  // For unlock board, reset emmergency stop or limit switch
#define ICNC_CMD_WRITE_POSITION			67  // Ecriture du compteur de position d'un axe

#define ICNC_CMD_GET_STATUS               70  // Return InterpCNC status information (axes pos, Digital input, analog input, buffer free space)
#define ICNC_CMD_MACHINE_HOME            71   // Start machine origine sequence
#define ICNC_CMD_SET_OVERRIDE            72  //
#define ICNC_CMD_GET_OVERRIDE            73  //
#define ICNC_CMD_SET_THC_TARGET			74  // THC set point

#define ICNC_CMD_CONFIG_PROMPT            75  // Enable/disable prompt
#define ICNC_CMD_GET_SYS_INFO            76 // Get miscallaneous system information (version, buffer size...
#define ICNC_CMD_GET_KEY				 77 // Read software licence key


#define ICNC_CMD_HOME_AXE               78 // Lancement d'un homing individuel sur un axe

#define ICNC_CMD_MOVE_PROFILE_ABS_ASYNC      80  // Lancement du déplacement d'un axe de manière asynchrone (sans interpolation)
																								 // Si le mouvement est déjà en cours, la cible est mise à jour
																								 // et le profile de vitesse recalculé
#define ICNC_CMD_MOVE_PROFILE_REL_ASYNC      81  // Lancement du déplacement d'un axe de manière asynchrone (sans interpolation)
#define ICNC_CMD_CHANGE_SPEED 							 96	// Modifie la vitesse de déplacement d'un axe en cours de mouvement


#define ICNC_CMD_BOOT_MODE      90  // Passage en mode Boot Loader
#define ICNC_CMD_GET_EXECTIME   91	// Lecture du compteur de temps de déplacement (cumul des temps en mouvement)

#define ICNC_CMD_LOCK_BOARD   92	// Vérouillage de la carte, vidage des buffers en attente d'un Unlock

#define ICNC_PULSE_FRAME		93
#define ICNC_USB_SPEED_SELECT		94

#define ICNC_CMD_SET_WATCHDOG 95	// Durée du watchdog de réception de commande (0=inactif par défaut).


#define ICNC_BASIC_CONTROL	98    // Controle de l'interpreteru Basic
#define ICNC_BASIC_CONTROL_GETTX 0 // Sub-Commande de remplissage du buffer de réception
#define ICNC_BASIC_CONTROL_SETRX 1 // Sub-Commande de lecture du buffer d'émission de l'interpréteur

#define ICNC_CMD_EXT_SPI 99	// Exploitation extension SPI

#define ICNC_CMD_MB_CONTROL	100    // Contrôle MOBUS 
#define ICNC_CMD_MB_BITS_SET 0     // Sub-Commande d'écriture des bits utilisateur Modbus
#define ICNC_CMD_MB_BITS_GET  1    // Sub-Commande de lecture des bits utilisateur Modbus
#define ICNC_CMD_MB_ONE_BIT_SET 2  // Sub-commande écriture d'un bit
#define ICNC_CMD_MB_GET_HOLDING_REG 3
#define ICNC_CMD_MB_SET_HOLDING_REG 4
#define ICNC_CMD_MB_GET_HOLDING_REGS 5  // Read 1 or more than one holding register (maximum of 31 registers in one time) NEW 27/02/2024
#define ICNC_CMD_MB_SET_HOLDING_REGS 6  // Write 1 or more than one holding register  (maximum of 30 registers in one time) NEW 27/02/2024
#define ICNC_CMD_MB_GET_INPUT_REGS 7  // Read 1 or more than one inputs register (maximum of 31 registers in one time) NEW 27/02/2024
#define ICNC_CMD_MB_GET_COILS 8         // Read coils  (maximum of 496 coils in one time) NEW 27/02/2024   
#define ICNC_CMD_MB_SET_COILS 9         // Write coils  (maximum of 480 coils in one time) NEW 27/02/2024
#define ICNC_CMD_MB_GET_INPUTS 10         // Lecture input bits  (maximum of 496 input bits in one time) NEW 27/02/2024



#define ICNC_CMD_CUSTOM 101			       // Custom commands
#define ICNC_CMD_CUSTOM_SETSYS 0	     // Set system register value
#define ICNC_CMD_CUSTOM_GETSYS 1	     // Get system register value
#define ICNC_CMD_CUSTOM_GETSYS_MULTI 2 // Get several system registers value 


#endif
