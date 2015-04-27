
// RTX_KinectDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "RTX_Kinect.h"
#include "RTXControl.h"
#include "rt100.h"


#define WM_MESSAGE_UPDATEDATA_TRUE  (WM_APP + 1)
#define WM_MESSAGE_UPDATEDATA_FALSE  (WM_MESSAGE_UPDATEDATA_TRUE + 1)

// CRTX_KinectDlg dialog
class CRTX_KinectDlg : public CDialogEx
{
// Construction
public:
	RTXControl  CRTX_KinectDlg::rtx;
	CRTX_KinectDlg(CWnd* pParent = NULL);	// standard constructor
	CRTX_KinectDlg::~CRTX_KinectDlg();	

	void CRTX_KinectDlg::JointData(RTXControl::JOINT_T j);
	 
	void CRTX_KinectDlg::KinectJointMove(RTXControl::JOINT_T j, int p);

	void CRTX_KinectDlg::SyncSliders();

// Dialog Data
	enum { IDD = IDD_RTX_KINECT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	afx_msg void OnBnClickedConnect();

	//Joint Movement
	afx_msg void OnBnClickedZedM();	
	afx_msg void OnNMReleasedcaptureZedSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedShoulderM();
	afx_msg void OnNMReleasedcaptureShoulderSlider(NMHDR *pNMHDR, LRESULT *pResult);
	
	afx_msg void OnBnClickedElbowM();
	afx_msg void OnNMReleasedcaptureElbowSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedPitchM();
	afx_msg void OnNMReleasedcapturePitchSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnNMReleasedcaptureRollSlider(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRollM();

	afx_msg void OnBnClickedYawM();
	afx_msg void OnNMReleasedcaptureYawSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedGripperM();
	afx_msg void OnNMReleasedcaptureGripperSlider(NMHDR *pNMHDR, LRESULT *pResult);

    afx_msg void OnBnClickedXM();
	afx_msg void OnNMReleasedcaptureXSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedYM();
	afx_msg void OnNMReleasedcaptureYSlider(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedZM();
	afx_msg void OnNMReleasedcaptureZSlider(NMHDR *pNMHDR, LRESULT *pResult);
 

	afx_msg void OnBnClickedKinectInit( );

	afx_msg LRESULT OnMessageUpdateData(UINT wParam, LONG lParam);

	afx_msg void OnBnClickedHome();

	//Temp Video Demo Buttons
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();


	DECLARE_MESSAGE_MAP()
public:
	CSliderCtrl  JointSliders[7];
	CEditView JointEditView[7];
	CButton JointButton[7];

	CSliderCtrl  KSliders[3];
	CEditView KEditView[3];
	CButton KButton[3];

	CEdit EditBoxControl;
};
