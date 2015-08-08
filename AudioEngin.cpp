/*=========================================================================
class name	: CAudioEngin
Author		: Li Lin
Date		: 03/04/2015 created
Describe	: This class encapsulates OpenSL APIs for playing audio in
	Buffer Queue.
=========================================================================*/
#include "AudioEngin.h"
#include <math.h>
#include <unistd.h>
#include "RingBuffer.h"

//stereo stereoBuffer2[SAMPLESIZE] = {0};
//short stereoBuffer2[SAMPLESIZE * 2] = { 0 };
short *stereoBuffer2 = NULL;

CAudioEngin::CAudioEngin ()
{
	m_isPlay = false;
	engineObject = NULL;
	m_pevent = NULL;
	bqPlayerBufferQueue = NULL;
	outputMixObject = NULL;
}

CAudioEngin::~CAudioEngin ()
{
}

void CAudioEngin::Reset ()
{
	
}

/*
 *	@function: CreateEngine
 *  @input	 : void
 *  @output	 : void
 *  @describe: Initialize OpenSL APIs objects, register callback function to OpenSL
 */
void CAudioEngin::CreateEngine ()
{
	SLresult result;

	if (engineObject != NULL) return;

	// create engine
	result = slCreateEngine (&engineObject, 0, NULL, 0, NULL, NULL);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the engine
	result = (*engineObject)->Realize (engineObject, SL_BOOLEAN_FALSE);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface (engineObject, SL_IID_ENGINE, &engineEngine);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean req[1] = { SL_BOOLEAN_FALSE };
	result = (*engineEngine)->CreateOutputMix (engineEngine, &outputMixObject, 1, ids, req);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the output mix
	result = (*outputMixObject)->Realize (outputMixObject, SL_BOOLEAN_FALSE);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;
}

/*
 *	@function: createBufferQueueAudioPlayer
 *  @input	 : void
 *  @output	 : void
 *  @describe: Intialize Audio Player and Buffer Queue objects. This interface must be
	invoked after CreateEngine(); 
 */
