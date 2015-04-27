
// RTX_KinectDlg.cpp : implementation file


#include "stdafx.h"
#include "RTX_Kinect.h"
#include "RTXControl.h"
#include "stdlib.h"
#include <cstdlib>
#include "rt100.h"
#include "CMotionThread.h"


#define STAT_TIP 1
#define MOTOR_T_NUM 7
#define stepinc 10

CMotionThread* adapter;

int min_display_limit[MOTOR_T_NUM] = { -151, -90, -900, -98, -145, -105,  0 };
int max_display_limit[MOTOR_T_NUM] = { 180,   90,    0,   0,  175,  105, 82 };

//int min_display_limit[MOTOR_T_NUM] = { -90, -90, -915, -98,-151,  -90, -2 };
//int max_display_limit[MOTOR_T_NUM] = { 90,   90,    0,   0, 181,   90, 30 };

int min_k_display_limit[6] = { -430, -430, -1004, -98, -151, -1106 };
int max_k_display_limit[6] = {  430, 430,  0,    0, 181, 110 };

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App AboutnectDlg.h"
#include "afxdialogex.h"
#include "RTX_KinectDlg.h"
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)

END_MESSAGE_MAP()

// CRTX_KinectDlg dialog
CRTX_KinectDlg::CRTX_KinectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRTX_KinectDlg::IDD, pParent)
{
	rtx = RTXControl();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}



CRTX_KinectDlg::~CRTX_KinectDlg(){
	if (adapter != NULL)
	{
		adapter->Delete();
		adapter = NULL;
	}
}

void CRTX_KinectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//Joint Movement
	DDX_Text(pDX, IDC_ZED_MSG, rtx.cVal.ZED_J_V);
	DDX_Control(pDX, IDC_ZED_SLIDER,  JointSliders[rtx.ZED_J]);

	DDX_Text(pDX, IDC_SHOULDER_MSG, rtx.cVal.SHOULDER_J_V);
	DDX_Control(pDX, IDC_SHOULDER_SLIDER, JointSliders[rtx.SHOULDER_J]);

	DDX_Text(pDX, IDC_ELBOW_MSG, rtx.cVal.ELBOW_J_V);
	DDX_Control(pDX, IDC_ELBOW_SLIDER, JointSliders[rtx.ELBOW_J]);

	DDX_Text(pDX, IDC_PITCH_MSG, rtx.cVal.PITCH_J_V);
	DDX_Control(pDX, IDC_PITCH_SLIDER, JointSliders[rtx.PITCH_J]);

	DDX_Text(pDX, IDC_ROLL_MSG, rtx.cVal.ROLL_J_V);
	DDX_Control(pDX, IDC_ROLL_SLIDER, JointSliders[rtx.ROLL_J]);

	DDX_Text(pDX, IDC_YAW_MSG, rtx.cVal.YAW_J_V);
	DDX_Control(pDX, IDC_YAW_SLIDER, JointSliders[rtx.YAW_J]);

	DDX_Text(pDX, IDC_GRIPPER_MSG, rtx.cVal.GRIP_J_V);
	DDX_Control(pDX, IDC_GRIPPER_SLIDER, JointSliders[rtx.GRIP_J]);

	//Kinematic Movement
	DDX_Text(pDX, IDC_X_MSG, rtx.kVal.X_DH);
	DDX_Control(pDX, IDC_X_SLIDER, KSliders[rtx.X_K]);

	DDX_Text(pDX, IDC_Y_MSG, rtx.kVal.Y_DH);
	DDX_Control(pDX, IDC_Y_SLIDER, KSliders[rtx.Y_K]);

	DDX_Text(pDX, IDC_Z_MSG, rtx.kVal.Z_DH);
	DDX_Control(pDX, IDC_Z_SLIDER, KSliders[rtx.Z_K]);

	DDX_Control(pDX, IDC_EDIT1, EditBoxControl);
}

