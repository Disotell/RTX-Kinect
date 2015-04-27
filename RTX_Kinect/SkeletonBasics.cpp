//------------------------------------------------------------------------------
// <copyright file="SkeletonBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#include <time.h>
#include "stdafx.h"
#include "RTX_Kinect.h"
#include <iostream>
#include <io.h>
#include <strsafe.h>
#include "SkeletonBasics.h"
#include "resource.h"

#include <io.h>
#include <fcntl.h>

#include <vector>
#include "NuiApi.h"
#include "NuiSensor.h"

#include "RTX_KinectDlg.h"
#define INITGUID
#include <guiddef.h>
#include <stdlib.h>

using namespace std;

static const float g_JointThickness = 3.0f;
static const float g_TrackedBoneThickness = 6.0f;
static const float g_InferredBoneThickness = 1.0f;

// Static initializers
LPCWSTR CSkeletonBasics::GrammarFileName = L"RTX_Kinect_Commands.grxml";

// This is the class ID we expect for the Microsoft Speech recognizer.
// Other values indicate that we're using a version of sapi.h that is
// incompatible with this sample.
DEFINE_GUID(CLSID_ExpectedRecognizer, 0x495648e7, 0xf7ab, 0x4267, 0x8e, 0x0f, 0xca, 0xfb, 0x7a, 0x33, 0xc1, 0x60);

//Vector for holding previously tracked values for hands
vector<float> vLeftHandX;
vector<float> vRightHandX;
vector<float> vLeftHandY;
vector<float> vRightHandY;
 
RTXControl::JOINT_T current_joint = RTXControl::JOINT_T::ELBOW_J;
const int  joint_step[7] = { 30, 30, 100, 30, 30, 30, 15 };

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>

/// <summary>
/// Constructor
/// </summary>
CSkeletonBasics::CSkeletonBasics(CRTX_KinectDlg * app) :
    m_pD2DFactory(NULL),
    m_hNextSkeletonEvent(INVALID_HANDLE_VALUE),
    m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE),
    m_bSeatedMode(false),
    m_pRenderTarget(NULL),
    m_pBrushJointTracked(NULL),
    m_pBrushJointInferred(NULL),
    m_pBrushBoneTracked(NULL),
    m_pBrushBoneInferred(NULL),
    m_pNuiSensor(NULL),
	m_pKinectAudioStream(NULL),
	m_pSpeechStream(NULL),
	m_pSpeechRecognizer(NULL),
	m_pSpeechContext(NULL),
	m_pSpeechGrammar(NULL),
	m_hSpeechEvent(INVALID_HANDLE_VALUE)
{

    ZeroMemory(m_Points,sizeof(m_Points));
	mainApp = app;
}

/// <summary>
/// Destructor
/// </summary>
CSkeletonBasics::~CSkeletonBasics()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hNextSkeletonEvent);
    }

    // clean up Direct2D objects
    DiscardDirect2DResources();

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    SafeRelease(m_pNuiSensor);

	SafeRelease(m_pNuiSensor);
	SafeRelease(m_pKinectAudioStream);
	SafeRelease(m_pSpeechStream);
	SafeRelease(m_pSpeechRecognizer);
	SafeRelease(m_pSpeechContext);
	SafeRelease(m_pSpeechGrammar);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CSkeletonBasics::Run(HWND handle)
{
	if (CLSID_ExpectedRecognizer != CLSID_SpInprocRecognizer)
	{
		MessageBoxW(NULL, L"This sample was compiled against an incompatible version of sapi.h.\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed and then rebuild application.", L"Missing requirements", MB_OK | MB_ICONERROR);

		return EXIT_FAILURE;
	}
	
	SetStatusMessage(L"Attempting to initialize Kinect...");

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (FAILED(hr))
	{
		CoUninitialize();
		return 0;
	}
	
	MSG       msg = {0};

    // gets main application window
	HWND hWndApp = handle;
	m_hWnd = handle;
	
    const int eventCount = 1;
    HANDLE hEvents[eventCount];

	// Init Direct2D
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

	// Look for a connected Kinect, and create it if found
	hr = CreateFirstConnected();
	if (FAILED(hr))
	{
		return hr;
	}

	SetStatusMessage(L"Kinect Voice and Motion initialized and ready to use");

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        hEvents[0] = m_hNextSkeletonEvent;

        // Check to see if we have either a message (by passing in QS_ALLEVENTS)
        // Or a Kinect event (hEvents)
        // Update() will check for Kinect events individually, in case more than one are signalled
		MsgWaitForMultipleObjects(eventCount, hEvents, FALSE, INFINITE, QS_ALLINPUT | MWMO_INPUTAVAILABLE);

        // Explicitly check the Kinect frame event since MsgWaitForMultipleObjects
        // can return for other reasons even though it is signaled.
        Update();

		// Explicitly check for new speech recognition events since
		// MsgWaitForMultipleObjects can return for other reasons
		// even though it is signaled.
		ProcessSpeech();

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
	
			// If a dialog message will be taken care of by the dialog proc
            if ((hWndApp != NULL) && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Main processing function
/// </summary>
void CSkeletonBasics::Update()
{
    if (NULL == m_pNuiSensor)
    {
        return;
    }

    // Wait for 0ms, just quickly test if it is time to process a skeleton
    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0) )
    {
        ProcessSkeleton();
    }
}

