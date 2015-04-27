
// RTX_Kinect.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CRTX_KinectApp:
// See RTX_Kinect.cpp for the implementation of this class
//


class CRTX_KinectApp : public CWinApp
{
public:
	CRTX_KinectApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRTX_KinectApp theApp;

