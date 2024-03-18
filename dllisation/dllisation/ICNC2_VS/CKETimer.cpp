// CKETimer.cpp: implementation of the CKETimer class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CKETimer.h"
#include "assert.h"
/**************************************************************************************************
This class supplies an accurate timer (miliseconds).
The need of this class lies in the fact that I needed to do an action exactly every second.
Th action has some I/O and can take a considerable time.
Whith a Sleep(1000) after the action, the complete while takes more then the desired 1000 miliseconds

A further problem was the accuracy of a Sleep. This is not high.
A Sleep of 300ms can take 310ms. a Sleep of 1ms takes at least 8ms.
To solve this, I used a dirty for loop. Because this generates a big CPU usage, I only used this at
the very end of the waiting time. In the NT-performance meter you cannot see any difference.
When you dont' need to use this accuracy, just InitTimer(false) will avoid this for loop, to the 
disadvantage of the accuracy.

The best way for a good timer is the QueryPerformanceCounter()
It gives clockpulses. Because this is hardware dependant, we need to know the frequenty (How many
pulses per second). This is achieved with QueryPerformanceFrequency().
QueryPerformanceCounter() works with a struct LARGE_INTEGER, which has some disatvantages when
calculating. It's no problem to use a LONGLONG (__int64) and cast it. This saves us form a lot of
casting and time-consuming member accesses.

Because my app runs lon times, I looked to the maximum time it could run:
We store in a __int64 and it's maximum val is 9.22337E+18 (=2^63)
So this is the maximuo period we can sleep.

The frequenty/divider of a P4-2.4GHz is 3.579.545

this means we can store maximum:-------------
	 2,576,688,388,288 	seconds
	    42,944,806,471 	minutes
	       715,746,775 	hours
	        29,822,782 	days
	            81,706 	years
This is enough for my humble programm...

Internal variables are all saved as clockpulses, not in miliseconds. all user input/output
is done in miliseconds and translated.


The function SleepNow() acts just as an ordinary Sleep() but when you name it the same
there is an anwanted case of recursive programming when you just need ten original Sleep()
**************************************************************************************************/


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



CKETimer::CKETimer()
{
	m_bSleepRun = false;
}

CKETimer::~CKETimer()
{
}

/**************************************************************************************************
Name:
	Function: InitTimer(bool bAccurate = true)

Description:
	Because a Sleep() needs always about 8 miliseconds, we
	replace the last approaching wait-cycles with a dirty while loop.
	This generates more proccessor usage. When you want to avoid this
	you can init the function with bAccurate false

Returns:
	true when OK, false when not.

Usage:
	Once after creation instance

Restrictions:
	None

**************************************************************************************************/
bool CKETimer::InitTimer(bool bAccurate)
{
	m_bAccurate = bAccurate;
    if (QueryPerformanceFrequency((LARGE_INTEGER*) &m_Freq))
    {
        QueryPerformanceCounter((LARGE_INTEGER*) &m_StartTime);	// Remember when started
        m_LastTime = m_StartTime;								// Reset last call
		return true;
    }
	return false;
}

/**************************************************************************************************
Name:
	Function: SetSleepMarker()

Description:
	Set a marker. A call of SleepFromMarker() will measure from this point

Returns:
	nothing.

Usage:
	Use it to mark a point from where you want to count your sleeping period

Restrictions:
	None

**************************************************************************************************/
void CKETimer::SetSleepMarker()
{
	assert(m_Freq);
	QueryPerformanceCounter((LARGE_INTEGER*) &m_MarkerTime);	// Set the marker
	m_bSleepRun = true;
}