/// <summary>
/// Create the first connected Kinect found 
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CSkeletonBasics::CreateFirstConnected()
{
    INuiSensor * pNuiSensor;

    int iSensorCount = 0;
    HRESULT hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using skeleton
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_AUDIO);
        if (SUCCEEDED(hr))
        {
            // Create an event that will be signaled when skeleton data is available
            m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

            // Open a skeleton stream to receive skeleton data
            hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0); 
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!");
		return -1;
    }


	hr = InitializeAudioStream();
	if (FAILED(hr))
	{
		SetStatusMessage(L"Could not initialize audio stream.");
		return hr;
	}

	hr = CreateSpeechRecognizer();
	if (FAILED(hr))
	{
		SetStatusMessage(L"Could not create speech recognizer. Please ensure that Microsoft Speech SDK and other sample requirements are installed.");
		return hr;
	}

	hr = LoadSpeechGrammar();
	if (FAILED(hr))
	{
		SetStatusMessage(L"Could not load speech grammar. Please ensure that grammar configuration file was properly deployed.");
		return hr;
	}

	hr = StartSpeechRecognition();
	if (FAILED(hr))
	{
		SetStatusMessage(L"Could not start recognizing speech.");
		return hr;
	}

    return hr;
}

/// <summary>
/// Handle new skeleton data
/// </summary>
void CSkeletonBasics::ProcessSkeleton()
{
    NUI_SKELETON_FRAME skeletonFrame = {0};

    HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
    if ( FAILED(hr) )
    {
        return;
    }

    // smooth out the skeleton data
    m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);
	
    // Endure Direct2D is ready to draw
    hr = EnsureDirect2DResources( );
    if ( FAILED(hr) )
    {
        return;
    }

    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear( );
	HWND c_handle = GetDlgItem(m_hWnd, IDC_VIDEOVIEW);
    RECT rct;
	GetClientRect(c_handle, &rct);
    int width = rct.right;
    int height = rct.bottom;

	
	float closestZValue = 4;
	int closestSkeletonIndex = -1;

    for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
    {
        NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;

        if (NUI_SKELETON_TRACKED == trackingState)
        {
			if (skeletonFrame.SkeletonData[i].Position.z < closestZValue){
				closestZValue = skeletonFrame.SkeletonData[i].Position.z;
				closestSkeletonIndex = i;
			}

        }
    }

	if (closestSkeletonIndex != -1){
		// We're tracking the skeleton, draw it
		DrawSkeleton(skeletonFrame.SkeletonData[closestSkeletonIndex], width, height);
	}

	checkForGesture(skeletonFrame,closestSkeletonIndex);

    hr = m_pRenderTarget->EndDraw();

    // Device lost, need to recreate the render target
    // We'll dispose it now and retry drawing
    if (D2DERR_RECREATE_TARGET == hr)
    {
        hr = S_OK;
        DiscardDirect2DResources();
    }
}

