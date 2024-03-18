#ifndef _CONSTANT_H_
#define _CONSTANT_H_
// V5.37 :  17/08/2023
//          La commande StopAxes bloque le générateur de profile si appelée avec arrêt d'urgence actif
// V5.36 :  01/03/2023
//          Mise au point pour compatibilité avec ICNC2 4 axes
//          Fusion des version CNC et PLC pour ICNC2.2 et 2.4
//          Augmentation taille programme PLC (44Ko à 48Ko) pour ICNC2.2 et 2.4
//          Augmentation buffer de commande d'interpolation sans rampe (de 15 à 30 commande bufferisées) pour ICNC2.2 et 2.4
//
// V5.35 :  31/12/2020
//          Modification generation de pulse pour Galaad (delai entre pulse et changement de direction)
// V5.34 :  10/00/2020
//          Ajout commande SetCaptureInt pour déclencher interruption sur position axe ou compteur
//          Ajout commande 
// V5.33 :  10/07/2019
//          Ajout commande GetFCnt(1..2) pour mesure de fréquence sur entrée compteur)
//
// V5.32 :  11/06/2019
//          Optimisation gestion Interruption PLC
//          Ajout Commande PLCBasic Select Case
//              Select Case Expression
//                  Case Value[[, Value]...]
//                  Casse Is < 50
//                  Case 80 to 100
//                  Case Else
//              End Select
//          Ajout function PLCBasic Time$ qui retourne le temps écoulé depuis la mise sous tension au format hh:mn:s,ms
//          Sur InterpCNC 5 axes, version PLCC44, rafraichissement période de lecture des entrée à 1ms au lieu de 3ms
//   
// V5.31 :  22/05/2019
//          Lors d'un changement de cible à la volée, si il y a inversion du sens de rotation, les bits AsyncMoving 
//          passaient à 0 au moment de l'inversion. Donc, les tests de bits donnaient une impression de mouvement terminé.
//          Ajout commande PLCBasic SetCaptureInt IntNumber, AxeNumber, PositionForTrigger, Label
//              Provoque un saut au lable indiqué lorsue la position de trigger est atteinte.
//              Le trigger est automatiquement effacé
//          Ajout des modes 4, 5 et 6 d'interuption sur entrée (SetInputINT)
//          Ces modes correspondent respéctivement aux mode 1 à 3 mais sont automatiquement supprimé après detection de l'evenement (One Shoot)
//          
//
// V5.30 : 
//          Utilisation driver WINUSB
//          Gestion de recettes et fonction SetRCPIndex, SetRCPSize, CopyRecette, InsertRecette, RemoveRecette
//          Ajout commande modbus ICNC_CMD_HOME_AXE (78) pour lancer des commandes de homing indépendantes.
//          
// V5.29 : 24/09/18
//          - Ajout paramètre système pour inhiber l'utilisation des sorties DIRection des axes. 
//            Il devient ainsi possible d'utiliser les sorties DIRx avec la commande OUTTTL sans interferance avec les commande de déplacement d'axes.
//              commande : SetSys 45, MasqueBits 
//          - Augmentation du nombre de userMem (de 10 à 16)
// V5.28 : 20/11/2017
//    Ajout des commandes SetTimer, GetTimer, SRFill et SRAdd
//    La variable Timer retourne maintenant une valeur en ms avec des décimales
//      Ajout SetSys 43 pour fair une capture de TickCount utilisée en déduction de la valeur retournée par Timer 
// 
// V5.27 :
//                  Filtrage entrée ENABLE (100ms sur toutes les cartes)
//                  Filtrage entrées lors de la prise d'origine
//                  Sortie analogique 2 non réinitialisée en cas d'AU, utilisation valeur paramètre (version 3 axes)
// V5.26 :
//                  Corection bug position fin de prise d'origine sur commande basic Home
// V5.25 :
//                   Gestion Sortie OUT4 sur module 3 axes V1.3
//                   Ajout fonction basic Home
//                   Ajout GetSys 600..608 pour lecture status THC (version 5 axes)
// 28/12/2015 : V5.24:
//                   Correction bug pour multiCN et MACH3 (lié à calcul de vitesse automatique en fonction du remplissage buffer)
//					 Ajout gestion forcage entrées par TestCenter
// 09/07/2015 : V5.23
//              Ajout commande SetIn NumEntrée, -1-0 ou 1   de forcage état entrées (-1=forcage à 0, 0=pas de forcage, 1=forcage à 1)
//						Egalement accéssible avec SetSys/GetSys 40 et 41
//				Ajout commande OutTTL
//              Ajout version -DPLC_44Ko et/ou -DDMX_CONTROL
//					Pour version DMX : Ajout commande GetDMX(1..512)
// 31/07/2014 :	V5.22 :
//        Version 3 axes : Erreur affectation des sorties pour la commande SetOutputAll
//		  Version 5 axes : Réactivation gestion matrice clavier désactivée depui V5.19
// 25/06/2014 :	V5.21 :
//        Correction gestion arrêt mouvement asynchrone. StopAxe laissait repartir la commande suivante si déjà envoyée
//				Ajout fonction basic Lock
// 12/05/2014 :	V5.20 :
//			  Ajout fonction régulateur RST
//				Ajout régulateur avec un pôle et un zéro K * (z-A)/(z+B)
// 14/02/2014 : V5.19
//			Ajout fonction PID
//			Ajout commande basic écriture position codeur
//      Ajout paramètres pour valeurs initiales des sorties analogiques (appliquées lors du vérouillage carte, au boot ou si Enable=false)
//		  Correction bug sur sortie analogique pour consigne à 1 ou à 1023 (ICNC2 3 axes seulement)
// 28/11/13 : V5.18
//    Ajout commande USB accès aux bits utilisateurs Modbus
//    Correction bug accès aux bits utilisateur au delà du bit 352
//    Ajout commande Basic GetSys(20)=Position Probe
// 14/10/13 : V5.17
//    Ajout commande basic GetSys et SetSys
//    Correction blocage sur décélération  
// 13/02/13 : V5.16
//		Ajout paramètre pour remise à 0 automatique des compteur lors de la prise d'origine
//	  Ajout commande USB de lecture etat sorties analogique
//  11/11/13 : V5.15
//			Ajout interpreteur Basic intégré
// 		  Erreur gestion bit de status BufferFree si interrogation Modbus
//  11/10/12 : V5.14
//     Erreur sens de déplacement axe B si couplage axe = 2 avec nouvel algorithm d'interpolation linéaire
//		 Blocage suite à un stop immédiatement suivi d'une autre commande bufferisée
//  27/09/12 V5.13
	//	Correction vitesse sur déplacement à vitesse constante
	//	Ajout fonction d'interpolation circulaire sans rampe
	//  Prise en compte en dynamique des modification paramètres par Modbus