/**************************************************************************************************
Name:
	Function: SleepFromMarker(LONGLONG llSleepValMiliSec)

Description:
	How long do you want to sleep, counted from the moment you did set the SleepMarker.
	If you didn't set the sleepmarker, it is set at start of this func

Returns:
	nothing.

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
void CKETimer::SleepFromMarker(LONGLONG llSleepValMiliSec)
{
	// This function doesnt' sleep just llSleepValMiliSec, but starts from the moment m_MarkerTime was set.
	// If m_MarkerTime was not set, then we set it now and it functions just as an ordinary sleep

	if (!m_MarkerTime)
	{
		m_bSleepRun = true;
		QueryPerformanceCounter((LARGE_INTEGER*) &m_MarkerTime);
	}

	m_SleepStartTime = m_MarkerTime;
	
	// Now do the Sleep
	SleepNow(llSleepValMiliSec);
}

/**************************************************************************************************
Name:
	Function: Sleep(LONGLONG llSleepValMiliSec)

Description:
	Almost ordinary Sleep, only more accurate, and can handle the marker

Returns:
	nothing.

Usage:
	Use as an ordinary Sleep()

Restrictions:
	None

**************************************************************************************************/
void CKETimer::SleepNow(LONGLONG llSleepValMiliSec)
{
	assert(m_Freq);
	LONGLONG llNow, llRest;

	if (!m_SleepStartTime)
	{
		QueryPerformanceCounter((LARGE_INTEGER*) &m_SleepStartTime);
		m_bSleepRun = true;
	}

	QueryPerformanceCounter((LARGE_INTEGER*) &llNow);

	// llDiff is in miliseconds!!
	llRest = llSleepValMiliSec - ((llNow -  m_SleepStartTime) * 1000 / m_Freq);

/*	TRACE(_T("Init llNow %I64d  SleepStart %I64d Freq Rest %I64d\n"),
				llNow*1000/m_Freq,
				m_SleepStartTime*1000/m_Freq,
				llRest);
*/
	int iSleep = 200;

	while ( llRest > 0)
	{
		if (TimerRollOver() || !m_bSleepRun)	// Emergancy break
			break;
		
		//TRACE(_T("Rest IN %I64d\n"), llRest );
		// differential sleep. When approaching the target we takes more samples
		if      (llRest > 500)
			iSleep = 200;
		else if (llRest <= 500 && llRest > 15 )
			iSleep = (int)(llRest - 15);
		else if (llRest <= 15 )
			iSleep = m_bAccurate? -1 : 10;


		//TRACE(_T("Sleep for %d ms \n"), iSleep );

		// Sleep uses always about 8 miliseconds, so when we are close to our goal
		// we stop using sleep and use a stupid for loop, which takes about 1 milisecond
		// By depending it on m_Freq the routine is hardwaredependant (at least I hope...)
		if (iSleep > 0)
			Sleep(iSleep);
		else
		{
			for (int i = 0; i<(m_Freq / 400) ;i++)
			{
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*) &llNow);
		llRest = llSleepValMiliSec - ((llNow -  m_SleepStartTime) * 1000 / m_Freq);
	}

	//TRACE(_T("Rest NA %I64d\n"), llRest );

	m_bSleepRun = false;
	m_SleepStartTime = 0;	// Set back for next sleep
}

/**************************************************************************************************
Name:
	Function: GetTotalElapsedTime()

Description:
	Get the time from the moment of the initialisation of the timer till now

Returns:
	time in miliseconds.

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
LONGLONG CKETimer::GetTotalElapsedTime()
{
	LONGLONG tmElapsed;
	LONGLONG tmCurrent;
	QueryPerformanceCounter((LARGE_INTEGER*) &tmCurrent);	// What's the time now?
	
	tmElapsed = ((tmCurrent - m_StartTime) * 1000)  / m_Freq;	// Time since start
	
	m_LastTime = tmCurrent;	// Reset last call
    return tmElapsed;	// Returned in miliseconds
}

/**************************************************************************************************
Name:
	Function: GetTimeFromMarker()

Description:
	Get the time from the moment of setting the marker till now

Returns:
	time in miliseconds.

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
LONGLONG CKETimer::GetTimeFromMarker()
{
	LONGLONG tmElapsed;
	LONGLONG tmCurrent;
	QueryPerformanceCounter((LARGE_INTEGER*) &tmCurrent);	// What's the time now?
	
	tmElapsed = ((tmCurrent - m_MarkerTime) * 1000)  / m_Freq;	// Time since start
	
	m_LastTime = tmCurrent;	// Reset last call
    return tmElapsed;	// Returned in miliseconds
}

/**************************************************************************************************
Name:
	Function: GetElapsedTime()

Description:
	Get the time from the moment of the last request of the timer til now

Returns:
	time in miliseconds.

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
LONGLONG CKETimer::GetElapsedTime()
{
	LONGLONG tmElapsed;
	LONGLONG tmCurrent;
	QueryPerformanceCounter((LARGE_INTEGER*) &tmCurrent);	// What's the time now?

	tmElapsed = ((tmCurrent - m_LastTime) * 1000) / m_Freq;	// Time since last call

	m_LastTime = tmCurrent;	// Reset last call
    return tmElapsed;	// Returned in miliseconds
} 

/**************************************************************************************************
Name:
	Function: CheckFreq()

Description:
	Debugging func. Just to find out the hardware dependant frequenty divider
	

Returns:
	frequenty.

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
LONGLONG CKETimer::CheckFreq()
{
	LONGLONG tmpFreq = -1;
	// Just a test func to test if the freq changes
    QueryPerformanceFrequency((LARGE_INTEGER*) &tmpFreq);

	if (tmpFreq != m_Freq)
	{
		//TRACE(_T("**************Frequenty changed!!!!!!!!! %d ==> %d **************\n"), m_Freq, tmpFreq);
		assert(FALSE);
	}
	return tmpFreq;
}


/**************************************************************************************************
Name:
	Function: TimerRollOver()

Description:
	When you have a very long sleep, it can happen that the timer is reaching the max.
	According to the calculations we'll never reach this, but anyway, we check for it.
	When this happens, we set back to start values and we break an eventual running sleep

Returns:
	false if mo problem, true when rollover occurs

Usage:
	-

Restrictions:
	None

**************************************************************************************************/
bool CKETimer::TimerRollOver() 
{
	// Check if the timer val is too big
	LONGLONG llTime;
    QueryPerformanceCounter((LARGE_INTEGER*) &llTime);
	
    if (m_StartTime > llTime) // check for timer roll-over 
    { 
		// Roll over hapended
        m_StartTime = llTime;
		m_LastTime  = llTime;
		m_bSleepRun = false; // Stop with any sleep
        return true; 
    } 

	return false; 
}
