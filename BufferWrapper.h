#ifndef _BUFFER_WRAPPER_H_
#define _BUFFER_WRAPPER_H_

#include <assert.h>
#include <jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <pthread.h>

#include "AudioEngin.h"


#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT void JNICALL Java_com_example_nativeaudio_NativeAudio_createEngine
(JNIEnv* env, jobject obj, jobject assetManager, jstring filename);

JNIEXPORT void JNICALL Java_com_example_nativeaudio_NativeAudio_stopPlaying
(JNIEnv* env, jobject obj);

typedef enum
{
	inite = 1,
	idle,
	reading,
	readover,
	filetoend,
}AnalyzeState;

class CBufferWrapper:
	public AudioEvenCallback
{
public:
    CBufferWrapper();
    ~CBufferWrapper();
	void ConvertMp3toPCM ();
	void EnginInite ();
	void setEnviroment (JNIEnv* env, jobject obj, jobject assetManager, jstring filename);
	static void CallBacktoJava (int isOver);
	void CreateThread ();
	static void *WriteThread (void *parameter);
	static void *ReadThread (void *parameter);
	virtual void PlayEqueueCallback ();
	bool IsTimeMatchBeat ();
	void Reset ();
public:
	CAudioEngin m_cEngine;
	static JNIEnv* m_env;
	static jobject m_obj;
	static JavaVM *m_jvm;
	jobject m_assetManager;
	jstring m_filename;

	AAsset* m_pcmAsset;
	
	int m_totalSize;
	int m_readSize;
	pthread_rwlock_t rwlock;
	bool m_isTerminate;
	unsigned int m_playSize;

	//used for Asset beat.txt
	double m_curBeatTime;
	double m_curPlayTime;
	bool m_isEngineInit;

	pthread_t th_w, th_r;
};

#ifdef __cplusplus
}
#endif

#endif