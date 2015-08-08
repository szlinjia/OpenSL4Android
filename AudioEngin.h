#ifndef _AUDIOENGIN_H_
#define _AUDIOENGIN_H_

#include <assert.h>
#include <jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


#define SAMPLESIZE 22050/2		//0.5 second per sample

#ifdef __cplusplus
extern "C" {
#endif

#define  LOG_TAG    "OpenSL"
#define  Print(...)  //__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#define SLEEP(x)

typedef struct {
		short left;
		short right;
	} stereo;



static float gVolume = 1.0f;
// 0.5 second of stereo audio at 22.05 kHz
extern short *stereoBuffer2;
class AudioEvenCallback
{
public:
	virtual void PlayEqueueCallback () = 0;
};

class CAudioEngin
{
public:
	CAudioEngin ();
	~CAudioEngin ();
	void CreateEngine ();
	void createBufferQueueAudioPlayer ();
	void Play ();
	void Stop ();
	bool getPlayState ();
	void Reset ();
	void setCallBackEven (AudioEvenCallback *peven);

	static void bqPlayerCallback (SLAndroidSimpleBufferQueueItf bq, void *context);
	void clear ();
	void RegisterEvent (AudioEvenCallback *pevent);
	void UnRegisterEvent ();
	void Shutdown ();

private:
	// engine interfaces
	SLObjectItf engineObject;
	SLEngineItf engineEngine;

	// output mix interfaces
	SLObjectItf outputMixObject;

	// buffer queue player interfaces
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLEffectSendItf bqPlayerEffectSend;
	SLVolumeItf bqPlayerVolume;
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
	
	bool m_isPlay;
	AAsset* m_pcmAsset;
	unsigned int m_totalSize;
	unsigned int m_readSize;
	AudioEvenCallback *m_pevent;
};

#ifdef __cplusplus
}
#endif

#endif