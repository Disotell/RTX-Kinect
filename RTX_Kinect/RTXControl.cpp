#include "stdafx.h"
#include "RTXControl.h"
#include "RT100.h"
#include "stdlib.h"
#include <cstdlib>

#define STAT_TIP 1
#define MOTOR_T_NUM 7
#define stepinc 10

int rCode = 0; //Status Hex from RTX
double RP_K = 0.07415;
boolean connected = false;
int data; // temp Field

//---------------------------------------------------
// ENCODER COUNTS FOR EACH UNIT OF MOTOR MOVEMENT
//
//	Axis	  Encoder count per unit
// 
//	Elbow				14.611  (count/deg)
//	Shoulder			29.221	(count/deg)
//	Zed					 3.750	(count/mm)
//	Wrist1				13.487	(count/deg)	
//	Wrist2				13.487	(count/deg)
//	Yaw					 9.740  (count/deg)
//	Gripper				14.706	(count/mm)
//---------------------------------------------------

double encoder_count[MOTOR_T_NUM] = { 14.611, 29.221, 3.750, 13.487, 13.487, 9.740,  14.706 };

int   post_init[MOTOR_T_NUM] = { 0, 0, -10, 0, 0, 0, 0};
 
//-------------------------------------------------------------------------------------------
// MAXIMUM RANGE FOR EACH UNIT OF MOTOR MOVEMENT		
//
//	Axis		EndStop to EndStop		Encoder Counts for	EndStop to EndStop		Total 
//				Encoder counts			Total Range			Range  Deg/mm 			Range 
// 
//	Elbow		 2206 to -2630			 4836				 90 to  -90	deg			331 deg
//	Shoulder	 2630 to -2630			 5260				180 to -151	deg			180 deg
//	Zed			 0 to -3554				 3303			   -915 to    0	 mm			915  mm
//	Pitch(W1+W2) 108 to -2642  			 2750				  2 to  -98	deg			102 deg
//	Roll(W1-W2)	 4882 to -3560  		 8442				181 to -151	deg			313 deg	 
//	Yaw			 1071+E/3 to -(1071+E/3) 2142				110 to  110 deg			220 deg
//	Gripper		 1200 to -30			 1200				  0 to   90  mm			90  mm
//--------------------------------------------------------------------------------------------

const double PI = 3.141592653589793238463;

//matrix variables
double a[4][4];
double b[4][4];
double c[4][4];
double d[4][4];

RTXControl::RTXControl()
{
	
}


RTXControl::~RTXControl()
{
    RTX_Shutdown();
}


void waitUntilDone(int *rCode)
{
	int status;
	do arm_general_status(&status, rCode);
	while ((status &STAT_TIP) != 0 && (*rCode == RT100_OK));
}

void  RTXControl::RTX_Init()
{
		arm_init_comms("COM1", &rCode);
        arm_init(&rCode);
 
		arm_write(ELBOW, SPEED, TOP_SPEED, &rCode);
		arm_write(SHOULDER, SPEED, TOP_SPEED, &rCode);
		arm_write(ZED, SPEED, TOP_SPEED, &rCode);
		arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
		arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);
		arm_write(YAW, SPEED, TOP_SPEED, &rCode);
		arm_write(GRIP, SPEED, 50, &rCode);

		waitUntilDone(&rCode);
		arm_stop(FREE_OFF, &rCode);

		EncoderReadRTX();
		JointUpdate();
		fk();
}

void RTXControl::RTX_Shutdown(){
	arm_shutdown(&rCode);
}


void RTXControl::EncoderUpdate(){
	ecVal.ELBOW = (int)(cVal.ELBOW_J_V*encoder_count[ELBOW]);
	ecVal.SHOULDER = (int)(cVal.SHOULDER_J_V*encoder_count[SHOULDER]);
	ecVal.ZED = (int)(cVal.ZED_J_V*encoder_count[ZED]);
	ecVal.WRIST1 = (int)((cVal.PITCH_J_V + cVal.ROLL_J_V) / 0.07415);
	ecVal.WRIST2 = (int)((cVal.PITCH_J_V - cVal.ROLL_J_V) / 0.07415);
	ecVal.YAW = (int)(cVal.YAW_J_V*encoder_count[YAW]);
	ecVal.GRIP = (int)(cVal.GRIP_J_V*encoder_count[GRIP]);
}

