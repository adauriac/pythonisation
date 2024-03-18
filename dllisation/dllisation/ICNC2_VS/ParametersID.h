#pragma once

// Vitesses maximum de déplacement des axes
#define EE_MAX_XSPEED            0  // Not used
#define EE_MAX_YSPEED            1 // Not used
#define EE_MAX_ZSPEED            2 // Not used
// Vitesse de déplacement par défaut
#define EE_DEFAULT_SPEED         3  //
#define EE_DEFAULT_ACCEL         4  //
#define EE_DEFAULT_STARTF        5


// Sens de rotation des moteurs
#define EE_XSENS                 6
#define EE_YSENS                 7
#define EE_ZSENS                 8
#define EE_ASENS                 37
#define EE_BSENS                 49

// Sens de rotation pour la prise d'origine
#define EE_ORIGINE_XSENS                 9
#define EE_ORIGINE_YSENS                 10
#define EE_ORIGINE_ZSENS                 11
#define EE_ORIGINE_ASENS                 38
#define EE_ORIGINE_BSENS                 50
#define EE_ORIGINE_SPEED_RAPIDE          12   // Commune à tous les axes en commande RS232, pour l'axe X en USB
#define EE_ORIGINE_SPEED_RAPIDEY         53   // 
#define EE_ORIGINE_SPEED_RAPIDEZ         54   // 
#define EE_ORIGINE_SPEED_RAPIDEA         55   // 
#define EE_ORIGINE_SPEED_RAPIDEB         56   // 
#define EE_ORIGINE_ACCEL_RAPIDE          13	  // Commune à tous les axes en commande RS232, pour l'axe X en USB
#define EE_ORIGINE_ACCEL_RAPIDEY         57	  // 
#define EE_ORIGINE_ACCEL_RAPIDEZ         58	  // 
#define EE_ORIGINE_ACCEL_RAPIDEA         59	  // 
#define EE_ORIGINE_ACCEL_RAPIDEB         60	  // 
#define EE_ORIGINE_STARTF_RAPIDE         14
#define EE_ORIGINE_SPEED_LENTE           15

#define EE_ORIGINE_RESET_POS_SOFT        128	// Mise à 0 automatique des positions ou contrôlée par logiciel

// Paramètres pour l'apprentissage
#define EE_TEACH_SPEED     16
#define EE_TEACH_ACCEL     17
#define EE_TEACH_STARTF    18

// Paramètres pour recherche d'un contact (RS232 uniquement)
#define EE_PROBE_SPEED 19
#define EE_PROBE_ACCEL 20
#define EE_PROBE_STARTF 21


// Paramètres pour la gestion des fiin de courses
#define EE_AU 22
#define EE_FDC    23
#define EE_VEROUILLAGE 24
#define EE_CAPOT 25
#define EE_FREE1 26
#define EE_FREE2 27
#define EE_FREE3 39
#define EE_FREE4 40

#define EE_POLARITY_FDC 28

// Paramètres pour la gestion des contacts de prise d'origine
#define EE_FDC_ORIGINEX 29
#define EE_FDC_ORIGINEY 30
#define EE_FDC_ORIGINEZ 31
#define EE_FDC_ORIGINEA 41
#define EE_FDC_ORIGINEB 51

#define EE_POLARITY_OM  32

#define EE_PWM_MODE 33        // 0=>PWM, 1=>Fréquence variable
#define EE_FPWM 43            // (0=>2.5KHz, 1=>5KHz, 2=>10KHz, 3 et plus => 20KHz)
#define EE_FMAX 44            // Fréquence en KHz max (obtenue avec commande WM500) si mode sortie fréquence

#define EE_MIN_PWMVAL 45      // Valeur de PWM correspondant à la commande WM0
#define EE_MAX_PWMVAL 46      // Valeur de PWM correspondant à la commande WM500
#define EE_OUTPUT_START  47     // Etat des sortie à la mise sous tension de l'InterpCNC


#define EE_MAX_COURSE_X 34     // Course max axe X pour commande H, P
#define EE_MAX_COURSE_Y 35
#define EE_MAX_COURSE_Z 36
#define EE_MAX_COURSE_A 42
#define EE_MAX_COURSE_B 52

#define EE_PULS_PAR_TOUR 48


#define EE_OVERIDE_SOURCE 61  // 0 = Programme, 1=ANA1, 2 ANA2
#define EE_OVERRIDE_MINI 62
#define EE_OVERRIDE_MAXI 63

#define EE_EMPTY_OVERRIDE1 64
#define EE_EMPTY_OVERRIDE2 65
#define EE_EMPTY_OVERRIDE3 66
#define EE_EMPTY_OVERRIDE4 67


// THC Control parameters
#define EE_THC_ALLOWED				68	// Activation/désactivation de la fonction THC
#define EE_THC_FMAX 				69	// Fréquence max de déplacement en cours de régulation
#define EE_THC_MAX_SPEED_DEVIATION 	70	// Accel/decel en Hz/unité de temps des mouvements de régulation
#define EE_THC_AIN_FILTER_TIME 		71	// Filtre entrée analogique
#define EE_THC_AIN_NUMBER 			72	// Numéro de l'entrée analogique de mesure
#define EE_THC_PID_SAMPLE_TIME 		73	// Période d'échantillonage du PI en ms
#define EE_THC_KI 					74	// Gain Intégrateur
#define EE_THC_KP 					75	// Gain proportionel
#define EE_THC_IMAX 				76	// Limitation positive de l'action de l'intégrateur (Hz)
#define EE_THC_IMIN 				77  // Limitation Négative de l'action de l'intégrateur (Hz)