// 27/08/12 : V5.12
	// Ajout gestion entrée codeur/compteur (ICNC V2.1C et plus)
// 22/08/12 : V5.11
	// Bloquer les déplacement MultiCN lorsque la carte est vérouillée
// 10/08/12 : V5.10
	// Correction envoie CRC Modbus
// 10/08/12 : V5.9
	// Modification timing reponse Modbus
	// Ajout visualisation trame modbus sur LED D2
	// Ajout nouvel algorithme d'interpolation linéaire à vitesse constante
// 11/07/12 : V5.8
	// Modification de commande de déplacement Asynchrones pour changement de cible à la volée
// 14/06/12 : V5.7
	// Ajout commande de changement de vitesse en cours de déplacement asynchrone (USB et MODBUS)
	// Ajout gestion de fin de course sur tous les axes
// 13/06/12 : V5.6
	// Correction bug gestion axe A (blocage axe)
// 11/04/12 : V5.5
	// Ajout gestion Sorties en Modbus
	// Ajout commande modbusk d'écriture de compteur de position
	// Gestion couplage axes sur mouvement asynchrone
// 28/03/12 : V5.4
	// Correction gestion vitesses de communication modbus (4800 à 256000b/s)
	
// 08/03/2012 : V5.3
	// Augmentation vitesse de fonctionnement +20%
	// Ajout fonctions modbus gestion memoire utilisateur
	// Modification bootloader
// 10/02/12 : V5.2
	// Gestion de l'arrêt machine durant prise d'origine
	// Gestion de base Modbus
// 27/01/12 : V5.1
	// Ajout gestion clavier matriciel

#include "IO_Utils.h"


#ifndef MINI_ICNC2
#ifdef ICNC22
// Numéro de version du firmware  pour 4 et 5 axes (V2.2 et V2.4))
#define VERSION_H 5
#define VERSION_L 37// Numéro de version du firmware  
#else
// Numéro de version du firmware  pour ancienne 5 axes (V2.1)
#define VERSION_H 5
#define VERSION_L 36// Numéro de version du firmware  ^
#endif
#else
// Numéro de version du firmware  pour ancienne 3 axes 
#define VERSION_H 5
#define VERSION_L 36
#endif


#define BOOT_LOADER_H 1
#define BOOT_LOADER_L 2

#ifndef MINI_ICNC2 
#ifdef ICNC22
  #define HARDWARE_VERSION 220  // 2.20
#else
 #define HARDWARE_VERSION 211  // 2.11