/// <summary>
/// Draws a skeleton
/// </summary>
/// <param name="skel">skeleton to draw</param>
/// <param name="windowWidth">width (in pixels) of output buffer</param>
/// <param name="windowHeight">height (in pixels) of output buffer</param>
void CSkeletonBasics::DrawSkeleton(const NUI_SKELETON_DATA & skel, int windowWidth, int windowHeight)
{      
    int i;

    for (i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i)
    {
        m_Points[i] = SkeletonToScreen(skel.SkeletonPositions[i], windowWidth, windowHeight);
    }

    // Render Torso
    DrawBone(skel, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
    DrawBone(skel, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

    // Left Arm
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

    // Right Arm
    DrawBone(skel, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

    // Left Leg
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
    DrawBone(skel, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

    // Right Leg
    DrawBone(skel, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
    DrawBone(skel, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);

    // Draw the joints in a different color
    for (i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i)
    {
        D2D1_ELLIPSE ellipse = D2D1::Ellipse( m_Points[i], g_JointThickness, g_JointThickness );

        if ( skel.eSkeletonPositionTrackingState[i] == NUI_SKELETON_POSITION_INFERRED )
        {
            m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointInferred);
        }
        else if ( skel.eSkeletonPositionTrackingState[i] == NUI_SKELETON_POSITION_TRACKED )
        {
            m_pRenderTarget->DrawEllipse(ellipse, m_pBrushJointTracked);
        }
    }
}

/// <summary>
/// Draws a bone line between two joints
/// </summary>
/// <param name="skel">skeleton to draw bones from</param>
/// <param name="joint0">joint to start drawing from</param>
/// <param name="joint1">joint to end drawing at</param>
void CSkeletonBasics::DrawBone(const NUI_SKELETON_DATA & skel, NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
    NUI_SKELETON_POSITION_TRACKING_STATE joint0State = skel.eSkeletonPositionTrackingState[joint0];
    NUI_SKELETON_POSITION_TRACKING_STATE joint1State = skel.eSkeletonPositionTrackingState[joint1];

    // If we can't find either of these joints, exit
    if (joint0State == NUI_SKELETON_POSITION_NOT_TRACKED || joint1State == NUI_SKELETON_POSITION_NOT_TRACKED)
    {
        return;
    }

    // Don't draw if both points are inferred
    if (joint0State == NUI_SKELETON_POSITION_INFERRED && joint1State == NUI_SKELETON_POSITION_INFERRED)
    {
        return;
    }

    // We assume all drawn bones are inferred unless BOTH joints are tracked
    if (joint0State == NUI_SKELETON_POSITION_TRACKED && joint1State == NUI_SKELETON_POSITION_TRACKED)
    {
        m_pRenderTarget->DrawLine(m_Points[joint0], m_Points[joint1], m_pBrushBoneTracked, g_TrackedBoneThickness);
    }
    else
    {
        m_pRenderTarget->DrawLine(m_Points[joint0], m_Points[joint1], m_pBrushBoneInferred, g_InferredBoneThickness);
    }
}

/// <summary>
/// Converts a skeleton point to screen space
/// </summary>
/// <param name="skeletonPoint">skeleton point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
D2D1_POINT_2F CSkeletonBasics::SkeletonToScreen(Vector4 skeletonPoint, int width, int height)
{
    LONG x, y;
    USHORT depth;

    // Calculate the skeleton's position on the screen
    // NuiTransformSkeletonToDepthImage returns coordinates in NUI_IMAGE_RESOLUTION_320x240 space
    NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depth);

    float screenPointX = static_cast<float>(x * width) / cScreenWidth;
    float screenPointY = static_cast<float>(y * height) / cScreenHeight;

    return D2D1::Point2F(screenPointX, screenPointY);
}

/// <summary>
/// Ensure necessary Direct2d resources are created
/// </summary>
/// <returns>S_OK if successful, otherwise an error code</returns>
HRESULT CSkeletonBasics::EnsureDirect2DResources()
{
    HRESULT hr = S_OK;

    // If there isn't currently a render target, we need to create one
    if (NULL == m_pRenderTarget)
    {
		HWND c_handle = GetDlgItem(m_hWnd, IDC_VIDEOVIEW);
		RECT rc;
		GetClientRect(c_handle, &rc);

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        D2D1_SIZE_U size = D2D1::SizeU( width, height );
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.pixelFormat = D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        // Create a Hwnd render target, in order to render to the window set in initialize
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(GetDlgItem( m_hWnd, IDC_VIDEOVIEW), size),
            &m_pRenderTarget
            );
        if ( FAILED(hr) )
        {
            SetStatusMessage(L"Couldn't create Direct2D render target!");
            return hr;
        }

        //light green
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);

        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 1.0f), &m_pBrushBoneTracked);
        m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 1.0f), &m_pBrushBoneInferred);
    }

    return hr;
}

