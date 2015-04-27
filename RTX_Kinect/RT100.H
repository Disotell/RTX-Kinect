/*
//-------------------------------------------------------------------------
// RT100 version 1.0 BETA
//
// Send bugs report or comments to miguel@eng.uts.edu.au
//-------------------------------------------------------------------------
*/
#ifndef __RT100_H
#define __RT100_H

#include <windows.h>

/*
// To be used with the arm_go command
// FORWARD = 1+4+16+64+256+1024+4096 = 5397
*/
const int FORWARD = 0x1555;

/*
// Default values set by arm_init
*/
const char MAXFORCE    =     40; // Elbow, Shoulder, Wrist, Yaw and Gripper.
const char MAXFORCEZED =     50; // Max. force for the Zed motor
const char TOP_SPEED   =    100; // Max. speed for all motors

/*
// Arm response codes
*/
const int RT100_OK               =  0;

/*
// Communications faults
*/
const BYTE COMMS_FAULT             = 0x10;
const BYTE COMMS_NOT_READY         = 0x11;
const BYTE COMMS_NOT_INITIALISED   = 0x12;

/*
// Response Errors
*/
const BYTE NO_RESPONSE             = 0x20;
const BYTE RESPONSE_OVERRUN        = 0x21;
const BYTE RESPONSE_INCOMPLETE     = 0x22;
const BYTE RESPONSE_UNKNOWN        = 0x23;

/*
// Arm Warnings
*/
const BYTE ARM_IN_PROGRESS         = 0x30;
const BYTE ARM_STORED              = 0x31;
const BYTE ARM_AXIS_BUSY           = 0x32;
const BYTE ARM_DECODER_BUSY        = 0x33;
const BYTE ARM_PARAMETER_OOR       = 0x34;

/*
// Arm Errors
*/
const BYTE ARM_READ_ONLY           = 0x40;
const BYTE ARM_SELECTION_OOR       = 0x41;
const BYTE ARM_COMMAND_OOR         = 0x42;
const BYTE ARM_NOT_SUPPORTED       = 0x43;
const BYTE ARM_FRAME_TIMEOUT       = 0x44;
const BYTE ARM_FRAME_OVERRUN       = 0x45;
const BYTE ARM_PARITY              = 0x46;
const BYTE ARM_RESTARTED           = 0x47;

/*
// Library Errors
*/
const BYTE SELECTION_OOR           = 0x50;
const BYTE CHECKSUM                = 0x51;
const BYTE READ_ONLY               = 0x52;
const BYTE PARAMETER_OOR           = 0x53;
const BYTE TOGGLE_MODE_OFF         = 0x54;
const BYTE COMMAND_NOT_IMPLEMENTED = 0x55;
const BYTE INVALID_PARAMETER       = 0x56;

typedef int POSITION_T[7];

typedef int GO_BITS_T;

typedef int I_DATA_T[8];

typedef enum PRECISION
           {
             PASSING,
             THEORETICAL,
             ACCURATE
           } PRECISION;

typedef enum TOGGLE_T
           {
             TOGGLE_OFF,
             TOGGLE_ON
           } TOGGLE_T;

typedef enum MOTOR_T
           {
             ELBOW,
             SHOULDER,
             ZED,
             WRIST1,
             WRIST2,
             YAW,
             GRIP,
             ZEDOWN
           } MOTOR_T;

typedef enum MOTOR_MODE_T
           {
             FORCE_MODE,
             POSITION_MODE,
             ABSOLUTE_MODE,
             RELATIVE_MODE,
             USER_INPUT,
             USER_OUTPUT
           } MOTOR_MODE_T;

typedef enum STOP_MODE_T
           {
             DEAD_STOP,
             RAMP_STOP,
             FREE_STOP,
             FREE_OFF
           } STOP_MODE_T;

typedef enum GO_MODE_T
           {
             MANUAL_GO,
             NUMERIC_GO
           } GO_MODE_T;