BEGIN_MESSAGE_MAP(CRTX_KinectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	//RTX Connect
	ON_BN_CLICKED(IDC_CONNECT, &CRTX_KinectDlg::OnBnClickedConnect)

	//Joint Movement Actions
	ON_BN_CLICKED(IDC_ZED_M, &CRTX_KinectDlg::OnBnClickedZedM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_ZED_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureZedSlider)

	ON_BN_CLICKED(IDC_SHOULDER_M, &CRTX_KinectDlg::OnBnClickedShoulderM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SHOULDER_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureShoulderSlider)

	ON_BN_CLICKED(IDC_ELBOW_M, &CRTX_KinectDlg::OnBnClickedElbowM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_ELBOW_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureElbowSlider)

	ON_BN_CLICKED(IDC_PITCH_M, &CRTX_KinectDlg::OnBnClickedPitchM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_PITCH_SLIDER, &CRTX_KinectDlg::OnNMReleasedcapturePitchSlider)

	ON_BN_CLICKED(IDC_ROLL_M, &CRTX_KinectDlg::OnBnClickedRollM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_ROLL_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureRollSlider)

	ON_BN_CLICKED(ID_YAW_M, &CRTX_KinectDlg::OnBnClickedYawM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_YAW_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureYawSlider)

	ON_BN_CLICKED(IDC_GRIPPER_M, &CRTX_KinectDlg::OnBnClickedGripperM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_GRIPPER_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureGripperSlider)

	//Cord Movement Actions
	ON_BN_CLICKED(IDC_X_M, &CRTX_KinectDlg::OnBnClickedXM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_X_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureXSlider)
	ON_BN_CLICKED(IDC_Y_M, &CRTX_KinectDlg::OnBnClickedYM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_Y_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureYSlider)
	ON_BN_CLICKED(IDC_Z_M, &CRTX_KinectDlg::OnBnClickedZM)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_Z_SLIDER, &CRTX_KinectDlg::OnNMReleasedcaptureZSlider)

	//Kinect Button
	ON_BN_CLICKED(IDC_KINECT_INIT, &CRTX_KinectDlg::OnBnClickedKinectInit)

	ON_MESSAGE(WM_MESSAGE_UPDATEDATA_FALSE, OnMessageUpdateData)

	ON_BN_CLICKED(IDC_HOME, &CRTX_KinectDlg::OnBnClickedHome)
 
	ON_BN_CLICKED(IDC_BUTTON1, &CRTX_KinectDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CRTX_KinectDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CRTX_KinectDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CRTX_KinectDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CRTX_KinectDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CRTX_KinectDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CRTX_KinectDlg::OnBnClickedButton7)

END_MESSAGE_MAP()

LRESULT CRTX_KinectDlg::OnMessageUpdateData(UINT wParam, LONG lParam){
	UpdateData(false);
	SyncSliders();
	return 0;
}

// CRTX_KinectDlg message handlers

BOOL CRTX_KinectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//Set Sliders Range and initial Position	 
	for (int i = 0; i < MOTOR_T_NUM; i++){
		CRTX_KinectDlg::JointSliders[i].SetRange(min_display_limit[i], max_display_limit[i], true);
	}

	//Set Sliders Range and initial Position	 
	for (int i = 0; i < 3; i++){
		CRTX_KinectDlg::KSliders[i].SetRange(min_k_display_limit[i], max_k_display_limit[i], true);
	}

	CFont *myFont = new CFont();
	myFont->CreateFont(20, 0, 0, 0, FW_HEAVY, true, false,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FIXED_PITCH | FF_MODERN, _T("Courier New"));
	EditBoxControl.SetFont(myFont);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRTX_KinectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRTX_KinectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRTX_KinectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRTX_KinectDlg::SyncSliders(){

	JointSliders[rtx.ELBOW_J].SetPos(rtx.cVal.ELBOW_J_V);
	JointSliders[rtx.SHOULDER_J].SetPos(rtx.cVal.SHOULDER_J_V);
	JointSliders[rtx.ZED_J].SetPos(rtx.cVal.ZED_J_V);
	JointSliders[rtx.PITCH_J].SetPos(rtx.cVal.PITCH_J_V);
	JointSliders[rtx.ROLL_J].SetPos(rtx.cVal.ROLL_J_V);
	JointSliders[rtx.YAW_J].SetPos(rtx.cVal.YAW_J_V);
	JointSliders[rtx.GRIP_J].SetPos(rtx.cVal.GRIP_J_V);
 
	KSliders[0].SetPos(rtx.kVal.X_DH);
	KSliders[1].SetPos(rtx.kVal.Y_DH);
	KSliders[2].SetPos(rtx.kVal.Z_DH);
}

