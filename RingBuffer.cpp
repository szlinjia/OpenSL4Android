#include "RingBuffer.h"

#define min(a,b) a<b?a:b

RingBuffer RingBuffer::_me = RingBuffer ();

RingBuffer::RingBuffer ()
{
	Reset ();
}

RingBuffer::~RingBuffer ()
{

}

void RingBuffer::Reset ()
{
	m_wIndex = 0;
	m_rIndex = -1;
	memset (g_ringBuf, 0, sizeof(g_ringBuf));
}

RingBuffer *RingBuffer::getRingBuffer ()
{
	return &_me;
}

void RingBuffer::ReadBuffer (short** stereoBuffer)
{
	int index = m_rIndex * SAMPLESIZE*2;
	Print ("[ReadBuffer] m_rIndex=%d m_wIndex=%d index=%d", m_rIndex, m_wIndex, index);
	if (m_rIndex == -1)
	{
		index = 0;
		m_rIndex = 0;
	}
	*stereoBuffer = &g_ringBuf[index];

}

void RingBuffer::IncreaseReadIndex ()
{
	m_rIndex++;
	if (m_rIndex > BUFFERCOUNT - 1)
	{
		m_rIndex = 0;
	}
}

void RingBuffer::WriteBuffer (short *data, int size)
{
	int index = m_wIndex * SAMPLESIZE * 2;
	
	//block here
	while (m_rIndex == m_wIndex)
	{
		//Print ("Wait for writing:m_rIndex=%d, m_wIndex=%d",m_rIndex, m_wIndex);
		usleep (1000);
	}
	
	int _size = min (size, SAMPLESIZE*sizeof(stereo));
	unsigned int nframes = _size / sizeof(stereo);
	memset (&g_ringBuf[index],0, SAMPLESIZE*sizeof(stereo));
	Print ("[WriteBuffer] m_wIndex=%d m_rIndex=%d nframes=%d", m_wIndex, m_rIndex, nframes);
	memcpy (&g_ringBuf[index], data, _size);
	m_wIndex++;
	if (m_wIndex > BUFFERCOUNT - 1)
	{
		m_wIndex = 0;
	}
}

bool RingBuffer::isRWIndexTheSame ()
{
	if (m_wIndex == m_rIndex)
	{
		//Print ("[isRWIndexTheSame]m_wIndex=%d,m_rIndex=%d", m_wIndex, m_rIndex);
	}
	
	return (m_wIndex == m_rIndex) ? true : false;
}