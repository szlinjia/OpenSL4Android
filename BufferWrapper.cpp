/*=======================================================================
class name	: BufferWrapper
Author		: Li Lin
Date		: 03/03/2015
Describe	: This class provides music reading service from asset to buffer.
Two buffers alternative play and read.Each time read 1 second sample to buffer.
There are two thread for working. Main thread is playing music, and the working
thread is reading and analyzing beat. Main thread reads ring buffer, working
thread writes ring buffer.
========================================================================*/

#include "BufferWrapper.h"
#include <math.h>
#include <unistd.h>
#include "RingBuffer.h"
#include <stdlib.h>

static short m_pcm[SAMPLESIZE * 2];


static CBufferWrapper g_bufferwrapper;
static pthread_rwlock_t lock;
/*
 *	@function: JNI interface
 *  @input	:  JNI parameters: env, obj, assetManager, filename
 *  @output	:  void
 *  @describe: call from java function to play music
 */
JNIEXPORT void JNICALL Java_com_example_nativeaudio_NativeAudio_createEngine
(JNIEnv* env, jobject obj, jobject assetManager, jstring filename){

	Print ("JNI Start to create Engine;");
	g_bufferwrapper.setEnviroment (env, obj, assetManager, filename);
	g_bufferwrapper.CreateThread ();
	
  }

/*
 *	@function: JNI interface
 *  @input	 : JNI parameters: env, obj
 *  @output	 : void
 *  @describe: call from java to stop playing music
 */
JNIEXPORT void JNICALL Java_com_example_nativeaudio_NativeAudio_stopPlaying
(JNIEnv* env, jobject obj)
{
	g_bufferwrapper.m_totalSize = -1;
	Print ("StopPlaying g_bufferwrapper.m_totalSize:%d %d", g_bufferwrapper.m_totalSize, g_bufferwrapper.m_readSize);
	int ret = pthread_join (g_bufferwrapper.th_w, NULL);
	Print ("Write thread exit.");

	g_bufferwrapper.m_isTerminate = true;
	int ret1 = pthread_join (g_bufferwrapper.th_r, NULL);
	Print ("Read thread exit.");

}

JNIEnv* CBufferWrapper::m_env = NULL;
jobject CBufferWrapper::m_obj = NULL;
JavaVM* CBufferWrapper::m_jvm = NULL;
CBufferWrapper::CBufferWrapper()
{
	m_pcmAsset = NULL;
	m_totalSize = 0;
	m_readSize = 0;
	m_playSize = 0;
	m_curBeatTime = 0.5;
	m_curPlayTime = 0.0;

	pthread_rwlock_init (&lock, NULL);
}

CBufferWrapper::~CBufferWrapper()
{
	pthread_rwlock_destroy (&lock);
	Print ("CBufferWrapper destructor");
}

/*
 *	@function:
 *  @input	 :
 *  @output	 :
 *  @describe:
 */
void CBufferWrapper::Reset ()
{
	Print ("[CBufferWrapper] Reset is called");
	if (m_pcmAsset != NULL)
	{
		AAsset_close (m_pcmAsset);
		m_pcmAsset = NULL;
	}

	m_readSize = 0;
	if (m_isTerminate)
	{
		m_playSize = 0;
		m_curPlayTime = 0;
		m_isEngineInit = false;
		m_totalSize = 0;
		pthread_rwlock_destroy (&rwlock);
	}
}
/*
 *	@function: setEnviroment
 *  @input	: JNIEnv* env, jobject obj, jobject assetManager, jstring filename
 *  @output	: void
 *  @describe: Set initial java enviroment to class. Then open music file, which is located in asset
 */
void CBufferWrapper::setEnviroment (JNIEnv* env, jobject obj, jobject assetManager, jstring filename)
{

	m_env = env;
	m_obj = m_env->NewGlobalRef (obj);
	m_env->GetJavaVM (&m_jvm);
	const char *utf8 = env->GetStringUTFChars (filename, NULL);
	AAssetManager* mgr = AAssetManager_fromJava (env, assetManager);
	assert (NULL != mgr);

	if (m_pcmAsset == NULL)
	{
		Print ("Open music file %s", utf8);
		m_pcmAsset = AAssetManager_open (mgr, utf8, AASSET_MODE_UNKNOWN);
		// the asset might not be found
		if (m_pcmAsset == NULL)
		{
			Print ("Error:open file failed.");
		}
		assert (m_pcmAsset != NULL);
		m_totalSize = AAsset_getLength (m_pcmAsset);
		m_readSize = 0;
		Print ("ConvertMp3toPCM:size=%d", m_totalSize);
	}

	env->ReleaseStringUTFChars (filename, utf8);
}

/*
 *	@Function: CallBacktoJava
 *  @input	: void		
 *  @output	: void
 *  @describe: This function is used to notify Java code that a beat is detected;
 */
void CBufferWrapper::CallBacktoJava (int isOver)
{
	JNIEnv *env;
	Print ("Notifiy a Beat Detection");
	if (m_jvm->AttachCurrentThread (&env, NULL) != JNI_OK)
	{
		Print ("Attach evn error.");
		return;
	}

	jclass clazz = env->GetObjectClass (m_obj);
	if (clazz == 0) {
		Print ("FindClass error");
		return;
	}

	jmethodID messageMe = env->GetStaticMethodID (clazz, "messageMe", "(I)V");
	env->CallStaticVoidMethod (clazz, messageMe,isOver);

	if (m_jvm->DetachCurrentThread () != JNI_OK)
	{
		Print ("DetachCurrentThread() failed");
	}
	
}