void CRTX_KinectDlg::JointData(RTXControl::JOINT_T j){
	int p = JointSliders[j].GetPos();
	switch (j)
	{
	case RTXControl::JOINT_T::ELBOW_J:
		rtx.cVal.ELBOW_J_V = p;
		break;
	case RTXControl::JOINT_T::SHOULDER_J:
		rtx.cVal.SHOULDER_J_V = p;
		break;
	case RTXControl::JOINT_T::ZED_J:
		rtx.cVal.ZED_J_V = p;
		break;
	case RTXControl::JOINT_T::PITCH_J:
		rtx.cVal.PITCH_J_V = p;
		break;
	case RTXControl::JOINT_T::ROLL_J:
		rtx.cVal.ROLL_J_V = p;
		break;
	case RTXControl::JOINT_T::YAW_J:
		rtx.cVal.YAW_J_V = p;
		break;
	case RTXControl::JOINT_T::GRIP_J:
		rtx.cVal.GRIP_J_V = p;
		break;
	}
	rtx.fk();
	SyncSliders();
	UpdateData(FALSE);
}



void CRTX_KinectDlg::OnBnClickedKinectInit(){
	adapter = new CMotionThread(m_hWnd, this);
	adapter->CreateThread(CREATE_SUSPENDED);
	adapter->m_bAutoDelete = false;

	adapter->sName = _T("Test"); // Initialize something
	adapter->ResumeThread();
}


void CRTX_KinectDlg::KinectJointMove(RTXControl::JOINT_T j, int p){
	rtx.KinectJointMove(j, p);
}


void CRTX_KinectDlg::OnBnClickedConnect()
{
	EditBoxControl.SetWindowTextW(L"Attempting to initialize the RTX...");
	rtx.RTX_Init();
	EditBoxControl.SetWindowTextW(L"RTX Initialized");
	SyncSliders();
	UpdateData(false);
}

//JOINT MOVEMENT GUI 

