#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H
/*=========================================================================
class name	: Ring buffer 
Author		: Li Lin
Date		: 03/24/15
Describe	: Provides read and write function 
=========================================================================*/
#include <assert.h>
#include <jni.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include "AudioEngin.h"

#define BUFFERCOUNT 8

static short g_ringBuf[SAMPLESIZE * 2 * BUFFERCOUNT];

class RingBuffer
{

public:
	RingBuffer ();
	~RingBuffer ();
	static RingBuffer *getRingBuffer ();
	void  ReadBuffer (short **stereoBuffer);
	void  IncreaseReadIndex ();
	void  WriteBuffer (short *data, int size);
	bool isRWIndexTheSame ();
	int getWIndex (){ return m_wIndex; }
	int getRIndex (){ return m_rIndex; }
	void Reset ();
private:
	unsigned int m_rIndex;
	unsigned int m_wIndex;
	static RingBuffer _me;

};

#endif