void RTXControl::JointUpdate(){
	cVal.ELBOW_J_V = (int)(ecVal.ELBOW / encoder_count[ELBOW]);
	cVal.SHOULDER_J_V = (int)(ecVal.SHOULDER / encoder_count[SHOULDER]);
	cVal.ZED_J_V = (int)(ecVal.ZED / encoder_count[ZED]);
	cVal.PITCH_J_V = (int)(((ecVal.WRIST1 + ecVal.WRIST2) / 2) * 0.07415);
	cVal.ROLL_J_V = (int)(((ecVal.WRIST1 - ecVal.WRIST2) / 2) *0.07415);
	cVal.YAW_J_V = (int)(ecVal.YAW / encoder_count[YAW]);
	cVal.GRIP_J_V = (int)(ecVal.GRIP / encoder_count[GRIP]);
	fk();
}


void RTXControl::EncoderReadRTX(){
	arm_read(ELBOW, CURRENT_POSITION, &ecVal.ELBOW, &rCode);
	arm_read(SHOULDER, CURRENT_POSITION, &ecVal.SHOULDER, &rCode);
	arm_read(ZED, CURRENT_POSITION, &ecVal.ZED, &rCode);
	arm_read(WRIST1, CURRENT_POSITION, &ecVal.WRIST1, &rCode);
	arm_read(WRIST2, CURRENT_POSITION, &ecVal.WRIST2, &rCode);
	arm_read(YAW, CURRENT_POSITION, &ecVal.YAW, &rCode);
	arm_read(GRIP, CURRENT_POSITION, &ecVal.GRIP, &rCode);
}

void RTXControl::MoveAll(){
	EncoderUpdate();
	arm_write(ELBOW, NEW_POSITION, ecVal.ELBOW, &rCode);
	arm_write(SHOULDER, NEW_POSITION, ecVal.SHOULDER, &rCode);
	arm_write(ZED, NEW_POSITION, ecVal.ZED, &rCode);
	arm_write(WRIST1, NEW_POSITION, ecVal.WRIST1, &rCode);
	arm_write(WRIST2, NEW_POSITION, ecVal.WRIST2, &rCode);
	arm_write(YAW, NEW_POSITION, ecVal.YAW, &rCode);
	arm_write(GRIP, NEW_POSITION, ecVal.GRIP, &rCode);

	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	arm_stop(FREE_OFF, &rCode);
	JointUpdate();
}

void RTXControl::JointMove(JOINT_T j){
	EncoderUpdate();
	switch (j)
	{
	case ELBOW_J:
		arm_write(ELBOW, NEW_POSITION, ecVal.ELBOW, &rCode);
		break;

	case SHOULDER_J:
		arm_write(SHOULDER, NEW_POSITION, ecVal.SHOULDER, &rCode);
		break;

	case ZED_J:
		arm_write(ZED, NEW_POSITION, ecVal.ZED, &rCode);
		break;

	case PITCH_J:
		arm_write(WRIST1, NEW_POSITION, ecVal.WRIST1, &rCode);
		arm_write(WRIST2, NEW_POSITION, ecVal.WRIST2, &rCode);
		break;

	case ROLL_J:
		arm_write(WRIST1, NEW_POSITION, ecVal.WRIST1, &rCode);
		arm_write(WRIST2, NEW_POSITION, ecVal.WRIST2, &rCode);
		break;
	case YAW_J:
		arm_write(YAW, NEW_POSITION, ecVal.YAW, &rCode);
		break;

	case GRIP_J:
		arm_write(GRIP, NEW_POSITION,ecVal.GRIP, &rCode);
	}

	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	arm_stop(FREE_OFF, &rCode);
}


