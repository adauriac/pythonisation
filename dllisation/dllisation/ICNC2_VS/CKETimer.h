 // CKETimer.h: interface for the CKETimer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CKETIMER_H__36F00088_AEAA_488C_A619_C375238A6BA5__INCLUDED_)
#define AFX_CKETIMER_H__36F00088_AEAA_488C_A619_C375238A6BA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <mmsystem.h>

class CKETimer  
{
public:
	/// <summary>
	/// CKETimer Constructor
	/// </summary>
	CKETimer();
	virtual ~CKETimer();

	bool     InitTimer(bool bAccurate = true);	// Initialisation of the timer
	LONGLONG GetTotalElapsedTime();				// Elapsed time since start
	LONGLONG GetElapsedTime();					// Elapsed time since last call
	
	void     SetSleepMarker();					// Set a marker, from where the sleeper counts
	LONGLONG GetTimeFromMarker();				// How long ago was the marker set

	void     SleepFromMarker(LONGLONG llSleepValMiliSec);	// Counted from set marker
	void     SleepNow(LONGLONG llSleepValMiliSec);				// Ordinary Sleep
	
	// some test funcs
	LONGLONG CheckFreq();		// See if frequency is stable
	bool     TimerRollOver();	// See if the variable rolls over


protected:
	LONGLONG m_Freq;			// Dependant from clock pulse of the processor
								// The frequenty/divider of a P4 2.4GHz is 3.579.545

	LONGLONG m_StartTime;		// Stored in native value, when was the timerfunc Start() called
	LONGLONG m_LastTime;		// Stored in native value, last time the time was requested
	LONGLONG m_MarkerTime;		// Stored in native value, Start of the total cyclus period
	LONGLONG m_SleepStartTime;	// Stored in native value, Start of the real sleep
	bool     m_bSleepRun;		// Is a sleep busy? And also a way to break an endless sleep
	bool     m_bAccurate;		// Because a Sleep() needs always about 8 miliseconds, we
								// replace the last approaching wait-cycles with a dirty while loop.
								// This generates more proccessor usage. When you want to avoid this
								// you can init the function with bAccurate false
};

#endif // !defined(AFX_CKETIMER_H__36F00088_AEAA_488C_A619_C375238A6BA5__INCLUDED_)