/// <summary>
/// Dispose Direct2d resources 
/// </summary>
void CSkeletonBasics::DiscardDirect2DResources( )
{
    SafeRelease(m_pRenderTarget);

    SafeRelease(m_pBrushJointTracked);
    SafeRelease(m_pBrushJointInferred);
    SafeRelease(m_pBrushBoneTracked);
    SafeRelease(m_pBrushBoneInferred);
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void CSkeletonBasics::SetStatusMessage(WCHAR * szMessage)
{

	LPTSTR temp = L"";
	mainApp->EditBoxControl.GetWindowTextW(temp, 500);
	
	mainApp->EditBoxControl.SetModify(false);
	mainApp->EditBoxControl.SetWindowTextW((std::wstring(temp) + L" " +  szMessage + L"\n").c_str());

}

//vector size, number of points needed to be consistent for a gesture
//ex. 20 points in a row need between min and maxMovement in a direction
unsigned int maxVectorSize = 20;
//min distance per check for it to be considered a gesture
float minMovement = 9e-4;
//max distance per check for it to be considered a gesture
float maxMovement = 1e-2;
//max change from beginning  to end to be considered a gesture
//ex. right hand moves along the Y axis, this would be the max the X value can change
float maxDelta = 5e-2;


void CSkeletonBasics::checkForGesture(NUI_SKELETON_FRAME newFrame, int closestSkeletonIndex)
{
	//Right Hand
	float rightHandY = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].y;
	float rightHandX = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].x;
	float rightElbowY = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT].y;

	//Left Hand
	float leftHandY = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].y;
	float leftHandX = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].x;
	float leftElbowY = newFrame.SkeletonData[closestSkeletonIndex].SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT].x;

	if (rightHandY > rightElbowY)
	{
		addToVector(vRightHandY, rightHandY);
		addToVector(vRightHandX, rightHandX);
		
		if (vRightHandY.size() == maxVectorSize && checkWithinDelta(vRightHandX, maxDelta)){

			if (checkNegative(vRightHandY, minMovement, maxMovement)){
				printRightHandDown();
				vRightHandY.clear();
				vRightHandX.clear();
			}
			else if (checkPositive(vRightHandY, minMovement, maxMovement)){
				printRightHandUp();
				vRightHandY.clear();
				vRightHandX.clear();
			}
		}

	}
	else {
		vRightHandY.clear();
		vRightHandX.clear();
	}


	if (leftHandY > leftElbowY)
	{

		addToVector(vLeftHandX, leftHandX);
		addToVector(vLeftHandY, leftHandY);

		if (vLeftHandX.size() == maxVectorSize && checkWithinDelta(vLeftHandY, maxDelta)){

			if (checkNegative(vLeftHandX, minMovement, maxMovement)){
				printLeftHandLeft();
				vLeftHandX.clear();
				vLeftHandY.clear();
			}
			else if (checkPositive(vLeftHandX, minMovement, maxMovement)){
				printLeftHandRight();
				vLeftHandX.clear();
				vLeftHandY.clear();
			}
		}

	}
	else {
		vLeftHandX.clear();
		vLeftHandY.clear();
	}

}

bool CSkeletonBasics::checkWithinDelta(std::vector<float> & hand, float delta){
	if (abs(hand[0] - hand[hand.size()-1]) > delta) return false;
	return true;
}

bool CSkeletonBasics::checkNegative(std::vector<float> & hand, float minMovement, float maxMovement){
	for (int i = 1; i < (int) hand.size(); i++){
		if (hand[i - 1] - hand[i] < minMovement || hand[i - 1] - hand[i] > maxMovement) return false;
	}
	return true;
}

