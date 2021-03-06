﻿//------------------------------------------------------------------------------
// <copyright file="SkeletonBasics.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once
 
#include "resource.h"
#include "NuiApi.h"
#include "RTX_KinectDlg.h"
#include <vector>

#include "KinectAudioStream.h"

// For configuring DMO properties
#include <wmcodecdsp.h>

// For FORMAT_WaveFormatEx and such
#include <uuids.h>

// For speech APIs
// NOTE: To ensure that application compiles and links against correct SAPI versions (from Microsoft Speech
//       SDK), VC++ include and library paths should be configured to list appropriate paths within Microsoft
//       Speech SDK installation directory before listing the default system include and library directories,
//       which might contain a version of SAPI that is not appropriate for use together with Kinect sensor.
#include <sapi.h>
#include <sphelper.h>

enum SpeechAction
{
	Home,
	ZedPlus,
	ZedMinus,
	ShoulderPlus,
	ShoulderMinus,
	ElbowPlus,
	ElbowMinus,
	YawPlus,
	YawMinus,
	PitchPlus,
	PitchMinus,
	RollPlus,
	RollMinus,
	GripperPlus,
	GripperMinus,
	RTXActionNone
};

class CSkeletonBasics
{
    static const int        cScreenWidth  = 320;
    static const int        cScreenHeight = 240;

    static const int        cStatusMessageMaxLen = MAX_PATH*2;

public:
	CRTX_KinectDlg * mainApp;

    /// <summary>
    /// Constructor
    /// </summary>
    CSkeletonBasics(CRTX_KinectDlg * app);

    /// <summary>
    /// Destructor
    /// </summary>
    ~CSkeletonBasics();

    /// <summary>
    /// Creates the main window and begins processing
    /// </summary>
    /// <param name="hInstance"></param>
    /// <param name="nCmdShow"></param>
    int                     Run(HWND handle);

private:
	static LPCWSTR          GrammarFileName;

    HWND                    m_hWnd;

    bool                    m_bSeatedMode;

    // Current Kinect
    INuiSensor*             m_pNuiSensor;

    // Skeletal drawing
    ID2D1HwndRenderTarget*   m_pRenderTarget;
    ID2D1SolidColorBrush*    m_pBrushJointTracked;
    ID2D1SolidColorBrush*    m_pBrushJointInferred;
    ID2D1SolidColorBrush*    m_pBrushBoneTracked;
    ID2D1SolidColorBrush*    m_pBrushBoneInferred;
    D2D1_POINT_2F            m_Points[NUI_SKELETON_POSITION_COUNT];

	//Speech 
	// Audio stream captured from Kinect.
	KinectAudioStream*      m_pKinectAudioStream;

	// Stream given to speech recognition engine
	ISpStream*              m_pSpeechStream;

	// Speech recognizer
	ISpRecognizer*          m_pSpeechRecognizer;

	// Speech recognizer context
	ISpRecoContext*         m_pSpeechContext;

	// Speech grammar
	ISpRecoGrammar*         m_pSpeechGrammar;

	// Event triggered when we detect speech recognition
	HANDLE                  m_hSpeechEvent;

    // Direct2D
    ID2D1Factory*           m_pD2DFactory;
    
    HANDLE                  m_pSkeletonStreamHandle;
    HANDLE                  m_hNextSkeletonEvent;
    
	

    /// <summary>
    /// Main processing function
    /// </summary>
    void                    Update();

    /// <summary>
    /// Create the first connected Kinect found 
    /// </summary>
    /// <returns>S_OK on success, otherwise failure code</returns>
    HRESULT                 CreateFirstConnected();

    /// <summary>
    /// Handle new skeleton data
    /// </summary>
    void                    ProcessSkeleton();

    /// <summary>
    /// Ensure necessary Direct2d resources are created
    /// </summary>
    /// <returns>S_OK if successful, otherwise an error code</returns>
    HRESULT                 EnsureDirect2DResources( );

    /// <summary>
    /// Dispose Direct2d resources 
    /// </summary>
    void                    DiscardDirect2DResources( );

    /// <summary>
    /// Draws a bone line between two joints
    /// </summary>
    /// <param name="skel">skeleton to draw bones from</param>
    /// <param name="joint0">joint to start drawing from</param>
    /// <param name="joint1">joint to end drawing at</param>
    void                    DrawBone(const NUI_SKELETON_DATA & skel, NUI_SKELETON_POSITION_INDEX bone0, NUI_SKELETON_POSITION_INDEX bone1);

    /// <summary>
    /// Draws a skeleton
    /// </summary>
    /// <param name="skel">skeleton to draw</param>
    /// <param name="windowWidth">width (in pixels) of output buffer</param>
    /// <param name="windowHeight">height (in pixels) of output buffer</param>
    void                    DrawSkeleton(const NUI_SKELETON_DATA & skel, int windowWidth, int windowHeight);

    /// <summary>
    /// Converts a skeleton point to screen space
    /// </summary>
    /// <param name="skeletonPoint">skeleton point to tranform</param>
    /// <param name="width">width (in pixels) of output buffer</param>
    /// <param name="height">height (in pixels) of output buffer</param>
    /// <returns>point in screen-space</returns>
    D2D1_POINT_2F           SkeletonToScreen(Vector4 skeletonPoint, int width, int height);

	/// <summary>
	/// Initialize Kinect audio stream object.
	/// </summary>
	/// <returns>S_OK on success, otherwise failure code.</returns>
	HRESULT                 InitializeAudioStream();

	/// <summary>
	/// Create speech recognizer that will read Kinect audio stream data.
	/// </summary>
	/// <returns>
	/// <para>S_OK on success, otherwise failure code.</para>
	/// </returns>
	HRESULT                 CreateSpeechRecognizer();

	/// <summary>
	/// Load speech recognition grammar into recognizer.
	/// </summary>
	/// <returns>
	/// <para>S_OK on success, otherwise failure code.</para>
	/// </returns>
	HRESULT                 LoadSpeechGrammar();

	/// <summary>
	/// Start recognizing speech asynchronously.
	/// </summary>
	/// <returns>
	/// <para>S_OK on success, otherwise failure code.</para>
	/// </returns>
	HRESULT                 StartSpeechRecognition();

	/// <summary>
	/// Process recently triggered speech recognition events.
	/// </summary>
	void                    ProcessSpeech();

    /// <summary>
    /// Set the status bar message
    /// </summary>
    /// <param name="szMessage">message to display</param>
    void                    SetStatusMessage(WCHAR* szMessage);

	void checkForGesture(NUI_SKELETON_FRAME newFrame, int closestSkeletonIndex);

	void addToVector(std::vector<float> & hand, float toAdd);

	bool checkWithinDelta(std::vector<float> & hand, float delta);

	bool checkNegative(std::vector<float> & hand, float minMovement, float maxMovement);
	
	bool checkPositive(std::vector<float> & hand, float minMovement, float maxMovement);

	void setNextJoint(boolean pn);
	
	void printLeftHandLeft();

	void printLeftHandRight();

	void printRightHandUp();

	void printRightHandDown();

	/// <summary>
	/// Maps a specified speech semantic tag to the corresponding action to be performed on turtle.
	/// </summary>
	/// <returns>
	/// Action that matches <paramref name="pszSpeechTag"/>, or TurtleActionNone if no matches were found.
	/// </returns>
	SpeechAction            MapSpeechTagToAction(LPCWSTR pszSpeechTag);

	void DoAction(SpeechAction action);

};
