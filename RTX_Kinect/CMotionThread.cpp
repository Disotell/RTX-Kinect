#include "stdafx.h"
#include "CMotionThread.h"
#include "SkeletonBasics.h"



CMotionThread::CMotionThread(HWND & wHandle, CRTX_KinectDlg * app)
{
	handle = wHandle;
	mainApp = app;
}

BOOL CMotionThread::InitInstance()
{
	return TRUE;
}


int CMotionThread::Run()
{
	CSkeletonBasics application(mainApp);
	
	application.Run(handle);
	return (0);
}