bool CSkeletonBasics::checkPositive(std::vector<float> & hand, float minMovement, float maxMovement){
	for (int i = 1; i < (int) hand.size(); i++){
		if (hand[i] - hand[i-1] < minMovement || hand[i] - hand[i-1] > maxMovement) return false;
	}
	return true;
}



void CSkeletonBasics::addToVector(std::vector<float> & hand, float toAdd){
	if (hand.size() >= maxVectorSize){
		hand.erase(hand.begin());
		hand.insert(hand.end(), toAdd);
	}
	else{
		hand.insert(hand.end(), toAdd);
	}
}

HRESULT CSkeletonBasics::InitializeAudioStream()
{
	INuiAudioBeam*      pNuiAudioSource = NULL;
	IMediaObject*       pDMO = NULL;
	IPropertyStore*     pPropertyStore = NULL;
	IStream*            pStream = NULL;

	// Get the audio source
	HRESULT hr = m_pNuiSensor->NuiGetAudioSource(&pNuiAudioSource);
	if (SUCCEEDED(hr))
	{
		hr = pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&pDMO);

		if (SUCCEEDED(hr))
		{
			hr = pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&pPropertyStore);

			// Set AEC-MicArray DMO system mode. This must be set for the DMO to work properly.
			// Possible values are:
			//   SINGLE_CHANNEL_AEC = 0
			//   OPTIBEAM_ARRAY_ONLY = 2
			//   OPTIBEAM_ARRAY_AND_AEC = 4
			//   SINGLE_CHANNEL_NSAGC = 5
			PROPVARIANT pvSysMode;
			PropVariantInit(&pvSysMode);
			pvSysMode.vt = VT_I4;
			pvSysMode.lVal = (LONG)(2); // Use OPTIBEAM_ARRAY_ONLY setting. Set OPTIBEAM_ARRAY_AND_AEC instead if you expect to have sound playing from speakers.
			pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
			PropVariantClear(&pvSysMode);

			// Set DMO output format
			WAVEFORMATEX wfxOut = { AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0 };
			DMO_MEDIA_TYPE mt = { 0 };
			MoInitMediaType(&mt, sizeof(WAVEFORMATEX));

			mt.majortype = MEDIATYPE_Audio;
			mt.subtype = MEDIASUBTYPE_PCM;
			mt.lSampleSize = 0;
			mt.bFixedSizeSamples = TRUE;
			mt.bTemporalCompression = FALSE;
			mt.formattype = FORMAT_WaveFormatEx;
			memcpy(mt.pbFormat, &wfxOut, sizeof(WAVEFORMATEX));

			hr = pDMO->SetOutputType(0, &mt, 0);

			if (SUCCEEDED(hr))
			{
				m_pKinectAudioStream = new KinectAudioStream(pDMO);

				hr = m_pKinectAudioStream->QueryInterface(IID_IStream, (void**)&pStream);

				if (SUCCEEDED(hr))
				{
					hr = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), (void**)&m_pSpeechStream);

					if (SUCCEEDED(hr))
					{
						hr = m_pSpeechStream->SetBaseStream(pStream, SPDFID_WaveFormatEx, &wfxOut);
					}
				}
			}

			MoFreeMediaType(&mt);
		}
	}

	SafeRelease(pStream);
	SafeRelease(pPropertyStore);
	SafeRelease(pDMO);
	SafeRelease(pNuiAudioSource);

	return hr;
}

/// <summary>
/// Create speech recognizer that will read Kinect audio stream data.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSkeletonBasics::CreateSpeechRecognizer()
{
	ISpObjectToken *pEngineToken = NULL;

	HRESULT hr = CoCreateInstance(CLSID_SpInprocRecognizer, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), (void**)&m_pSpeechRecognizer);

	if (SUCCEEDED(hr))
	{
		m_pSpeechRecognizer->SetInput(m_pSpeechStream, FALSE);
		hr = SpFindBestToken(SPCAT_RECOGNIZERS, L"Language=409;Kinect=True", NULL, &pEngineToken);

		if (SUCCEEDED(hr))
		{
			m_pSpeechRecognizer->SetRecognizer(pEngineToken);
			hr = m_pSpeechRecognizer->CreateRecoContext(&m_pSpeechContext);

			// For long recognition sessions (a few hours or more), it may be beneficial to turn off adaptation of the acoustic model. 
			// This will prevent recognition accuracy from degrading over time.
			//if (SUCCEEDED(hr))
			//{
			//    hr = m_pSpeechRecognizer->SetPropertyNum(L"AdaptationOn", 0);                
			//}
		}
	}

	SafeRelease(pEngineToken);

	return hr;
}

