#ifndef PCS_DATAQUEUE_INCLUDED_
#define PCS_DATAQUEUE_INCLUDED_

#include "application.h"
#include "xliCommon.h"

//	little class to make code more readable
class CManulSync;
class CManulSync
{
protected:
	bool		m_lock;

public:
	CManulSync();
	//~CManulSync();

	CManulSync(CManulSync& s);
	CManulSync& operator= (CManulSync& s);

	bool Enter(UL ms=0);
	void Leave();
};

class CDataQueue
{
public:
	CDataQueue();
	CDataQueue(UL f_maxlen);
	~CDataQueue();

	UL GetMaxLength();
	void SetMaxLength(UL f_maxlen);
	UL Append(const UC* data, UL len);
	UL Remove(UL f_first = 0, UC* data = NULL);
	UL Length();
	void ClearBuffer();
	UC *GetBuffer();
	void LockBuffer();
	void UnLockBuffer();

protected:
	UC			*m_pBuffer;
	CManulSync	m_sync;
	UL		m_length;
	UL		m_maxlen;

	void		CreateDataBuffer(UL f_nlen);
};

#endif // PCS_DATAQUEUE_INCLUDED_

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
