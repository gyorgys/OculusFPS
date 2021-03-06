#include "OculusPerfInfo.h"
#include <assert.h>
#include <time.h>

int rand_int(int n) {
	return rand() / ((RAND_MAX + 1u) / n);
}

OculusPerfInfo::OculusPerfInfo()
{
	this->m_pfiLastPerfInfo.fHeadsetActive = false;
}

bool OculusPerfInfo::init()
{
	ovrInitParams initParams = { ovrInit_Invisible | ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	if (!OVR_SUCCESS(result)) {
		return false;
	}
	this->m_fInitialized = true;
	this->m_thread = std::thread(&OculusPerfInfo::collectPerfData, this);

	return true;
}

bool OculusPerfInfo::initTestMode()
{
	srand((UINT)time(NULL));
	this->m_fInitialized = true;
	this->m_fHmdRefreshRate = 90.0;
	this->m_thread = std::thread(&OculusPerfInfo::collectTestPerfData, this);

	return true;
}

void OculusPerfInfo::setCallback(void(*callback)(opi_PerfInfo&))
{
	this->m_pfCallback = callback;
}

opi_PerfInfo OculusPerfInfo::getPerfInfo()
{
	return this->m_pfiLastPerfInfo;
}

OculusPerfInfo::~OculusPerfInfo()
{
	this->destroyOvrSession();
	this->m_fStop = true;
	if (this->m_thread.joinable()) {
		this->m_thread.join();
	}
}

bool OculusPerfInfo::createOvrSession()
{
	this->destroyOvrSession();
	ovrSession session;
	ovrGraphicsLuid luid;
	ovrResult result = ovr_Create(&session, &luid);
	if (!OVR_SUCCESS(result)) {
		return false;
	}

	this->m_session = session;
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	this->m_fHmdRefreshRate = hmdDesc.DisplayRefreshRate;
	return true;
}

void OculusPerfInfo::destroyOvrSession()
{
	if (this->m_session != NULL) {
		ovr_Destroy(this->m_session);
		this->m_session = NULL;
	}
}

void OculusPerfInfo::collectPerfData()
{
	assert(this->m_fInitialized);
	auto sleepTime = std::chrono::milliseconds(3000);

	ovrPerfStats lastPerfStats;
	ovrPerfStatsPerCompositorFrame& lastFrameStats = lastPerfStats.FrameStats[0]; // [0] contains the most recent stats

	ovrPerfStats perfStats;
	bool waitingForStatsMessaged = false;

	while (!this->m_fStop) {
		if (this->m_session == NULL) {
			if (this->createOvrSession())
			{
				sleepTime = std::chrono::milliseconds(1000);
				this->m_pfiLastPerfInfo.fHeadsetActive = true;
				lastFrameStats.HmdVsyncIndex = -1;  // reset to make sure we know it's not updated via the SDK yet
				lastPerfStats.VisibleProcessId = -1;
			}
			else {
				sleepTime = std::chrono::milliseconds(3000);
				this->m_pfiLastPerfInfo.fHeadsetActive = false;
			}
		}
		else {
			assert(this->m_session != NULL);
			ovrSessionStatus sessionStatus;
			ovr_GetSessionStatus(this->m_session, &sessionStatus);
			if (sessionStatus.ShouldQuit)
			{
				break;
			}
			if (sessionStatus.DisplayLost || !sessionStatus.HmdPresent || !sessionStatus.HmdMounted)
			{
				sleepTime = std::chrono::milliseconds(1000);
				this->m_pfiLastPerfInfo.fHeadsetActive = false;
			}
			else {
				if (this->m_pfiLastPerfInfo.fHeadsetActive == false) {
					if (!this->createOvrSession()) {
						continue;
					}
					this->m_pfiLastPerfInfo.fHeadsetActive = true;
				}

				sleepTime = std::chrono::milliseconds((int)(1000 / m_fHmdRefreshRate * 15));
				ovr_GetPerfStats(this->m_session, &perfStats);

				// Did we get any valid stats?
				if (perfStats.FrameStatsCount > 0)
				{
					// Did we process a frame before?
					if (lastFrameStats.HmdVsyncIndex > 0)
					{
						// Are we still looking at the same app, or did focus shift?
						if (lastPerfStats.VisibleProcessId == perfStats.VisibleProcessId)
						{
							ovrPerfStatsPerCompositorFrame& frameStats = perfStats.FrameStats[0];
							int framesSinceLastInterval = frameStats.HmdVsyncIndex - lastFrameStats.HmdVsyncIndex;

							int appFramesSinceLastInterval = frameStats.AppFrameIndex - lastFrameStats.AppFrameIndex;
							int compFramesSinceLastInterval = frameStats.CompositorFrameIndex - lastFrameStats.CompositorFrameIndex;

							float appFrameRate = (float)appFramesSinceLastInterval / framesSinceLastInterval * this->m_fHmdRefreshRate;
							float compFrameRate = (float)compFramesSinceLastInterval / framesSinceLastInterval * this->m_fHmdRefreshRate;

							this->m_pfiLastPerfInfo.fHeadsetActive = true;
							this->m_pfiLastPerfInfo.nAppFps = (int)round(appFrameRate);
							this->m_pfiLastPerfInfo.nCompFps = (int)round(compFrameRate);
							this->m_pfiLastPerfInfo.nLatencyMs = (int)round(perfStats.FrameStats[0].AppMotionToPhotonLatency * 1000.0);
							this->m_pfiLastPerfInfo.fAwsActive = perfStats.FrameStats[0].AswIsActive;
							this->m_pfiLastPerfInfo.nDroppedFrames = perfStats.FrameStats[0].AppDroppedFrameCount - lastFrameStats.AppDroppedFrameCount;
						}
						else
						{
							auto result = ovr_ResetPerfStats(this->m_session);
							if (!OVR_SUCCESS(result))
							{
								break;
							}
						}
					}

					// save off values for next interval's calculations
					lastPerfStats = perfStats;
					waitingForStatsMessaged = false;
				}
				else if (!waitingForStatsMessaged)
				{
					// This can happen if there are no VR applications currently in focus, or if the HMD has been
					// static for a certain amount of time, causing the SDK to put the HMD display to sleep.
					waitingForStatsMessaged = true;
					this->m_pfiLastPerfInfo.fHeadsetActive = false;
				}
			}
		}
		if (this->m_pfCallback != NULL) {
			this->m_pfCallback(this->m_pfiLastPerfInfo);
		}
		std::this_thread::sleep_for(sleepTime);
	}
	this->destroyOvrSession();
	ovr_Shutdown();
}

void OculusPerfInfo::collectTestPerfData()
{
	auto sleepTime = std::chrono::milliseconds((int)(1000 / m_fHmdRefreshRate * 15));
	bool fAws = true;
	int nMaxFps = (int)round(this->m_fHmdRefreshRate);
	int nMaxAwsFps = (int)round(this->m_fHmdRefreshRate / 2.0);
	int nPerfDropCount = 0;

	while (!this->m_fStop) {

		int nFpsDrop = 0;
		bool fDropFrames = false;

		if (nPerfDropCount > 0) {
			// continue simulated perf drop
			nPerfDropCount--;
			nFpsDrop = rand_int((int)(round(fAws ? nMaxAwsFps * 0.7 : nMaxFps * 0.7)));
			fDropFrames = rand_int(3) > 1;
		}
		else {
			// figure out next event to simulate
			int nRnd = rand_int(300);

			// Do random stuff
			if (nRnd < 15) {
				// slight fps drop
				nFpsDrop = nRnd;
			}
			else if (nRnd < 20) {
				// long, sever perf drop
				nPerfDropCount = (nRnd - 15) * 25;
			}
			else if (nRnd < 30) {
				// mode switch
				fAws = !fAws;
			}
			else {
				// do nothing
			}
		}
		this->m_pfiLastPerfInfo.fHeadsetActive = true;
		this->m_pfiLastPerfInfo.nAppFps = (fAws ? nMaxAwsFps : nMaxFps) - nFpsDrop;
		this->m_pfiLastPerfInfo.nMaxAppFps = (fAws ? nMaxAwsFps : nMaxFps);
		this->m_pfiLastPerfInfo.nCompFps = 90;
		this->m_pfiLastPerfInfo.nLatencyMs = 24;
		this->m_pfiLastPerfInfo.fAwsActive = fAws;
		this->m_pfiLastPerfInfo.nDroppedFrames = fDropFrames ? 1 : 0;

		if (this->m_pfCallback != NULL) {
			this->m_pfCallback(this->m_pfiLastPerfInfo);
		}

		std::this_thread::sleep_for(sleepTime);
	}
}