void CAudioEngin::createBufferQueueAudioPlayer ()
{
	SLresult result;

	if (bqPlayerObject != NULL) return;

	Print ("Start to create buffer queue audio player.");
	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_22_05,
		SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
	SLDataSource audioSrc = { &loc_bufq, &format_pcm };

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
	SLDataSink audioSnk = { &loc_outmix, NULL };

	// create audio player
	const SLInterfaceID ids[3] = { SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
		/*SL_IID_MUTESOLO,*/ SL_IID_VOLUME };
	const SLboolean req[3] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
		/*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE };
	result = (*engineEngine)->CreateAudioPlayer (engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
		3, ids, req);

	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the player
	result = (*bqPlayerObject)->Realize (bqPlayerObject, SL_BOOLEAN_FALSE);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// get the play interface
	result = (*bqPlayerObject)->GetInterface (bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;

	// get the buffer queue interface
	result = (*bqPlayerObject)->GetInterface (bqPlayerObject, SL_IID_BUFFERQUEUE,
		&bqPlayerBufferQueue);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;
	// register callback on the buffer queue
	
	result = (*bqPlayerBufferQueue)->RegisterCallback (bqPlayerBufferQueue, bqPlayerCallback, this);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;


	// get the volume interface
	result = (*bqPlayerObject)->GetInterface (bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;


	// verify that player is initially stopped
	SLuint32 playerState;
	result = (*bqPlayerPlay)->GetPlayState (bqPlayerPlay, &playerState);
	if (SL_PLAYSTATE_STOPPED != playerState)
	{
		result = (*bqPlayerPlay)->SetPlayState (bqPlayerPlay, SL_PLAYSTATE_STOPPED);
		Print ("Set state stopped manually.");
	}

	// set the player's state to playing
	result = (*bqPlayerPlay)->SetPlayState (bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	assert (SL_RESULT_SUCCESS == result);
	(void)result;
}


/*
 *	@function: getPlayState
 *  @input	 : void
 *  @output	 : true if player is playing, otherwise false.
 *  @describe: return current audio player's state.
 */
bool CAudioEngin::getPlayState ()
{
	return m_isPlay;
}

void CAudioEngin::Stop ()
{
	Print ("[CAudioEngin::Stop]try to stop player");
	if (bqPlayerObject == NULL) return;

	SLuint32 res;
	SLuint32 playerState;
	
	m_isPlay = false;
	res = (*bqPlayerBufferQueue)->Clear (bqPlayerBufferQueue);
	res = (*bqPlayerPlay)->GetPlayState (bqPlayerPlay, &playerState);
	Print ("Stop play state=%d", playerState);
	if (playerState == SL_PLAYSTATE_PLAYING)
	{
		(*bqPlayerPlay)->SetPlayState (bqPlayerPlay, SL_PLAYSTATE_STOPPED);
	}
}

/*
 *	@function: Play
 *  @input	 : void.
 *  @output	 : void
 *  @describe: Play a buffer audio. 
 */
void CAudioEngin::Play ()
{
	Print ("[CAudioEngin::Play]Play buffer Queue");
	SLuint32 res;
	SLuint32 playerState;

	clear ();
	// enqueue a buffer
	res = (*bqPlayerBufferQueue)->Enqueue (bqPlayerBufferQueue, stereoBuffer2,
		SAMPLESIZE*sizeof(stereo)); 

	// state should be playing immediately after enqueue
	res = (*bqPlayerPlay)->GetPlayState (bqPlayerPlay, &playerState);
	if (playerState != SL_PLAYSTATE_PLAYING)
	{
		// set play state to playing
		res = (*bqPlayerPlay)->SetPlayState (bqPlayerPlay, SL_PLAYSTATE_PLAYING);
		
	}
	m_isPlay = true;
	// buffer should still be on the queue
	SLAndroidSimpleBufferQueueState bufferqueueState;
	res = (*bqPlayerBufferQueue)->GetState (bqPlayerBufferQueue, &bufferqueueState);
	Print ("buffer should still be on the queue, count:%d index:%d", bufferqueueState.count, bufferqueueState.index);

	// buffer should be removed from the queue
	res = (*bqPlayerBufferQueue)->GetState (bqPlayerBufferQueue, &bufferqueueState);
	Print ("clean buffer:%d %d", bufferqueueState.count, bufferqueueState.index);
	
}

/*
 *	@function: bqPlayerCallback
 *  @input	 : SLAndroidSimpleBufferQueueItf bq; context: current audio engin instance
 *  @output	 : void
 *  @describe: This function is called by AudioPlayer when buffer queue is played over.
			Then it will notify BufferWrapper to update player's buffer for playing.
 */
void CAudioEngin::bqPlayerCallback (SLAndroidSimpleBufferQueueItf bq, void *context)
{
	Print ("[CAudioEngin]Engine call back");
	assert (bq == bqPlayerBufferQueue);
	assert (NULL == context);
	CAudioEngin *audio = (CAudioEngin *)context;

	//notify BufferWrapper to update buffer.
	if (audio->m_pevent != NULL)
	{
		audio->m_pevent->PlayEqueueCallback ();
		audio->Play ();
	}
	else
	{
		Print ("Why is null?");
	}
}

/*
 *	@function:
 *  @input	 :
 *  @output	 :
 *  @describe:
 */
void CAudioEngin::clear ()
{
	(*bqPlayerBufferQueue)->Clear ((bqPlayerBufferQueue));
}

/*
 *	@function:
 *  @input	 :
 *  @output	 :
 *  @describe:
 */
void CAudioEngin::RegisterEvent (AudioEvenCallback *pevent)
{
	m_pevent = pevent;
}

/*
*	@function:
*  @input	 :
*  @output	 :
*  @describe:
*/
void CAudioEngin::UnRegisterEvent ()
{
	m_pevent = NULL;
}

void CAudioEngin::Shutdown ()
{
	Print ("Shut Down.");
	// destroy buffer queue audio player object, and invalidate all associated interfaces
	if (bqPlayerObject != NULL) {
		(*bqPlayerObject)->Destroy (bqPlayerObject);
		bqPlayerObject = NULL;
		bqPlayerPlay = NULL;
		bqPlayerBufferQueue = NULL;
		bqPlayerVolume = NULL;
	}
	Print ("Destory Player");
	/*
	// destroy output mix object, and invalidate all associated interfaces
	if (outputMixObject != NULL) {
		(*outputMixObject)->Destroy (outputMixObject);
		outputMixObject = NULL;
	}
	Print ("Destory Mix");
	// destroy engine object, and invalidate all associated interfaces
	if (engineObject != NULL) {
		(*engineObject)->Destroy (engineObject);
		engineObject = NULL;
		engineEngine = NULL;
	}
	Print ("Destory Engine");
	*/
	
}