#define EE_THC_MAX_CORRECTION_PLUS	78	// Distance maxi de correction +
#define EE_THC_MAX_CORRECTION_MOINS 79  // Distance mini de correction -

#define EE_THC_ACTIVATION_OUTPUT    80  // Sortie dont l'activation provoque la mise en route du THC
#define EE_THC_CONTROL_INPUT    	81  // Entrée de validation du THC

#define EE_THC_TEMPO_START    	    82  // Temporisation d'activation du THC

#define EE_THC_XYSPEED_LIMITATION 83	// Enable/disable speed limitation depending of actual XY speed
#define EE_THC_XYSPEED_LIMITATION_VALUE 84   // % of actual XY speed

#define EE_THC_SOURCE			 85   // source de réglage du THC 0=> Logiciel, 1=>Analog 1 ...
#define EE_THC_SOURCE_MINI			 90   // Valeur mini de consigne THC correspondant à une entrée à 0V si entrée analogique sélectionnée
#define EE_THC_SOURCE_MAXI			 91   // Valeur maxi de consigne THC correspondant à une entrée à 0V si entrée analogique sélectionnée


#define  EE_COUPLAGE_AXE   86	// Permet de lier le déplacement de X et A si 1, de X et B si 2, pas de couplage si 0
								// Par exemple, si  EE_COUPLAGE_AXE == 0x01, les commande de déplacement X seront 
								// également appliquées à l'axe A (pour les déplacements interpolés uniquement)

#define EE_OUTPUT_SET_NC_READY 87   // Numéro de sortie activé lorsque Locked=False
#define EE_OUTPUT_RESET_NC_READY 92   // Numéro de sortie activé lorsque Locked=False
#define EE_OUTPUT_SET_NC_NOT_READY 93   // Numéro de sortie activé lorsque Locked=False
#define EE_OUTPUT_RESET_NC_NOT_READY 94   // Numéro de sortie activé lorsque Locked=False
#define EE_INPUT_UNLOCK    88   // Numéro d'entrée de réarmement de la carte
#define EE_INPUT_UNLOCK_POLARITY 89  // Polarité entrée de réarmement


#define EE_OUTPUT_ZONE 95   // Numéro de sortie activé lorsque position X > LIMITEX_ZONE1
#define	LIMITEX_ZONE1  96   // Position X limite de Zone 1


#define	EE_CLAVIER_MATRICE  97   // Activation gestion clavier matriciel
#define EE_MATRICE_LIGNE 98			 // Nombre de ligne clavier (sorties)
#define EE_MATRICE_COLONNE 99	   // Nombre de colone clavier (entrées)
#define EE_FILTRE_CLAVIER 100	   // Filtre anti rebond (ms)

#define EE_MODBUS_SPEED 101
#define EE_MODBUS_PARITY 102
#define EE_MODBUS_BASE_ADRESS 103


// Paramètres liés à la gestion des fins de course
#define EE_FDC_XNEG 104
#define EE_FDC_XNEG_POLARITY 105
#define EE_FDC_XPOS 106
#define EE_FDC_XPOS_POLARITY 107

#define EE_FDC_YNEG 108
#define EE_FDC_YNEG_POLARITY 109
#define EE_FDC_YPOS 110
#define EE_FDC_YPOS_POLARITY 111

#define EE_FDC_ZNEG 112
#define EE_FDC_ZNEG_POLARITY 113
#define EE_FDC_ZPOS 114
#define EE_FDC_ZPOS_POLARITY 115

#define EE_FDC_ANEG 116
#define EE_FDC_ANEG_POLARITY 117
#define EE_FDC_APOS 118
#define EE_FDC_APOS_POLARITY 119

#define EE_FDC_BNEG 120
#define EE_FDC_BNEG_POLARITY 121
#define EE_FDC_BPOS 122
#define EE_FDC_BPOS_POLARITY 123

#define EE_ACTION_FDC 124

#define EE_USE_BRESENHAM 125

#define EE_ENCODER_MODE 126		

#define EE_AUTORUN_BASIC 127		// Chargement et lancement automatique du programme Basic à la mise sous tension				


#define EE_FILTRE_ENTREE 129  // période de filtrage entrées TOR (en multiple de 5ms et seulement sur MINI_ICNC2)

#define EE_AOUT1_LOCKED_VALUE 130	// Valeurs initiales sorties analogiques (au boot ou si verouillage)
#define EE_AOUT2_LOCKED_VALUE 131
#define EE_AOUT3_LOCKED_VALUE 132
#define EE_AOUT4_LOCKED_VALUE 133


// Stepper driver initialisation parameters
#define EE_DRVCTRL 	134
#define EE_CHOPCONF 135
#define EE_SMARTEN 	136
#define EE_SGCSCONF      137
#define EE_DRVCONF 	     138
#define EE_SGVMIN	     139
#define EE_STANDBY_LEVEL 140
#define EE_STANDBY_DELAY 141


#define EE_HOMING_FILTRE_ALLE 150
#define EE_HOMING_FILTRE_RETOUR 151