#endif
#else
  #define HARDWARE_VERSION 300  // 3.00
#endif

#define EEPROM_SIZE 	 0x7FFF
#define DAC_RESOLUTION	   1024
#define ADC_RESOLUTION     1024

#ifndef MINI_ICNC2
#ifdef ICNC22
#define AVAILABLE_AOUT        3   // Number of analog output
	#define AVAILABLE_AIN         4   // Number of analog input
	#define AVAILABLE_DOUT       24   // Number of Digital output
	#define AVAILABLE_DIN        31   // Number of Digital input

#else

	#define AVAILABLE_AOUT        2   // Number of analog output
	#define AVAILABLE_AIN         4   // Number of analog input
	#define AVAILABLE_DOUT       24   // Number of Digital output
	#define AVAILABLE_DIN        31   // Number of Digital input
#endif
#else
	#define AVAILABLE_AOUT        2   // Number of analog output
	#define AVAILABLE_AIN         2   // Number of analog input
	#define AVAILABLE_DOUT       4   // Number of Digital output
	#define AVAILABLE_DIN        10   // Number of Digital input
#endif

#define FMAXL 200000			  // Maximum pulse frequency 


#if USE_DFM_COUNT==1
    #define USER_MEM_SIZE        32 // Number of 32bits integer available for user (commandes CMD_WRITE_USER_MEM_BUF, CMD_WRITE_USER_MEM, CMD_READ_USER_MEM)
#else
    #define USER_MEM_SIZE        16 // Number of 32bits integer available for user (commandes CMD_WRITE_USER_MEM_BUF, CMD_WRITE_USER_MEM, CMD_READ_USER_MEM)
#endif


#ifndef PLC_44Ko
// Taille du buffer de commande USB
	#define CMD_BUFFER_SIZE 8192  
#else
	#define CMD_BUFFER_SIZE 1024  
#endif


// Number of tick delay between rampe update
#define RAMPE_UPDATE_PERIODE 2

// Number of tick entre lecture des entrées pour filtrage sur MINI_ICNC2
#define INPUT_UPDATE_PERIODE 5


// Stockage en mémoire EEPROM sur le bus SPI
#define EE_FLASH_SIZE   32768   // Taille total (en octet) mémoire EEPROM
#define EE_PARAMETERS_ADRESS   0    // Adresse stockage des paramètres interpCNC
#define EE_PARAMETERS_SIZE  400*4   // Maximum de 400 paramètres 32 bits        (0 à 1603)
#define EE_LICENCE_KEY_ADRESS 401*4 // Paramètre licence key                    (1608 à 1607)
#define EE_USER_DATA_ADRESS 3128 // base (en octet) de stockage des variables EE_DATA utilisateur
#define EE_USER_DATA_SIZE   1024 // taille (en octet) de stockage des variables EE_DATA utilisateur (3128 à 4151)

#define EE_CNT_SIZE    32
#define EE_CNT1_ADRESS 4160
#define EE_CNT2_ADRESS (EE_CNT1_ADRESS+EE_CNT_SIZE)
#define EE_CNT3_ADRESS (EE_CNT1_ADRESS+(2*EE_CNT_SIZE))
#define EE_CNT4_ADRESS (EE_CNT1_ADRESS+(3*EE_CNT_SIZE))



#define EE_RECETTES_ADRESS  4500    // Adresse de base (en octet) des recettes
#define EE_RECETTE_SIZE_ADRESS  EE_RECETTES_ADRESS + 2 // Index accès recette
#define EE_RECETTE_INDEX1_ADRESS EE_RECETTES_ADRESS + 4 // Index accès recette page 1
#define EE_RECETTE_INDEX2_ADRESS EE_RECETTES_ADRESS + 6 // Index accès recette page 2
#define EE_RECETTE_INDEX3_ADRESS EE_RECETTES_ADRESS + 8 // Index accès recette page 3
#define EE_RECETTE_INDEX4_ADRESS EE_RECETTES_ADRESS + 10 // Index accès recette page 4
#define EE_RECETTE_DATA_ADRESS  EE_RECETTES_ADRESS + 12   // Debut des données de recette 
#define EE_RECETTES_SIZE    5000 * 2    // taille reservée aux recettes (5000 registres 16 bits) (4504 à 14503)
#define EE_RECETTE_DATA_ADRESS_MAX  (EE_RECETTE_DATA_ADRESS + EE_RECETTES_SIZE - 1)      // taille reservée aux recettes (5000 registres 16 bits) (4504 à 14503)







#ifndef MINI_ICNC2
#ifdef ICNC22
#define NB_AXES 5
#else
#define NB_AXES 5
#endif
#else
#define NB_AXES 3
#endif



#endif