void CRTX_KinectDlg::OnBnClickedZedM()
{
	RTXControl::JOINT_T j = rtx.ZED_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureZedSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T j = rtx.ZED_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedShoulderM()
{
	RTXControl::JOINT_T   j = rtx.SHOULDER_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureShoulderSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T   j = rtx.SHOULDER_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedElbowM()
{
	RTXControl::JOINT_T  j = rtx.ELBOW_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureElbowSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T  j = rtx.ELBOW_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedPitchM()
{
	RTXControl::JOINT_T  j = rtx.PITCH_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcapturePitchSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T  j = rtx.PITCH_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedRollM()
{
	RTXControl::JOINT_T  j = rtx.ROLL_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureRollSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T  j = rtx.ROLL_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedYawM()
{
	RTXControl::JOINT_T  j = rtx.YAW_J;
	rtx.JointMove(j);
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureYawSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T  j = rtx.YAW_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}

void CRTX_KinectDlg::OnBnClickedGripperM()
{
	RTXControl::JOINT_T  j = rtx.GRIP_J;
	rtx.JointMove(j);
	UpdateData(false);
}


void CRTX_KinectDlg::OnNMReleasedcaptureGripperSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	RTXControl::JOINT_T  j = rtx.GRIP_J;
	JointData(j);
	UpdateData(false);
	*pResult = 0;
}


//Coorindate Movement

void CRTX_KinectDlg::OnBnClickedXM()
{
	 rtx.MoveAll();
 
	UpdateData(false);
}

void CRTX_KinectDlg::OnNMReleasedcaptureXSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	rtx.kVal.X_DH = KSliders[0].GetPos();
	rtx.Oninvcalc();
	*pResult = 0;

	JointSliders[rtx.ELBOW_J].SetPos(rtx.cVal.ELBOW_J_V);
	JointSliders[rtx.SHOULDER_J].SetPos(rtx.cVal.SHOULDER_J_V);
	JointSliders[rtx.ZED_J].SetPos(rtx.cVal.ZED_J_V);
	JointSliders[rtx.PITCH_J].SetPos(rtx.cVal.PITCH_J_V);
	JointSliders[rtx.ROLL_J].SetPos(rtx.cVal.ROLL_J_V);
	JointSliders[rtx.YAW_J].SetPos(rtx.cVal.YAW_J_V);
	JointSliders[rtx.GRIP_J].SetPos(rtx.cVal.GRIP_J_V);

	KSliders[1].SetPos(rtx.kVal.Y_DH);
	KSliders[2].SetPos(rtx.kVal.Z_DH);
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedYM()
{
	rtx.MoveAll();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnNMReleasedcaptureYSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	rtx.kVal.Y_DH = KSliders[1].GetPos();
	rtx.Oninvcalc();
	JointSliders[rtx.ELBOW_J].SetPos(rtx.cVal.ELBOW_J_V);
	JointSliders[rtx.SHOULDER_J].SetPos(rtx.cVal.SHOULDER_J_V);
	JointSliders[rtx.ZED_J].SetPos(rtx.cVal.ZED_J_V);
	JointSliders[rtx.PITCH_J].SetPos(rtx.cVal.PITCH_J_V);
	JointSliders[rtx.ROLL_J].SetPos(rtx.cVal.ROLL_J_V);
	JointSliders[rtx.YAW_J].SetPos(rtx.cVal.YAW_J_V);
	JointSliders[rtx.GRIP_J].SetPos(rtx.cVal.GRIP_J_V);

	KSliders[0].SetPos(rtx.kVal.X_DH);
	 
	KSliders[2].SetPos(rtx.kVal.Z_DH);
	UpdateData(false);
	*pResult = 0;
}



void CRTX_KinectDlg::OnBnClickedZM()
{
	rtx.MoveAll();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnNMReleasedcaptureZSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	rtx.kVal.Z_DH = KSliders[2].GetPos();
	rtx.Oninvcalc();
	JointSliders[rtx.ELBOW_J].SetPos(rtx.cVal.ELBOW_J_V);
	JointSliders[rtx.SHOULDER_J].SetPos(rtx.cVal.SHOULDER_J_V);
	JointSliders[rtx.ZED_J].SetPos(rtx.cVal.ZED_J_V);
	JointSliders[rtx.PITCH_J].SetPos(rtx.cVal.PITCH_J_V);
	JointSliders[rtx.ROLL_J].SetPos(rtx.cVal.ROLL_J_V);
	JointSliders[rtx.YAW_J].SetPos(rtx.cVal.YAW_J_V);
	JointSliders[rtx.GRIP_J].SetPos(rtx.cVal.GRIP_J_V);

	KSliders[0].SetPos(rtx.kVal.X_DH);
	KSliders[1].SetPos(rtx.kVal.Y_DH);
	 
	UpdateData(false);
	*pResult = 0;
}


void CRTX_KinectDlg::OnBnClickedHome()
{
	rtx.RTX_Home();
	rtx.MoveAll();
	SyncSliders();
	UpdateData(false);
}

//Temp Video Demo Buttons

void CRTX_KinectDlg::OnBnClickedButton1()
{
	rtx.RTX_B1();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedButton2()
{
	rtx.RTX_B2();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedButton3()
{
	rtx.RTX_B3();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedButton4()
{
	rtx.RTX_B4();
	SyncSliders();
	UpdateData(false);
}

void CRTX_KinectDlg::OnBnClickedButton5()
{
	rtx.RTX_B5();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedButton6()
{
	rtx.RTX_B6();
	SyncSliders();
	UpdateData(false);
}


void CRTX_KinectDlg::OnBnClickedButton7()
{
	rtx.RTX_B7();
	SyncSliders();
	UpdateData(false);
}
