#pragma once
#include "framework.h"
#include "OVR_CAPI.h"   // Include the Oculus SDK
#include <thread>
#include <cstddef>

struct opi_PerfInfo {
    bool fHeadsetActive;
    int nAppFps;
    int nCompFps;
    int nLatencyMs;
    int nDroppedFrames;
    bool fAwsActive;
};

class OculusPerfInfo
{
private:
    bool m_fInitialized = false;
    ovrSession m_session = NULL;
    std::thread m_thread;
    bool m_fStop = false;
    float m_fHmdRefreshRate = 90.0;
    opi_PerfInfo m_pfiLastPerfInfo;
    HWND m_hWnd = NULL;
    void(*m_pfCallback)(opi_PerfInfo&) = NULL;

public:
    OculusPerfInfo();
    ~OculusPerfInfo();
    bool init();
    void collectPerfData();
    void setCallback(void(*pfCallback)(opi_PerfInfo&));
    opi_PerfInfo getPerfInfo();

protected:
    bool createOvrSession();
    void destroyOvrSession();
};