void RTXControl::KinectJointMove(JOINT_T j, int p){
	switch (j)
	{
	case ELBOW_J:
		cVal.ELBOW_J_V = cVal.ELBOW_J_V + p;

		break;

	case SHOULDER_J:
		cVal.SHOULDER_J_V = cVal.SHOULDER_J_V + p;
		break;

	case ZED_J:
		cVal.ZED_J_V = cVal.ZED_J_V + p;
		break;

	case PITCH_J:
		cVal.PITCH_J_V = cVal.PITCH_J_V + p;
		break;
	case ROLL_J:
		cVal.ROLL_J_V = cVal.ROLL_J_V + p;
		break;
	case YAW_J:
		cVal.YAW_J_V = cVal.YAW_J_V + p;

		break;
	case GRIP_J:
		cVal.GRIP_J_V = cVal.GRIP_J_V + p;
		break;
	}

	RTXControl::JointMove(j);
}

void RTXControl::RTX_Home(){
	arm_write(ELBOW, SPEED, TOP_SPEED, &rCode);
	arm_write(SHOULDER, SPEED, TOP_SPEED, &rCode);
	arm_write(ZED, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);
	arm_write(YAW, SPEED, TOP_SPEED, &rCode);
	arm_write(GRIP, SPEED, 50, &rCode);

	arm_write(ELBOW, NEW_POSITION, post_init[ELBOW], &rCode);
	arm_write(SHOULDER, NEW_POSITION, post_init[SHOULDER], &rCode);
	arm_write(ZED, NEW_POSITION, post_init[ZED], &rCode);
	arm_write(WRIST1, NEW_POSITION, post_init[WRIST1], &rCode);
	arm_write(WRIST2, NEW_POSITION, post_init[WRIST2], &rCode);
	arm_write(YAW, NEW_POSITION, post_init[YAW], &rCode);
	arm_write(GRIP, NEW_POSITION, post_init[GRIP], &rCode);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	arm_stop(DEAD_STOP, &rCode);
	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::e(int p){
	arm_write(ELBOW, NEW_POSITION, (p*encoder_count[0]), &rCode);       
}
void RTXControl::s(int p){
	arm_write(SHOULDER, NEW_POSITION, (p*encoder_count[1]), &rCode);
}
void RTXControl::z(int p){
	arm_write(ZED, NEW_POSITION, (p*encoder_count[2]), &rCode);
}
void RTXControl::w(int p, int r){
	int w1 = (int)((p + r) / 0.07415);
	int w2 = (int)((p - r) / 0.07415);
	arm_write(WRIST1, NEW_POSITION, w1, &rCode);
	arm_write(WRIST1, NEW_POSITION, w2, &rCode);
}
void RTXControl::y(int p){
	arm_write(YAW, NEW_POSITION, (p*encoder_count[5]), &rCode);
}
void RTXControl::g(int p){
	arm_write(GRIP, NEW_POSITION, (p*encoder_count[6]), &rCode);
}



void rot_z(int t)
{
	double phi = t;

	a[0][0] = cos(phi);
	a[1][0] = -sin(phi);
	a[2][0] = 0;
	a[3][0] = 0;

	a[0][1] = sin(phi);
	a[1][1] = cos(phi);
	a[2][1] = 0;
	a[3][1] = 0;

	a[0][2] = 0;
	a[1][2] = 0;
	a[2][2] = 1;
	a[3][2] = 0;

	a[0][3] = 0;
	a[1][3] = 0;
	a[2][3] = 0;
	a[3][3] = 1;

}

void rot_y(int t)
{
	double phi = t;
	a[0][0] = cos(phi);
	a[1][0] = 0;
	a[2][0] = sin(phi);
	a[3][0] = 0;

	a[0][1] = 0;
	a[2][1] = 0;
	a[3][1] = 0;
	a[1][1] = 1;

	a[0][2] = -sin(phi);
	a[1][2] = 0;
	a[2][2] = cos(phi);
	a[3][2] = 0;

	a[0][3] = 0;
	a[1][3] = 0;
	a[2][3] = 0;
	a[3][3] = 1;
}

void rot_x(int t)
{
	double phi = t;

	a[0][0] = 1;
	a[1][0] = 0;
	a[2][0] = 0;
	a[3][0] = 0;

	a[0][1] = 0;
	a[1][1] = cos(phi);
	a[2][1] = -sin(phi);
	a[3][1] = 0;

	a[0][2] = 0;
	a[1][2] = sin(phi);
	a[2][2] = cos(phi);
	a[3][2] = 0;

	a[0][3] = 0;
	a[1][3] = 0;
	a[2][3] = 0;
	a[3][3] = 1;
}


void mult_a_b(void)
{
	c[0][0] = a[0][0] * b[0][0] + a[1][0] * b[0][1] + a[2][0] * b[0][2] + a[3][0] * b[0][3];
	c[1][0] = a[0][0] * b[1][0] + a[1][0] * b[1][1] + a[2][0] * b[1][2] + a[3][0] * b[1][3];
	c[2][0] = a[0][0] * b[2][0] + a[1][0] * b[2][1] + a[2][0] * b[2][2] + a[3][0] * b[2][3];
	c[3][0] = a[0][0] * b[3][0] + a[1][0] * b[3][1] + a[2][0] * b[3][2] + a[3][0] * b[3][3];

	c[0][1] = a[0][1] * b[0][0] + a[1][1] * b[0][1] + a[2][1] * b[0][2] + a[3][1] * b[0][3];
	c[1][1] = a[0][1] * b[1][0] + a[1][1] * b[1][1] + a[2][1] * b[1][2] + a[3][1] * b[1][3];
	c[2][1] = a[0][1] * b[2][0] + a[1][1] * b[2][1] + a[2][1] * b[2][2] + a[3][1] * b[2][3];
	c[3][1] = a[0][1] * b[3][0] + a[1][1] * b[3][1] + a[2][1] * b[3][2] + a[3][1] * b[3][3];

	c[0][2] = a[0][2] * b[0][0] + a[1][2] * b[0][1] + a[2][2] * b[0][2] + a[3][2] * b[0][3];
	c[1][2] = a[0][2] * b[1][0] + a[1][2] * b[1][1] + a[2][2] * b[1][2] + a[3][2] * b[1][3];
	c[2][2] = a[0][2] * b[2][0] + a[1][2] * b[2][1] + a[2][2] * b[2][2] + a[3][2] * b[2][3];
	c[3][2] = a[0][2] * b[3][0] + a[1][2] * b[3][1] + a[2][2] * b[3][2] + a[3][2] * b[3][3];

	c[0][3] = a[0][3] * b[0][0] + a[1][3] * b[0][1] + a[2][3] * b[0][2] + a[3][3] * b[0][3];
	c[1][3] = a[0][3] * b[1][0] + a[1][3] * b[1][1] + a[2][3] * b[1][2] + a[3][3] * b[1][3];
	c[2][3] = a[0][3] * b[2][0] + a[1][3] * b[2][1] + a[2][3] * b[2][2] + a[3][3] * b[2][3];
	c[3][3] = a[0][3] * b[3][0] + a[1][3] * b[3][1] + a[2][3] * b[3][2] + a[3][3] * b[3][3];
}

void trans_d_a()
{
	a[0][0] = d[0][0];
	a[1][0] = d[1][0];
	a[2][0] = d[2][0];
	a[3][0] = d[3][0];

	a[0][1] = d[0][1];
	a[1][1] = d[1][1];
	a[2][1] = d[2][1];
	a[3][1] = d[3][1];

	a[0][2] = d[0][2];
	a[1][2] = d[1][2];
	a[2][2] = d[2][2];
	a[3][2] = d[3][2];

	a[0][3] = d[0][3];
	a[1][3] = d[1][3];
	a[2][3] = d[2][3];
	a[3][3] = d[3][3];

}
void trans_c_b()
{
	b[0][0] = c[0][0];
	b[1][0] = c[1][0];
	b[2][0] = c[2][0];
	b[3][0] = c[3][0];

	b[0][1] = c[0][1];
	b[1][1] = c[1][1];
	b[2][1] = c[2][1];
	b[3][1] = c[3][1];

	b[0][2] = c[0][2];
	b[1][2] = c[1][2];
	b[2][2] = c[2][2];
	b[3][2] = c[3][2];

	b[0][3] = c[0][3];
	b[1][3] = c[1][3];
	b[2][3] = c[2][3];
	b[3][3] = c[3][3];

}
void trans_c_d()
{
	d[0][0] = c[0][0];
	d[1][0] = c[1][0];
	d[2][0] = c[2][0];
	d[3][0] = c[3][0];

	d[0][1] = c[0][1];
	d[1][1] = c[1][1];
	d[2][1] = c[2][1];
	d[3][1] = c[3][1];

	d[0][2] = c[0][2];
	d[1][2] = c[1][2];
	d[2][2] = c[2][2];
	d[3][2] = c[3][2];

	d[0][3] = c[0][3];
	d[1][3] = c[1][3];
	d[2][3] = c[2][3];
	d[3][3] = c[3][3];
}



void RTXControl::fk()
{
	//Cord Fields
	double fnrotx, fnroty, fnrotz;
	EncoderUpdate();

	b[0][0] = 1;
	b[1][0] = 0;
	b[2][0] = 0;
	b[3][0] = 254;

	b[0][1] = 0;
	b[1][1] = 1;
	b[2][1] = 0;
	b[3][1] = 0;

	b[0][2] = 0;
	b[1][2] = 0;
	b[2][2] = 1;
	b[3][2] = 0;

	b[0][3] = 0;
	b[1][3] = 0;
	b[2][3] = 0;
	b[3][3] = 1;


	//b loaded with translation
	rot_z(cVal.SHOULDER_J_V);   //load a with rotation of shoulder
	mult_a_b();  //c=a*b
	trans_c_d();  //d=c
	//b has translation of z
	b[3][2] = cVal.ZED_J_V;
	b[3][0] = 0;
	rot_z(cVal.ELBOW_J_V);  //loads a with rot
	mult_a_b();  //a*b
	trans_d_a(); //ans to a
	trans_c_b();
	mult_a_b();
	trans_c_d();

	//yaw length
	b[0][0] = 1;
	b[1][0] = 0;
	b[2][0] = 0;
	b[3][0] = 0;

	b[0][1] = 0;
	b[1][1] = 1;
	b[2][1] = 0;
	b[3][1] = 0;

	b[0][2] = 0;
	b[1][2] = 0;
	b[2][2] = 1;
	b[3][2] = 0;

	b[0][3] = 0;
	b[1][3] = 0;
	b[2][3] = 0;
	b[3][3] = 1;

	rot_z(cVal.YAW_J_V);  //loads a with rot
	mult_a_b();  //a*b
	trans_d_a(); //ans to a
	trans_c_b();
	mult_a_b();
	trans_c_d();

	//end effector length
	b[0][0] = 1;
	b[1][0] = 0;
	b[2][0] = 0;
	b[3][0] = 177;

	b[0][1] = 0;
	b[1][1] = 1;
	b[2][1] = 0;
	b[3][1] = 0;

	b[0][2] = 0;
	b[1][2] = 0;
	b[2][2] = 1;
	b[3][2] = 0;

	b[0][3] = 0;
	b[1][3] = 0;
	b[2][3] = 0;
	b[3][3] = 1;


	int theta;
	int w1 = (int)((cVal.PITCH_J_V + cVal.ROLL_J_V) / 0.07415);
	int w2 = (int)((cVal.PITCH_J_V - cVal.ROLL_J_V) / 0.07415);


	theta = -(int)(w1 / 2 + w2 / 2);

	rot_y(theta);  //loads a with rot
	mult_a_b();  //a*b
	trans_d_a(); //ans to a
	trans_c_b();
	mult_a_b();
	trans_c_d();


	//end effector length, rotation
	b[0][0] = 1;
	b[1][0] = 0;
	b[2][0] = 0;
	b[3][0] = 0;

	b[0][1] = 0;
	b[1][1] = 1;
	b[2][1] = 0;
	b[3][1] = 0;

	b[0][2] = 0;
	b[1][2] = 0;
	b[2][2] = 1;
	b[3][2] = 0;

	b[0][3] = 0;
	b[1][3] = 0;
	b[2][3] = 0;
	b[3][3] = 1;



	theta = (int)(w1 + w2);

	rot_y(theta);  //loads a with rot
	mult_a_b();  //a*b
	trans_d_a(); //ans to a
	trans_c_b();
	mult_a_b();
	trans_c_d();

	//show x,y,z position
	kVal.X_DH = (int)c[3][0];
	kVal.Y_DH = (int)c[3][1];
	kVal.Z_DH = (int)c[3][2];

	//and roll, pitch, yaw (equations taken from CRAIG-Introduction to robotics)
	//frn11=c[0][0];frn21=c[0][1];frn31=c[2][0];frn32=c[1][2];frn33=c[2][2];
	fnroty = (360 / (2 * 3.1415927))*atan2(-c[2][0], sqrt(c[0][0] * c[0][0] + c[0][1] * c[0][1]));
	fnrotz = (360 / (2 * 3.1415927))*atan2((c[0][1] / cos(fnroty)), (c[0][0] / cos(fnroty)));
	fnrotx = (360 / (2 * 3.1415927))*atan2((c[1][2] / cos(fnroty)), (c[2][2] / cos(fnroty)));

 		kVal.PITCH_DH = (int) fnroty;
	 	kVal.ROLL_DH  = (int) fnrotx;
	 	kVal.YAW_DH   =  (int) fnrotz;

}


void RTXControl::Oninvcalc()
{
	double alpha2, alpha3, r, a2, a3, theta_s, theta_e, theta1, x2, y2, z2, xl;

	//x2,y2,z2 are values at end of shoulder and elbow (ignore yaw)


	xl = 177 * cos((kVal.PITCH_DH * PI) / 180);
	z2 = 177 * sin((kVal.PITCH_DH * PI) / 180);


	x2 = kVal.X_DH - xl*cos((kVal.YAW_DH * PI) / 180); // changed to * PI from + PI
	y2 = kVal.Y_DH - xl*sin((kVal.YAW_DH * PI) / 180);


	cVal.YAW_J_V = kVal.YAW_DH;

	r = sqrt(x2*x2 + y2*y2);
	a2 = 253;
	a3 = 253;
	alpha2 = acos((r*r + a2*a2 - a3*a3) / (2 * r*a2));
	alpha3 = acos((r*r + a3*a3 - a2*a2) / (2 * r*a3));
	theta1 = atan(y2 / x2);
	theta_s = (360 / (2 * PI))*(theta1 + alpha2);
	cVal.SHOULDER_J_V = (int)theta_s;
	theta_e = -((alpha2 + alpha3) * 360) / (2 * PI);
	cVal.ELBOW_J_V = (int)theta_e;
	cVal.ZED_J_V =(int) (kVal.Z_DH + z2) ;

	//end effector length is 177
}



//TEMP Sequences For Video Demo

void RTXControl::RTX_B1(){
	e(156);
	s(-85);
	z(-450);
	w(0, -25);
	y(24);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTX_Start();

	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::RTX_B2(){
	e(156);
	s(-85);
	z(-450);
	w(0, -25);
	y(24);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	e(5);
	s(75);
	y(-55);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	e(-20);
	s(10);
	z(-450);
	w(0, -90);
	y(-5);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	arm_write(WRIST1, NEW_POSITION, 606, &rCode);      // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, -606, &rCode);      // ROLL DEG VAL = 45
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	e(156);
	s(-85);
	z(-450);
	w(0, -25);
	y(24);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::RTX_B3(){
	arm_write(ELBOW, SPEED, 120, &rCode);
	arm_write(SHOULDER, SPEED, 120, &rCode);
	arm_write(ZED, SPEED, 120, &rCode);
	arm_write(WRIST1, SPEED, 120, &rCode);
	arm_write(WRIST2, SPEED, 120, &rCode);
	arm_write(YAW, SPEED, 120, &rCode);

	//Always Start Method with all Joint Values so it go exactly where you want it to
	e(156);
	s(-85);
	z(-450);
	w(0, -25);
	y(24);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	e(50);
	s(-75);
	z(-450);
	w(0, 45);
	y(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	e(0);
	s(-15);
	w(0, 0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	//Change to different Speeds when Both Roll and Pitch Change

	arm_write(WRIST1, SPEED, 120, &rCode);
	arm_write(WRIST2, SPEED, 100, &rCode);

	s(0);
	w(-24, 60);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	w(0, -30);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//Return to same speed when returning to singe pitch or roll movement
	// ALSO return to same speed before end of Method
	arm_write(WRIST1, SPEED, 120, &rCode);
	arm_write(WRIST2, SPEED, 120, &rCode);

	arm_write(ELBOW, NEW_POSITION, -1610, &rCode);        // ELBOW DEG VAL = -115
	arm_write(SHOULDER, NEW_POSITION, 1305, &rCode);      // SHOULDER DEG VAL = 45
	arm_write(WRIST1, NEW_POSITION, 458, &rCode);       // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, -458, &rCode);      // ROLL DEG VAL = 34
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	arm_write(ELBOW, SPEED, TOP_SPEED, &rCode);
	arm_write(SHOULDER, SPEED, TOP_SPEED, &rCode);
	arm_write(ZED, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);
	arm_write(YAW, SPEED, TOP_SPEED, &rCode);
	arm_write(GRIP, SPEED, 50, &rCode);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	//End Method with these methods
	RTXControl::EncoderReadRTX();
	JointUpdate();
}
void RTXControl::RTX_B4(){
	arm_write(ELBOW, SPEED, 160, &rCode);
	arm_write(SHOULDER, SPEED, 160, &rCode);
	arm_write(ZED, SPEED, 160, &rCode);
	arm_write(WRIST1, SPEED, 160, &rCode);
	arm_write(WRIST2, SPEED, 160, &rCode);
	arm_write(YAW, SPEED, 160, &rCode);



	e(-118);
	s(26);
	z(-450);

	arm_write(WRIST1, NEW_POSITION, -1078, &rCode);
	arm_write(WRIST2, NEW_POSITION, 1078, &rCode);
	y(1);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	arm_write(ELBOW, SPEED, 120, &rCode);
	arm_write(SHOULDER, SPEED, 120, &rCode);
	arm_write(ZED, SPEED, 120, &rCode);
	arm_write(WRIST1, SPEED, 120, &rCode);
	arm_write(WRIST2, SPEED, 120, &rCode);
	arm_write(YAW, SPEED, 120, &rCode);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::RTX_B5(){

}

void RTXControl::RTX_B6(){


}

void RTXControl::RTX_B7(){
	//End Method with these methods
	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::RTX_Lunge(){

	arm_write(ELBOW, NEW_POSITION, -2100, &rCode);      // ELBOW DEG VAL = -150
	arm_write(SHOULDER, NEW_POSITION, 2610, &rCode);      // SHOULDER DEG VAL = 90
	arm_write(WRIST1, NEW_POSITION, 0, &rCode);      // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, 0, &rCode);      // ROLL DEG VAL = 0
	arm_write(YAW, NEW_POSITION, 0, &rCode);      // YAW DEG VAL = 0
	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	arm_write(ELBOW, NEW_POSITION, 0, &rCode);      // ELBOW DEG VAL = 0
	arm_write(SHOULDER, NEW_POSITION, 0, &rCode);      // SHOULDER DEG VAL = 0
	arm_write(WRIST1, NEW_POSITION, -1213, &rCode);      // PITCH DEG VAL = -90
	arm_write(WRIST2, NEW_POSITION, -1213, &rCode);      // ROLL DEG VAL = 0
	arm_write(YAW, NEW_POSITION, 0, &rCode);      // YAW DEG VAL = 0
	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	RTXControl::EncoderReadRTX();
	JointUpdate();

}

void RTXControl::RTX_Sword_Home(int direction){
	//Change to different Speeds when Both Roll and Pitch Change
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, 80, &rCode);

	z(-450);
	s(direction * 45);
	e(direction * -135);
	w(0, -20);
	y(direction * -35);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//Change the values back
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);

}

void RTXControl::RTX_Sword_Swing(int direction){
	//Change to different Speeds when Both Roll and Pitch Change
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, 80, &rCode);

	z(-450);
	s(50);
	e(-95);
	w(-30, 15);
	y(direction * -35);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//Change to different Speeds when Both Roll and Pitch Change
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);
}
void RTXControl::RTX_Parry(){

	arm_write(ELBOW, NEW_POSITION, 0, &rCode);      // ELBOW DEG VAL = 0
	arm_write(SHOULDER, NEW_POSITION, 0, &rCode);      // SHOULDER DEG VAL = 0
	arm_write(ZED, NEW_POSITION, -1350, &rCode);      // ZED mm VAL = -450
	arm_write(WRIST1, NEW_POSITION, 0, &rCode);      // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, 0, &rCode);      // ROLL DEG VAL = 0
	arm_write(YAW, NEW_POSITION, 0, &rCode);      // YAW DEG VAL = 0
	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	arm_write(ZED, NEW_POSITION, -2812, &rCode);      // ZED mm VAL = -750
	arm_write(WRIST1, NEW_POSITION, -1213, &rCode);      // PITCH DEG VAL = -90
	arm_write(WRIST2, NEW_POSITION, -1213, &rCode);      // ROLL DEG VAL = 0
	arm_write(GRIP, NEW_POSITION, 1134, &rCode);      // GRIP mm VAL = 81
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTXControl::EncoderReadRTX();
	JointUpdate();

}

void RTXControl::RTX_Center(){

	arm_write(ZED, NEW_POSITION, -1687, &rCode);      // ZED mm VAL = -450
	arm_write(ELBOW, NEW_POSITION, 2184, &rCode);      // ELBOW DEG VAL = 156
	arm_write(SHOULDER, NEW_POSITION, -2465, &rCode);      // SHOULDER DEG VAL = -85
	arm_write(WRIST1, NEW_POSITION, 337, &rCode);      // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, -337, &rCode);      // ROLL DEG VAL = -25
	arm_write(YAW, NEW_POSITION, 216, &rCode);      // YAW DEG VAL = 24
	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
}

void RTXControl::RTX_Start(){

	e(-89);
	s(45);
	z(-450);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//Change to different Speeds when Both Roll and Pitch Change
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, 80, &rCode);
	w(-45, 5);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	w(-45, -10);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	w(0, 0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//Return to same speed when returning to singe pitch or roll movement
	// ALSO return to same speed before end of Method
	arm_write(WRIST1, SPEED, TOP_SPEED, &rCode);
	arm_write(WRIST2, SPEED, TOP_SPEED, &rCode);

	e(-120);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	arm_write(ZED, NEW_POSITION, -1687, &rCode);      // ZED mm VAL = -450
	arm_write(ELBOW, NEW_POSITION, 2184, &rCode);      // ELBOW DEG VAL = 156
	arm_write(SHOULDER, NEW_POSITION, -2465, &rCode);      // SHOULDER DEG VAL = -85
	arm_write(WRIST1, NEW_POSITION, 337, &rCode);      // PITCH DEG VAL = 0
	arm_write(WRIST2, NEW_POSITION, -337, &rCode);      // ROLL DEG VAL = -25
	arm_write(YAW, NEW_POSITION, 216, &rCode);      // YAW DEG VAL = 24
	arm_write(GRIP, NEW_POSITION, 0, &rCode);      // GRIP mm VAL = 0
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTXControl::EncoderReadRTX();
	JointUpdate();
}

void RTXControl::GetWater(){

	// grab bottle 1st step
	e(0);
	s(0);
	z(-90);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	// bottle Position
	e(15);
	s(89);
	z(-680);
	w(0, 0);
	y(45);
	g(81);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	// bottle Position
	e(15);
	s(89);
	z(-700);
	w(0, 0);
	y(45);
	g(81);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	// drop down on bottle  
	z(-860);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//end Position
	e(0);
	s(8);
	z(-280);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	//end Position
	e(0);
	s(8);
	z(-80);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);


	g(84);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	//End Method with these methods
	RTXControl::EncoderReadRTX();
	JointUpdate();

	z(-200);
	w(-90, 0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);
	//End Method with these methods
	RTXControl::EncoderReadRTX();
	JointUpdate();

}

void RTXControl::GrabWater(){

	e(0);
	s(0);
	z(0);
	w(0, 0);
	y(0);
	g(84);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	// grab bottle 1st step
	e(0);
	s(0);
	z(0);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	// grab bottle 1st step
	e(75);
	s(63);
	z(-740);
	w(0, 0);
	y(63);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	g(84);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	z(-600);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	// grab bottle 1st step
	e(0);
	s(0);
	z(0);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

}

void RTXControl::RTX_Rabbit(){
	e(0);
	s(0);
	z(-450);
	w(0, 0);
	y(0);
	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	w(-90, 0);
	g(81);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	z(-720);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	g(0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	z(-450);
	w(0, 0);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	g(81);
	arm_go(NUMERIC_GO, 0x1555, &rCode);
	waitUntilDone(&rCode);

	RTXControl::EncoderReadRTX();
	JointUpdate();
}