/// <summary>
/// Load speech recognition grammar into recognizer.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSkeletonBasics::LoadSpeechGrammar()
{
	HRESULT hr = m_pSpeechContext->CreateGrammar(1, &m_pSpeechGrammar);

	if (SUCCEEDED(hr))
	{
		// Populate recognition grammar from file
		hr = m_pSpeechGrammar->LoadCmdFromFile(GrammarFileName, SPLO_STATIC);
	}

	return hr;
}

/// <summary>
/// Start recognizing speech asynchronously.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSkeletonBasics::StartSpeechRecognition()
{
	HRESULT hr = m_pKinectAudioStream->StartCapture();

	if (SUCCEEDED(hr))
	{
		// Specify that all top level rules in grammar are now active
		m_pSpeechGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

		// Specify that engine should always be reading audio
		m_pSpeechRecognizer->SetRecoState(SPRST_ACTIVE_ALWAYS);

		// Specify that we're only interested in receiving recognition events
		m_pSpeechContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

		// Ensure that engine is recognizing speech and not in paused state
		hr = m_pSpeechContext->Resume(0);
		if (SUCCEEDED(hr))
		{
			m_hSpeechEvent = m_pSpeechContext->GetNotifyEventHandle();
		}
	}

	return hr;
}

/// <summary>
/// Process recently triggered speech recognition events.
/// </summary>
void CSkeletonBasics::ProcessSpeech()
{
	const float ConfidenceThreshold = 0.3f;

	SPEVENT curEvent;
	ULONG fetched = 0;
	HRESULT hr = S_OK;

	m_pSpeechContext->GetEvents(1, &curEvent, &fetched);

	while (fetched > 0)
	{
		switch (curEvent.eEventId)
		{
		case SPEI_RECOGNITION:
			if (SPET_LPARAM_IS_OBJECT == curEvent.elParamType)
			{
				// this is an ISpRecoResult
				ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);
				SPPHRASE* pPhrase = NULL;

				hr = result->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr))
				{
					if ((pPhrase->pProperties != NULL) && (pPhrase->pProperties->pFirstChild != NULL))
					{
						const SPPHRASEPROPERTY* pSemanticTag = pPhrase->pProperties->pFirstChild;
						if (pSemanticTag->SREngineConfidence > ConfidenceThreshold)
						{
							SpeechAction action = MapSpeechTagToAction(pSemanticTag->pszValue);
							DoAction(action);
						}
					}
					::CoTaskMemFree(pPhrase);
				}
			}
			break;
		}

		m_pSpeechContext->GetEvents(1, &curEvent, &fetched);
	}

	return;
}
 
int joint = 0;// = current_joint;
void CSkeletonBasics::setNextJoint(boolean goingRight)
{
	 
	if (goingRight){
		if (joint == 6) joint = -1;
		joint++;
	}
	else{
		if (joint == 0) joint = 7;
		joint--;
	}
	current_joint = RTXControl::JOINT_T(joint);

	switch (current_joint){
	case RTXControl::JOINT_T::ELBOW_J:
		SetStatusMessage(L"Current joint changed to Elbow");
		break;
	case RTXControl::JOINT_T::SHOULDER_J:
		SetStatusMessage(L"Current joint changed to Shoulder");
		break;
	case RTXControl::JOINT_T::ZED_J:
		SetStatusMessage(L"Current joint changed to Zed");
		break;
	case RTXControl::JOINT_T::PITCH_J:
		SetStatusMessage(L"Current joint changed to Pitch");
		break;
	case RTXControl::JOINT_T::ROLL_J:
		SetStatusMessage(L"Current joint changed to Roll");
		break;
	case RTXControl::JOINT_T::YAW_J:
		SetStatusMessage(L"Current joint changed to Yaw");
		break;
	case RTXControl::JOINT_T::GRIP_J:
		SetStatusMessage(L"Current joint changed to Gripper");
		break;
	}
	 
}

 
void CSkeletonBasics::printLeftHandLeft()
{
	setNextJoint(false);
}

