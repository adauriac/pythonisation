#ifndef __nfTimer_H__
#define __nfTimer_H__
#ifdef _WIN32
#include <MMSystem.h>
#include <timeapi.h>

class nfTimer
{
public:
    // ctors and initializers
    // ----------------------

    // default: if you don't call SetOwner(), your only chance to get timer
    // notifications is to override Notify() in the derived class
    nfTimer()
    {
        Init();
    }

    virtual ~nfTimer();


    // working with the timer
    // ----------------------

    // NB: Start() and Stop() are not supposed to be overridden, they are only
    //     virtual for historical reasons, only Notify() can be overridden

    // start the timer: if milliseconds == -1, use the same value as for the
    // last Start()
    //
    // it is now valid to call Start() multiple times: this just restarts the
    // timer if it is already running
    virtual bool Start(int milliseconds = -1, bool oneShot = false);

    // start the timer for one iteration only, this is just a simple wrapper
    // for Start()
    bool StartOnce(int milliseconds = -1) { return Start(milliseconds, true); }

    // stop the timer, does nothing if the timer is not running
    virtual void Stop(bool kill = true);

    // override this in your _Timer-derived class if you want to process timer
    // messages in it, use non default ctor or SetOwner() otherwise
    virtual void Notify();


    // accessors
    // ---------

    // return true if the timer is running
    bool IsRunning() const;
    void SetRunning(bool running = true) { m_isRunning = running; }

    // return the timer ID
    int GetId() const;

    // get the (last) timer interval in milliseconds
    int GetInterval() const;

    // return true if the timer is one shot
    bool IsOneShot() const;

	inline bool InCallback(void) { return(m_inCallback); }
	inline void SetInCallback(bool inCallback) { m_inCallback = inCallback; }

protected:
    // common part of all ctors
    void Init();
	UINT        m_uResolution{ 0 };
	UINT        m_TimerPeriod{ 0 };
	MMRESULT    m_nfTimer{ 0 };
	bool        m_oneShot{ false };
	bool        m_isRunning{ false };
	bool        m_inCallback{ false };
	int         m_previousTimePeriod{ 0 };
};
#else // _WIN32
#define nfTimer _Timer
#endif // _WIN32
#endif // __nfTimer_H__