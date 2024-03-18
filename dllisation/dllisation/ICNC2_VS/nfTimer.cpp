// Multimedia Timmer wrapper for windows.  Wraps plain _Timer for Linux and Mac
// Steve Murphree, 2013

#include "stdafx.h"
#include "nfTimer.h"
#ifdef _WIN32

#define nfUnusedVar(v)

void CALLBACK TimerProc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	nfUnusedVar(dw1);
	nfUnusedVar(dw2);
	nfUnusedVar(msg);
	nfUnusedVar(wTimerID);
	nfTimer* obj = reinterpret_cast<nfTimer *>(dwUser);
		obj->SetInCallback(true);
		if (obj->IsOneShot()) {
			obj->Stop(false);
			//obj->SetRunning(false);
		}
		obj->Notify();
		obj->SetInCallback(false);
}

void nfTimer::Init(void)
{
	m_oneShot = false;
	m_uResolution = 0;
	m_TimerPeriod = 0;
	m_isRunning = false;
	m_nfTimer = NULL;
	m_inCallback = false;
	m_previousTimePeriod = 0;
}

nfTimer::~nfTimer(void)
{
	if (m_isRunning) {
		Stop();
		while (m_inCallback) {
			::Sleep(2);
		}
	}
}

bool nfTimer::Start(int ms, bool oneShot)
{
	m_oneShot = oneShot;
	//
	// Initialize the Multimedia Timer
	TIMECAPS tc;
	UINT     timerType = m_oneShot ? TIME_ONESHOT : TIME_PERIODIC;
	timerType |= TIME_KILL_SYNCHRONOUS | TIME_CALLBACK_FUNCTION;
	if (ms == -1) {
		ms = m_previousTimePeriod;
	}

	m_previousTimePeriod = ms;

	if ( timeGetDevCaps(&tc, (UINT)sizeof(TIMECAPS)) == TIMERR_NOERROR ) {
		m_uResolution = min( max( tc.wPeriodMin, 0 ), tc.wPeriodMax );
		m_TimerPeriod = max(m_uResolution, (UINT)ms);  // Elapsed time between timer callbacks

		if ( timeBeginPeriod( m_uResolution ) == TIMERR_NOERROR ) {
			// create the timer
			m_nfTimer = timeSetEvent(m_TimerPeriod, m_uResolution, TimerProc, (DWORD)this, timerType);
			if (!m_nfTimer) {
				m_isRunning = false;
				return(false);
			}
		} else { 
			m_isRunning = false;
			return(false);
		}
	} else { 
		m_isRunning = false;
		return(false);
	}
	m_isRunning = true;
	return(true);
}

// When killing off a one-shot timer we try to let it finish the current
// cycle which 'should' also have already stopped it. If it is still
// ruinning (m_isRunning == true) we go ahead and issue a timeEnd Period.
// The docs seem to suggest that timeBeginPeriod/timeEndPeriod should 
// remain balanced.
// Also, may need timeKillEvent(m_nfTimer); instead for periodic timers.
void nfTimer::Stop(bool kill)
{
	if (kill) {
		if (m_isRunning) {
			timeEndPeriod(m_uResolution);
		} 
	}
	else {
		if (m_isRunning) {
			timeEndPeriod(m_uResolution);
		}
	}

	m_nfTimer = NULL;
	m_isRunning = false;
}

void nfTimer::Notify(void)
{
	// Needs to be overriden.
}

// return true if the timer is running
bool nfTimer::IsRunning(void) const
{
	return(m_isRunning);
}

// return the timer ID
int nfTimer::GetId(void) const
{
	return((int)m_nfTimer);
}

// get the (last) timer interval in milliseconds
int nfTimer::GetInterval(void) const
{
	return((int)m_TimerPeriod);
}

// return true if the timer is one shot
bool nfTimer::IsOneShot(void) const
{
	return(m_oneShot);
}

#endif _WIN32