/*
 *	@function: EnginInite
 *  @input	: void
 *  @output	: void
 *  @describe: Create Audio Engine. This function can call multiple times. 
 */
void CBufferWrapper::EnginInite ()
{
	Print ("[CBufferWrapper::EnginInite]");
	m_cEngine.CreateEngine ();
	m_cEngine.createBufferQueueAudioPlayer ();
	m_cEngine.RegisterEvent (this);
}

/*
 *	@function: ConvertMp3toPCM
 *  @input	: void
 *  @output	: void
 *  @describe:Each time read one second sample to buffer and send it to audio mixer for playing. when it read to the end
 *            of file, it stops playing;
 */
void CBufferWrapper::ConvertMp3toPCM ()
{
	RingBuffer* ring = RingBuffer::getRingBuffer ();

	if (m_readSize < m_totalSize)
	{
		Print ("[ConvertMp3toPCM]Read File:%d total:%d",m_readSize, m_totalSize);
		unsigned int remaind = m_totalSize - m_readSize;
		unsigned int len = remaind < SAMPLESIZE*sizeof(stereo) ? remaind : SAMPLESIZE*sizeof(stereo);
		memset (m_pcm, 0, sizeof(m_pcm));
		AAsset_read (m_pcmAsset, m_pcm, len);
		m_readSize += len;
		if (!pthread_rwlock_rdlock (&rwlock))
		{
			ring->WriteBuffer (m_pcm, len);
			pthread_rwlock_unlock (&rwlock);
		}

	}
	else
	{
		//Print ("Buffer read over");
	}
}

/*
 *	@function: PlayEqueueCallback
 *  @input	 : void 
 *  @output	 : void
 *  @describe: This callback function is called by AudionEndgin class when
		play over one buffer in queue.
 */
void CBufferWrapper::PlayEqueueCallback ()
{
	Print ("PlayEqueueCallback:update buffer...");
	RingBuffer* ring = RingBuffer::getRingBuffer ();
	if (m_playSize >= m_totalSize)
	{
		Print ("[PlayEqueueCallback]Play over.stop playing...");
		m_isTerminate = true;
		m_cEngine.UnRegisterEvent ();
		Reset ();
		CallBacktoJava (1);
		return;
	}

	
	ring->IncreaseReadIndex ();

	m_curPlayTime += 0.5;
	CallBacktoJava (0);

	if (!pthread_rwlock_rdlock (&rwlock))
	{
		ring->ReadBuffer(&stereoBuffer2);
		pthread_rwlock_unlock (&rwlock);
	}
	m_playSize += SAMPLESIZE * sizeof(stereo);
}

/*
 *	@function: CreateThread
 *  @input	 : void
 *  @output	 : void
 *  @describe: Create a working thread to read and anaynize PCM data.
 */
void CBufferWrapper::CreateThread ()
{
	
	int Num;

	Print ("Create Thread");
	pthread_rwlock_init (&rwlock, NULL);

	RingBuffer *ring = RingBuffer::getRingBuffer ();
	ring->Reset ();
	int ret = pthread_create (&th_w, NULL, WriteThread,
		(void*)this);

	int ret1 = pthread_create (&th_r, NULL, ReadThread,
		(void*)this);

	

}

/*
 *	@function: run
 *  @input	 : a void pointer of parameter.
 *  @output	 : a void pointer
 *  @describe: running body of working thread. Will ternimate automatically 
 *		after read over PCM data
 */
void *CBufferWrapper::WriteThread (void *parameter)
{
	CBufferWrapper *bufwrapper = (CBufferWrapper*)parameter;

	Print ("Start to run write thread.%d %d.", bufwrapper->m_readSize, bufwrapper->m_totalSize);

	while (bufwrapper->m_readSize < bufwrapper->m_totalSize)
	{

		bufwrapper->ConvertMp3toPCM ();
		usleep (1000);
		while (!bufwrapper->m_isEngineInit)
		{
			usleep (5000);
			//Print ("Wait for engin init...");
		}
	}
	
	bufwrapper->Reset ();
	Print ("Write Thread body exit");
	
	return NULL;
}

void *CBufferWrapper::ReadThread (void *parameter)
{
	Print ("Start to run read thread.");

	CBufferWrapper *obj = (CBufferWrapper*)parameter;
	obj->m_isEngineInit = false;
	obj->EnginInite ();
	obj->m_isTerminate = false;
	obj->m_isEngineInit = true;


	//at first, wait for writting thread write buffer...
	RingBuffer* ring = RingBuffer::getRingBuffer ();

	//wait for write thread to read first buffer to play.
	if (ring->getWIndex () == 0 && ring->getRIndex() == -1)
	{
		while (ring->getWIndex () == 0)
		{
			usleep (100);
		}
	}
	Print ("First read ring buffer and play...");
	ring->ReadBuffer (&stereoBuffer2);
	obj->m_playSize += SAMPLESIZE*sizeof(stereo);
	obj->m_cEngine.Play ();
	obj->CallBacktoJava (0);
	while (!obj->m_isTerminate)
	{
		usleep(100);
	}

	obj->m_cEngine.Stop ();
	obj->m_cEngine.Shutdown ();
	obj->Reset ();
	
	Print ("Read Thread body exit");
	return NULL;
}