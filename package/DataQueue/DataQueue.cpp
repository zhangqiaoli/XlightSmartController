#include "DataQueue.h"

CManulSync::CManulSync()
{
	//m_lock = CreateMutex (NULL, false, NULL);
	m_lock = false;
}

/*
CManulSync::~CManulSync()
{
	if (m_lock != NULL)
	{
		CloseHandle (m_lock);
		m_lock = NULL;
	}
}
*/

bool CManulSync::Enter(UL ms)
{
	//WaitForSingleObject (m_sync, INFINITE);
	UL start = millis();
	while( millis() - start  <= ms || ms == 0) {
		if(!m_lock) break;
	}

	return !m_lock;
}

void CManulSync::Leave()
{
	//ReleaseMutex (m_sync);
	m_lock = false;
}

CDataQueue::CDataQueue()
{
	m_pBuffer = NULL;
	m_length = 0;
	m_maxlen = 0;
}

CDataQueue::CDataQueue(UL f_maxlen)
{
	m_pBuffer = NULL;
	m_length = 0;
	m_maxlen = 0;
	CreateDataBuffer(f_maxlen);
}

CDataQueue::~CDataQueue()
{
	ClearBuffer();
}

void CDataQueue::CreateDataBuffer(UL f_nlen)
{
	if( f_nlen > 0 )
	{
		m_sync.Enter();
		if( m_pBuffer )
			delete[] m_pBuffer;
		m_pBuffer = new UC[f_nlen];
		m_length = 0;
		m_maxlen = f_nlen;
		m_sync.Leave();
	}
}

UL CDataQueue::GetMaxLength()
{
	UL lv_Value;

	m_sync.Enter();
	lv_Value = m_maxlen;
	m_sync.Leave();

	return lv_Value;
}

void CDataQueue::SetMaxLength(UL f_maxlen)
{
	CreateDataBuffer(f_maxlen);
}

UL CDataQueue::Append(const UC* data, UL len)
{
	if (data == NULL || len <= 0 ) return 0;

	UL lv_retval;

	m_sync.Enter();
	if( m_length + len > m_maxlen )
		lv_retval = -1;
	else
	{
		memcpy(m_pBuffer + m_length, data, len);
		m_length += len;
		lv_retval = len;
	}
	m_sync.Leave();

	return lv_retval;
}

UL CDataQueue::Remove(UL f_first, UC* data)
{
	UL length = 0, copylen;

	m_sync.Enter();

	copylen = ( f_first > 0 ? f_first : m_length );
	if( copylen > 0 )
	{
		if( data != NULL )
		{
			//try
			//{
				memcpy(data, m_pBuffer, copylen);
				length = copylen;
			//}
			/*
			catch(...)
			{
				length = -1;
			}*/
		}
		m_length -= copylen;
		if( m_length )
			memmove(m_pBuffer, m_pBuffer + copylen, m_length);
	}
	m_sync.Leave();

	return length;
}

UL CDataQueue::Length()
{
	UL length;

	m_sync.Enter();
	length = m_length;
	m_sync.Leave();

	return length;
}

void CDataQueue::ClearBuffer()
{
	m_sync.Enter();
	if( m_pBuffer )
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_length = 0;
	m_maxlen = 0;
	m_sync.Leave();
}

UC *CDataQueue::GetBuffer()
{
	return m_pBuffer;
}

void CDataQueue::LockBuffer()
{
	m_sync.Enter();
}

void CDataQueue::UnLockBuffer()
{
	m_sync.Leave();
}
