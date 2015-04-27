
#include "stdafx.h"
#include "Resource.h"
#include "RTX_KinectDlg.h"

#include <windows.h>

class CMotionThread : public CWinThread
{

public:
	HWND handle;
	CRTX_KinectDlg * mainApp;
	int temp;
	CMotionThread(HWND & wHandle, CRTX_KinectDlg * app);
	virtual ~CMotionThread() {};
	virtual BOOL InitInstance();
	virtual int Run();
	CString sName;
};