typedef enum DATA_CODE_T
           {
             CP_ERROR,
             CURRENT_POSITION,
             ERROR_LIMIT,
             NEW_POSITION,
             SPEED,
             KP,
             KI,
             KD,
             DEAD_BAND,
             OFFSET,
             MAX_FORCE,
             CURRENT_FORCE,
             ACCELERATION_TIME,
             USER_RAM,
             USER_IO,
             ACTUAL_POSITION
           } DATA_CODE_T;

typedef enum SOAK_T
           {
             INIT,
             SOAK_ON,
             INIT_SOAK,
             SOAK_OFF
           } SOAK_T;

typedef enum IP_T
           {
             IP0,
             IP1
           } IP_T;

#ifdef __cplusplus
extern "C" {
#endif

/*
//-------------------------------------------------------------------------
//  Functions for preparing the arm
//-------------------------------------------------------------------------
/*
//  arm_init_comms
//
//  This is the first function to be used in the program to initialise
//  the communication port
//
//  CommPort: "COM1", "COM2", ...
//
//  Note: Use arm_shutdown to close the communication port.
//
*/
void arm_init_comms(char* CommPort, int* rCode);

/*
//
//  arm_init
//
//  This function initialises the arm and set arm default values.
//
*/
void arm_init(int* rCode);


/*
//
//  arm_init_IPx
//
//  This function initialise a specific robot arm controller.
//
//  This function is usefull when one of the motors has not been
//  initialised properly.
//
//  IP0: controls: Wrist 1 & 2 (pitch and roll)
//  IP1: controls: Shoulder, elbow, zed and yaw
//
*/
void arm_init_IPx(IP_T IPx, int* rCode);


/*
//
//  arm_shutdown
//
//  This is the last function to be used in a program.
//  It closes the communication port and switches off the power to all
//  the motors and also to the vacuum plunger.
//
*/
void arm_shutdown (int* rCode);


/*
//  arm_reload_pids
//
//  Resets all the control parameter stored in the IPs to their default
//  values.
//
*/
void arm_reload_pids (int* rCode);

/*
//  arm_restart
//
//  Resets all the parameter stored in the IPs to their default values.
//  It is an unconditional reset, and so stops the arm immediately.
//  It is used following a power failure on the IP board, it re-establishes
//  communications and turn the motors on.
//
*/
void arm_restart (int* rCode);


/*
//  arm_define_origin
//
//  Zeros each motor's position counter and error, so that the current arm
//  position becomes the home position.
//
//  No other variables are affected.
//
*/
void arm_define_origin (int* rCode);

/*
//  arm_version
//
//  Returns the library version and revision numbers.
//  The ROM version is returned from the IP as 256*the version number,
//  plus the revision number.
//
*/
void arm_version (int* library_version, int* library_revision,
                  int* ROM_version, int* rCode);

/*
//-------------------------------------------------------------------------
//  Function for defining the motion of the arm
//-------------------------------------------------------------------------
//
//  arm_set_mode
//
//  Sets any motor into one of the three motor modes: Force, Position, I/O.
//  It can also set the user's input/output lines to input or output.
//
//  Example:
//  arm_set_mode (ZED, USER_INPUT, &rCode);
//
//  Sets the six user input/output lines on the IP which controls the ZED
//  motor to input, that is, the lines on IP0.
//  The motor you specify is only used to select the IP when you have
//  USER_INPUT or USER_OUTPUT as the motor mode with the SET_MODE keyword.
//
*/
void arm_set_mode (MOTOR_T motor, MOTOR_MODE_T mode, int* rCode);

/*
//  arm_read
//
//  Reads a control parameter associated with a motor from the IP,
//  and copies the data into an integer variable.
//
//  Example:
//  arm_read (ZED, DEAD_BAND, &Z_DEAD_BAND, &rCode);
//
//  Reads the parameter DEAD_BAND associated with the ZED motor,
//  puts the result into Z_DEAD_BAND, and puts and error number in the
//  variable rCode.
//
*/
void arm_read (MOTOR_T motor, DATA_CODE_T code, int* data, int* rCode);

/*
//  arm_write
//
//  Writes an integer to a data code in the IP associated with a
//  particular motor.
//
*/
void arm_write (MOTOR_T motor, DATA_CODE_T code, int data, int* rCode);

/*
//-------------------------------------------------------------------------
//  Function for initialising movement of the arm
//-------------------------------------------------------------------------
//
//  arm_go
//
//  Sets one or more motors moving in one of the three drive modes.
//  The GO_BITS variable is an integer arranged as a sequence of 14 control
//  bits, two for each motor. The control bits are:
//
//  00  power stop
//  01  forward
//  10  reverse
//  11  free stop (unpowered)
//
//  The table in the following page gives the decimal number required for
//  any motion of any motor.
//  To use the table:
//
//  1- For each  motor, find the associated number to set it moving in the
//  way you required. Don't take more than one number from each row of the
//  table.
//
//  2- Add together all the numbers you have found, and enter the result in
//  the GO_BITS variable.
//
//  motor   stop  forward   reverse   free stop
//  G       0     4096      8192      12288
//  Y       0     1024      2048      3072
//  W1      0     256       512       768
//  W2      0     64        128       192
//  Z       0     16        32        48
//  S       0     4         8         12
//  E       0     1         2         3
//
//  In manual mode, the motors are controlled directly from the GO_BITS.
//  In numeric mode, only 01 and 10 have any effect, and they both start
//  the motor moving to a new position.
//
//  Example:
//  arm_go (NUMERIC, 5, &rCode);
//
//  5 is equivalent to the binary number 0101, so this command sets motors
//  E and S moving in numeric mode, and puts the error code into the
//  variable rCode.
//
//  If you want to:
//    move the wrist yaw motor forward
//    move the wrist 2 motor backwards
//    make a free stop of the elbow motor
//    make a power stop of all the other motors
//
//  The numbers are:  1024 + 128 + 3 + 0 = 1155.
//  Therefore the command needed is: arm_go (NUMERIC, 1155, &rCode);
//
//  NOTE:
//   Normally the arm_go command is used as folows:
//    arm_go( NUMERIC_GO, FORWARD, &rCode );
//
*/
void arm_go (GO_MODE_T mode, GO_BITS_T bits, int* rCode);

/*
//  arm_interpolate
//  Moves each motor by the increment given in the array.
//  Your program would call the array repeatedly, to produce a series of
//  small movements.
//
//  Note that in order to achieve the correct data transmission rate,
//  the toggle mode has to be set to toggle on (see init_comms).
//
//  Example:
//  arm_interpolate (movement_table, &rCode);
//
//  Moves the arm following the instructions in the variable movement_table,
//  and puts an error number in the variable rCode.
//
*/
void arm_interpolate (I_DATA_T* data, int* rCode);

/*
//-------------------------------------------------------------------------
//  Functions for getting status report from the arm
//-------------------------------------------------------------------------
//
//  arm_general_status
//
//  Checks the current arm status and puts the result into the integer
//  variable you specify. The result contains the following flags
//  (for bits 0 to 4, 1 means true, 0 false):
//
//  Bit 0 any tasks still in progress?
//  Bit 1 has any axis stopped?
//  Bit 2 any error limits exceeded
//  Bit 3 has any axis been reset?
//  Bit 4 has there been a change in the state of one of the I/O lines?
//  Bit 5 user configuration for IP0: 1 means input, 0 means output
//  Bit 6 user configuration for IP1: 1 means input, 0 means output
//  Bit 7 0
//
//  Example:
//  arm_general_status (&status, &rCode);
//
//  Check the general status of the arm, puts the result in the variable
//  status, and an error number in the variable rCode.
//
*/
void arm_general_status (int* status, int* rCode);

/*
//  arm_motor_status
//
//  Checks the current status of one particular motor and puts the result
//  into the integer variable you specify. The byte contains the following
//  flags (for bits 0 to 3, 1 means true, 0 false):
//
//  Bit 0 tasks still in progress?
//  Bit 1 axis stopped?
//  Bit 2 error limit exceeded?
//  Bit 3 Axis reset? (Usually means that MAX_FORCE isn't enough to
//                     overcome an obstacle).
//  Bit 4 motor mode:
//                    0 absolute
//                    1 relative
//  Bit 5 motor mode:
//                    0 position
//                    1 force
//  Bit 6 0
//  Bit 7 0
//
//  Example:
//  arm_motor_status (Z, &Z_STATUS, &rCode);
//
//  Checks the status of the motor z, puts the result in the variable
//  Z_STATUS and an error number in the variable rCode.
//
*/
void arm_motor_status (MOTOR_T motor, int* status, int* rCode);

/*
//-------------------------------------------------------------------------
//  Function for stopping the arm
//-------------------------------------------------------------------------
//
//  arm_stop
//
//  Emergency stop: stops all the motors, leaving the variables unchanged.
//
//  There are three stopping methods:
//
//  DEAD  Stops all the motors instantly and leaves them locked in position.
//
//  RAMP  Stops all the motors gradually but leaves them locked in position.
//
//  FREE  The safest way to stop: the motor's power is switched off,
//        but the axes are left free so that you can move them manually.
//
//  FREE_OFF  Allows power to be sent to the motors after STOP has been
//  used, or after a RESTART.
//
//  Example:
//  arm_stop (FREE, &rCode);
//
//  Stops all the motors using the free motors method, and puts the error
//  number into the variable rCode.
//
*/
void arm_stop (STOP_MODE_T mode, int* rCode);

/*
//-------------------------------------------------------------------------
//  Function for using IPC directly
//-------------------------------------------------------------------------
//
//  arm_raw_command
//
//  IP and nbytes are integer type variables.
//  IP can be 0 or 1;
//  nbytes can be 1 to 3.
//
//  It sends a raw IPC command to a particular IP. Valid IPC commands have
//  one or three bytes.
//
//  Example:
//  arm_raw_command (IP0, 1, 0x08, 0x00, 0x00, &rCode);
//  arm_raw_command (IP1, 1, 0x08, 0x00, 0x00, &rCode);
//
//  Sends the init command to IP0 abd IP1, and puts a return code
//  in the variable rCode.
//
*/
void arm_raw_command (IP_T IP, int nbytes, int byte1,
                      int byte2, int byte3, int* rCode);

/*
//  arm_raw_response
//
//  IP and nbytes are integer type variables.
//  IP can be 0 or 1;
//  nbytes can be 1 to 3.
//
//  Gets information in raw form, as either one or three bytes.
//
//  Example:
//  arm_raw_response (0, &number_of_bytes, &rb1, &rb2, &rb3, &rCode);
//
//  Gets a response from IP0, puts the number of the response bytes into
//  number_of_bytes and the bytes themselves into rb1, rb2, rb3.
//  An error number is returned in the variable rCode.
//
//  Note: arm_raw_command and arm_raw_response have to be used together,
//  you can't have an isolated command or response. The way you set up the
//  bytes to control the arm is explained in the RT100 user's manual
//  Using Intelligent Peripherals Communications, section E.
//
*/
void arm_raw_response (IP_T IP, int* nbytes, int* byte1,
                       int* byte2, int* byte3, int* rCode);


/*
//-------------------------------------------------------------------------
//  Extra functions
//-------------------------------------------------------------------------
//
//  arm_vacuum_plunger
//
//  To turn ON/OFF the vacuum plunger.
//
*/
void arm_vacuum_plunger(TOGGLE_T mode, int* rCode);

/*
//  arm_has_power
//
//  To check if the arm has power. It can be used to check if the emergency
//  stop button is pressed.
//
// Returns: 0 when power is OFF and 1 otherwise.
//
*/
int arm_has_power(int* rCode);

#ifdef __cplusplus
}
#endif

#endif  // __RT100_H