void CSkeletonBasics::printLeftHandRight()
{
	setNextJoint(true);
}

void CSkeletonBasics::printRightHandUp()
{
	 mainApp->KinectJointMove(current_joint, joint_step[current_joint]);
	 SendMessage(mainApp->m_hWnd, WM_MESSAGE_UPDATEDATA_FALSE, (WPARAM)0, (LPARAM)0);
}

void CSkeletonBasics::printRightHandDown()
{
     mainApp->KinectJointMove(current_joint, -joint_step[current_joint]);
	 SendMessage(mainApp->m_hWnd, WM_MESSAGE_UPDATEDATA_FALSE, (WPARAM)0, (LPARAM)0);
}




/// <summary>
/// Maps a specified speech semantic tag to the corresponding action to be performed on turtle.
/// </summary>
/// <returns>
/// Action that matches <paramref name="pszSpeechTag"/>, or TurtleActionNone if no matches were found.
/// </returns>
SpeechAction CSkeletonBasics::MapSpeechTagToAction(LPCWSTR pszSpeechTag)
{
	struct SpeechTagToAction
	{
		LPCWSTR pszSpeechTag;
		SpeechAction action;
	};

	const SpeechTagToAction Map[] =
	{
		{ L"RTXHome", Home},
		{ L"RTXZedPlus", ZedPlus },
		{ L"RTXZedMinus", ZedMinus },
		{ L"RTXShoulderPlus", ShoulderPlus },
		{ L"RTXShoulderMinus", ShoulderMinus },
		{ L"RTXElbowPlus", ElbowPlus },
		{ L"RTXElbowMinus", ElbowMinus },
		{ L"RTXYawPlus", YawPlus },
		{ L"RTXYawMinus", YawMinus },
		{ L"RTXPitchPlus", PitchPlus },
		{ L"RTXPitchMinus", PitchMinus },
		{ L"RTXRollPlus", RollPlus },
		{ L"RTXRollMinus", RollMinus },
		{ L"RTXGripperPlus", GripperPlus },
		{ L"RTXGripperMinus", GripperMinus }
	};

	SpeechAction action = RTXActionNone;

	for (int i = 0; i < _countof(Map); ++i)
	{
		if (0 == wcscmp(Map[i].pszSpeechTag, pszSpeechTag))
		{
			action = Map[i].action;
			break;
		}
	}

	return action;
}

void CSkeletonBasics::DoAction(SpeechAction action)
{
	int targeted_joint = 0;
	int direction = 1;
	switch (action)
	{
		case Home:
			mainApp->rtx.RTX_Home();
			return;
		case ZedPlus:
			targeted_joint = 2;
			break;
		case ZedMinus:
			targeted_joint = 2;
			direction = -1;
			break;
		case ShoulderPlus:
			targeted_joint = 1;
			break;
		case ShoulderMinus:
			direction = -1;
			targeted_joint = 1;
			break;
		case ElbowPlus:
			targeted_joint = 0;
			break;
		case ElbowMinus:
			direction = -1;
			targeted_joint = 0;
			break;
		case YawPlus:
			targeted_joint = 5;
			break;
		case YawMinus:
			direction = -1;
			targeted_joint = 5;
			break;
		case PitchPlus:
			targeted_joint = 3;
			break;
		case PitchMinus:
			direction = -1;
			targeted_joint = 3;
			break;
		case RollPlus:
			targeted_joint = 4;
			break;
		case RollMinus:
			direction = -1;
			targeted_joint = 4;
			break;
		case GripperPlus:
			targeted_joint = 6;
			break;
		case GripperMinus:
			targeted_joint = 6;
			direction = -1;
			break;
		case RTXActionNone:
			return;
	}
 
	mainApp->KinectJointMove(RTXControl::JOINT_T(targeted_joint), direction * joint_step[targeted_joint]);
	SendMessage(mainApp->m_hWnd, WM_MESSAGE_UPDATEDATA_FALSE, (WPARAM)0, (LPARAM)0);
}
