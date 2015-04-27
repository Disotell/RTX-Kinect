#include "stdafx.h"
 
#define LEFT -1;
#define RIGHT 1;

#pragma once
class RTXControl
{
public:
	RTXControl();
	~RTXControl();
 
	typedef enum JOINT_T
	{
		ELBOW_J,
		SHOULDER_J,
		ZED_J,
		PITCH_J,
		ROLL_J,
		YAW_J,
		GRIP_J
	}  JOINT_T;

	typedef enum KM_T
	{
		X_K,
		Y_K,
		Z_K,
		PITCH_K,
		ROLL_K,
		YAW_K,
	} RTXControl::KM_T;

	struct EC_V
	{
		int ELBOW =0;
		int SHOULDER=0;
		int ZED=0;
		int WRIST1=0;
		int WRIST2=0;
		int YAW=0;
		int GRIP=0;
	} RTXControl::ecVal;

	struct JOINT_V
	{
		int ELBOW_J_V = 0;
		int SHOULDER_J_V = 0;
		int ZED_J_V = 0;
		int PITCH_J_V = 0;
		int ROLL_J_V = 0;
		int YAW_J_V = 0;
		int GRIP_J_V = 0;
	} RTXControl::cVal;


	struct DH_V
	{
		int X_DH = 430;
		int Y_DH = 0;
		int Z_DH = 0;
		int PITCH_DH = 0;
		int ROLL_DH = 0;
		int YAW_DH = 0;
	}RTXControl::kVal;


	void RTXControl::RTX_Init();
	void RTXControl::RTX_Shutdown();
	void RTXControl::RTX_Home();

	void RTXControl::e(int p);
	void RTXControl::s(int p);
	void RTXControl::z(int p);
	void RTXControl::w(int p,int r);
	void RTXControl::y(int p);
	void RTXControl::g(int p);

	void RTXControl::EncoderUpdate();
	void RTXControl::JointUpdate();


	void RTXControl::EncoderReadRTX();
	void RTXControl::MoveAll();
	void RTXControl::KinectJointMove(RTXControl::JOINT_T j, int p);
	void RTXControl::JointMove(RTXControl::JOINT_T j);
	

	
	void RTXControl::fk();
	void RTXControl::Oninvcalc();
	void RTXControl::ForwardKinematic();
	void movepos(int e, int s, int z, int w1, int w2, int y, int g);
	 

	//Temp Video Demo Methods
  
	void RTXControl::RTX_B1();
	void RTXControl::RTX_B2();
	void RTXControl::RTX_B3();
	void RTXControl::RTX_B4();
	void RTXControl::RTX_B5();
	void RTXControl::RTX_B6();
	void RTXControl::RTX_B7();
 
	void RTXControl::RTX_Lunge();
	void RTXControl::RTX_Parry();
	void RTXControl::RTX_Rabbit();
	void RTXControl::RTX_Center();
	void RTXControl::RTX_Start();

	void RTXControl::GetWater();
	void RTXControl::GrabWater();
	void RTXControl::RTX_Sword_Home(int direction);
	void RTXControl::RTX_Sword_Swing(int direction);
	void RTXControl::RTX_Sword_Pullback(int direction);
	void Extend_Sword();
	void Tap_Sword();
	void Prep_Downward_Strike();
	void Do_Downward_Strike();
	void Block_Position();

};

