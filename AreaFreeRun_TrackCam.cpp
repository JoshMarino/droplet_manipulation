////////////////////////////////////////////////////////////////////////////
// ME4 Frame grabber example
//
//
//
// File:	AreaFreeRun.cpp
//
// Copyrights by Silicon Software 2002-2010
////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <conio.h>
#include "pfConfig.h"

#include <stdio.h>
#include <windows.h>
#include <string>       // std::string
#include <iostream>     // std::cout, std::ostream, std::hex
#include <sstream>      // std::stringbuf
#include <time.h>		// Sleep(milliseconds)

#include "board_and_dll_chooser.h"

#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <math.h>



//#define NUMTESTS	5				// Number of tests to run and record: NOT IN USE
//#define NUMIMAGES	1000			// Number of images to grab in one sequence
#define WIDTH		104				// 4-8192 in increments of 4 -> TrackCam max of 1024
#define HEIGHT		175				// 1-8192 in increments of 1 -> TrackCam max of 1024
#define CAMTYP		108				// 8: 8-bit Single Tap, 10: 10-bit Single Tap, 12: 12-bit Single Tap, 14: 14-bit Single Tap, 16: 16-bit Single Tap, 
									// 108: 8-bit Dual Tap, 110: 10-bit Dual Tap, 112: 12-bit Dual Tap

#define EXPOSURE	0.199			// Exposure of TrackCam (ms)
#define FRAMETIME	0.57			// Frame time of TrackCam (ms)

#define TRIGMODE	ASYNC_TRIGGER	// 0: Free Run, 1: Grabber Controlled, 2: Extern Trigger, 4: Software Trigger
#define EXPOSUREMS	20				// Exposure used for frame grabber (not sure what it really does - made as small as possible)
#define TRIGSOURCE	0				// 0-3: Trigger Source 0-3 (respectively)

#define BUFLEN		45000			// Buffer length allowable on computer for frame grabber storage
#define nBoard		0				// Board number
#define NCAMPORT	PORT_A			// Port (PORT_A / PORT_B)

#define VIDEOFPS	5				//VideoWriter FPS
#define ISCOLOR		0				// VideoWriter color [0:grayscale, 1:color]

//#define PULSETIME	850				// Pulsetime for piezoelectric disk, [us]

//#define FPS			1750			// TrackCam FPS

#define PHASE 		0				// droplet contact phase relationship to vertical bouncing, 0-100
#define AMPL 		11				// shaking amplitude, [m/s^2]
#define FREQ 		32				// shaking frequency, [Hz]
#define FIXEDDIST	4.95			// distance from bottom of nozzle to top of fluid bath without shaking, [mm]
#define GRAVITY		9.81			// gravitational acceleration, [m/s^2]
#define ADJUST		(-5)				// frames to adjust for droplet trajectory
									// positive: drop earlier

#define ComPortPIC	L"COM7"			// Serial communication port
									//	- neccessary to add L in front for wide character
#define ComPortGUI	L"COM6"			// Serial communication port
									//	- neccessary to add L in front for wide character

using namespace cv;
using namespace std;


int ErrorMessage(Fg_Struct *fg)
{
	int			error	= Fg_getLastErrorNumber(fg);
	const char*	err_str = Fg_getLastErrorDescription(fg);
	fprintf(stderr,"Error: %d : %s\n",error,err_str);
	return error;
}


int ErrorMessageWait(Fg_Struct *fg)
{
	int			error	= ErrorMessage(fg);
	printf (" ... press ENTER to continue\n");
	getchar();
	return error;
}


/* HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const wchar_t* m_pszPortName)

Before starting any communication on a serial port, we must first open a connection, in this case in non-overlapped mode. 
This is achieved by using CreateFile function in Win32.
	The CreateFile function takes in seven parameters. 
		1) Specifies the port name. In our case, this is usually COM, COM2, COM3 or COM4.
		2) GENERIC_READ | GENERIC_WRITE to support both read and write access.
		3) Must always be 0 for serial port communication because unlike files, serial port access cannot be shared.
		4) Used to set security attributes. If no security attribute needs to be specified, just use NULL.
		5) Must always be set to OPEN_EXISTING.
		6) Used to specify flags and attributes (either 0 or FILE_ATTRIBUTE_NORMAL can be used).
		7) must always be NULL as we only support non-overlapped communication.
The HANDLE m_hSerialComm that is returned by the CreateFile function can now be used for performing operations like Configure, Read and Write.

After opening connection to a serial port, the next step is usually to configure the serial port connect settings like Baud Rate, Parity Checking, Byte Size, Error Character, EOF Character etc. 
Win32 provides a DCB struct that encapsulates these settings. 
Configuration of the serial port connection settings is performed in the following three steps:
	1) First, we have to access the present settings of the serial port using the GetCommState function. The function takes in two parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Output parameter, which returns the DCB structure containing the present settings.
	2) Next, using the DCB structure that we obtained from the previous step, we can modify the necessary settings according to the application needs.
	3) Finally, we update the changes by using the SetCommState method.

Another important part of configuration of serial port connection settings is setting timeouts. 
Again, Win32 provides a COMMTIMEOUTS struct for setting Read and Write Timeouts. 
We are also provided with two functions GetCommTimeouts and SetCommTimeouts to access, modify, and update the timeout settings. 

*/
HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const wchar_t* m_pszPortName) {

	// Attempt to open a connection on COM7 in non-overlapped mode
	m_hSerialComm = CreateFile(m_pszPortName, 
            GENERIC_READ | GENERIC_WRITE, 
            0, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL);

	// Check to see if connection was established
	if (m_hSerialComm == INVALID_HANDLE_VALUE) {  
		printf("Handle error connection.\r\n");
	}
	else {
		printf("Connection established.\r\n");
	}
	

	// DCB (device control block) structure containing the current settings of the port
	DCB dcbConfig;
	
	// Fill all fields of DCB with zeros
	FillMemory(&dcbConfig, sizeof(dcbConfig), 0);

	// Get current settings of the port
	if(!GetCommState(m_hSerialComm, &dcbConfig))
	{
		printf("Error in GetCommState.\r\n");
	}

	// Configuration of serial port
	dcbConfig.BaudRate = 230400;						// 230400 baud rate
	dcbConfig.Parity = NOPARITY;						// No parity
	dcbConfig.ByteSize = 8;								// 8 data bits
	dcbConfig.StopBits = ONESTOPBIT;					// 1 stop bit
	dcbConfig.fRtsControl = RTS_CONTROL_HANDSHAKE;		// RequestToSend handshake
	dcbConfig.fBinary = TRUE;							// Windows API does not support nonbinary mode transfers, so this member should be TRUE
	dcbConfig.fParity = TRUE;							// Specifies whether parity checking is enabled.
	
	// Set new state of port
	if(!SetCommState(m_hSerialComm, &dcbConfig)) {
		// Error in SetCommState
		printf("Error in SetCommState. Possibly a problem with the communications port handle or a problem with the DCB structure itself.\r\n");
	}
	printf("Serial port COM7 configured.\r\n");

	
	// COMMTIMEOUTS struct for setting Read and Write Timeouts
	COMMTIMEOUTS commTimeout;

	// Get current settings of the Timeout structure
	if(!GetCommTimeouts(m_hSerialComm, &commTimeout))
	{
		printf("Error in GetCommTimeouts.\r\n");
	}
	
	// Set Read and Write Timeouts
	commTimeout.ReadIntervalTimeout				= 20;
	commTimeout.ReadTotalTimeoutConstant		= 10;
	commTimeout.ReadTotalTimeoutMultiplier		= 100;
	commTimeout.WriteTotalTimeoutConstant		= 10;
	commTimeout.WriteTotalTimeoutMultiplier		= 100;

	// Set new state of CommTimeouts
	if(!SetCommTimeouts(m_hSerialComm, &commTimeout)) {
		// Error in SetCommTimeouts
		printf("Error in SetCommTimeouts. Possibly a problem with the communications port handle or a problem with the COMMTIMEOUTS structure itself.\r\n");
	}
	printf("Serial port COM7 set for Read and Write Timeouts.\r\n");

	return m_hSerialComm;

}


/* std::string ReadSerialPort (HANDLE m_hSerialComm)

First, we setup a Read Event using the SetCommMask function. This event is fired when a character is read and buffered internally by Windows Operating System. 
	The SetCommMask function takes in two parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Specify the event type. To specify a Read Event, we use EV_RXCHAR flag.
After calling the SetCommMask function, we call the WaitCommEvent function to wait for the event to occur. 
	This function takes in three parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Output parameter, which reports the event type that was fired.
		3) NULL for our purpose since we do not deal with non-overlapped mode
Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally. 
	The ReadFile function takes in five parameters:
		1) HANDLE we received from the call to the CreateFile function.
		2) Buffer that would receive the bytes if the ReadFile function returns successfully.
		3) Specifies the size of our buffer we passed in as the second parameter.
		4) Output parameter that will notify the user about the number of bytes that were read.
		5) NULL for our purpose since we do not deal with non-overlapped mode.

In our example, we read one byte at a time and store it in a temporary buffer. 
This continues until the case when ReadFile function returns successfully and the fourth parameter has a value of 0. 
This indicates that the internal buffer used by Windows OS is empty and so we stopping reading. 

*/
std::string ReadSerialPort (HANDLE m_hSerialComm) {

	// String buffer to store bytes read and dwEventMask is an output parameter, which reports the event type that was fired
	std::stringbuf sb;
	DWORD dwEventMask;

	// Setup a Read Event using the SetCommMask function using event EV_RXCHAR
	if(!SetCommMask(m_hSerialComm, EV_RXCHAR)) {
		printf("Error in SetCommMask to read an event.\r\n");
	}

	// Call the WaitCommEvent function to wait for the event to occur
	if(WaitCommEvent(m_hSerialComm, &dwEventMask, NULL))
	{
		//printf("WaitCommEvent successfully called.\r\n");
		char szBuf;
		DWORD dwIncommingReadSize;
		DWORD dwSize = 0;

		// Once this event is fired, we then use the ReadFile function to retrieve the bytes that were buffered internally
		do
		{
			if(ReadFile(m_hSerialComm, &szBuf, 1, &dwIncommingReadSize, NULL) != 0) {
				if(dwIncommingReadSize > 0)	{
					dwSize += dwIncommingReadSize;
					sb.sputn(&szBuf, dwIncommingReadSize);
				}
			}
			else {
				//Handle Error Condition
				printf("Error in ReadFile.\r\n");
			}
		} while(dwIncommingReadSize > 0);
	}
	else {
		//Handle Error Condition
		printf("Could not call the WaitCommEvent.\r\n");
	}

	return sb.str();

}


/* void WriteSerialPort (HANDLE m_hSerialComm)

Write operation is easier to implement than Read. It involves using just one function, WriteFile. 
	It takes in five parameters similar to ReadFile function.
		1) HANDLE we received from the call to the CreateFile function.
		2) Specifies the buffer to be written to the serial port.
		3) Specifies the size of our buffer we passed in as the second parameter.
		4) Output parameter that will notify the user about the number of bytes that were written.
		5) NULL for our purpose since we do not deal with non-overlapped mode.
		
The following example shows implementation of Write using WriteFile. 
It writes one byte at a time until all the bytes in our buffer are written.

*/
void WriteSerialPort (HANDLE m_hSerialComm, char message[]) {

	unsigned long dwNumberOfBytesSent = 0;

	// Append return carriage and new line to end of message.
	// Code adopted from: http://stackoverflow.com/questions/9955236/append-to-the-end-of-a-char-array-in-c
    char temp1[] = "\r\n";
    char * pszBuf = new char[std::strlen(message)+std::strlen(temp1)+1];
    std::strcpy(pszBuf,message);
    std::strcat(pszBuf,temp1);

	// Size of the buffer pszBuf ("message\r\n")
	DWORD dwSize = (unsigned)strlen(pszBuf);

	// Continue writing until all bytes are sent
	while(dwNumberOfBytesSent < dwSize)
	{
		unsigned long dwNumberOfBytesWritten;

		// Write to COM7 whatever is in pszBuf[]
		if(WriteFile(m_hSerialComm, &pszBuf[dwNumberOfBytesSent], 1, &dwNumberOfBytesWritten, NULL) != 0)
		{
			// Increment number of bytes sent
			if(dwNumberOfBytesWritten > 0) {
				++dwNumberOfBytesSent;
			}
			else {
				//Handle Error Condition
				printf("Error in BytesWritten.\r\n");
			}
		}

		else {
			//Handle Error Condition
			printf("Error in WriteFile.\r\n");
		}
	}
	//std::cout << "\r\nLine written to COM7: " << pszBuf << "\r\n";

}


/* void CloseSerialPort (HANDLE m_hSerialComm)

After we finish all our communication with the serial port, we have to close the connection. 
This is achieved by using the CloseHandle function and passing in the serial port HANDLE we obtained from the CreateFile function call. 
Failure to do this results in hard to find handle leaks.

*/
void CloseSerialPort (HANDLE m_hSerialComm) {

	if(m_hSerialComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hSerialComm);
		m_hSerialComm = INVALID_HANDLE_VALUE;
	}
	printf("Serial port closed.\r\n");

}

const char *dll_list_me3[] = {
#ifdef MEUNIX
		"libSingleAreaGray.so",
		"libDualAreaRGB.so",
		"libMediumAreaGray.so",
		"libMediumAreaRGB.so",
#else
		"SingleAreaGray.dll",
		"DualAreaRGB.dll",
		"MediumAreaGray.dll",
		"MediumAreaRGB.dll",
#endif
		""
};

const char *dll_list_me3xxl[] = {
#ifdef MEUNIX
		"libDualAreaGray12XXL.so",
		"libDualAreaRGB36XXL.so",
		"libMediumAreaGray12XXL.so",
		"libMediumAreaRGBXXL.so",
#else
		"DualAreaGray12XXL.dll",
		"DualAreaRGB36XXL.dll",
		"MediumAreaGray12XXL.dll",
		"MediumAreaRGBXXL.dll",
#endif
		""
};


const char *dll_list_me4_dual[] = {
#ifdef MEUNIX
	"libDualAreaGray16.so",
		"libDualAreaRGB48.so",
		"libMediumAreaGray16.so",
		"libMediumAreaRGB48.so",
#else
		"DualAreaGray16.dll",
		"DualAreaRGB48.dll",
		"MediumAreaGray16.dll",
		"MediumAreaRGB48.dll",
#endif
		""
};

const char *dll_list_me4_single[] = {
#ifdef MEUNIX
	"libSingleAreaGray16.so",
	"libSingleAreaRGB48.so",
#else
	"SingleAreaGray16.dll",
	"SingleAreaRGB48.dll",
#endif
	""
};






int main(int argc, char* argv[], char* envp[]){


/*
Note: Frame grabber must be re-opened/closed for taking more than one AVI in the same script
	- Might be a work around if can allocate the correct memory and change the location for each AVI
*/



	// =================================================================================================================
	// Configuration of serial port: COM7 (Lower USB port on front panel of computer) to talk to PIC
	// =================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommPIC = {0};

	// Serial communication using port COM7
	const wchar_t* m_pszPortNamePIC = ComPortPIC;

	// Configure serial port
	m_hSerialCommPIC = ConfigureSerialPort(m_hSerialCommPIC, m_pszPortNamePIC);



	// =================================================================================================================
	// Configuration of serial port: COM6 (virtual COM port pair 5/6 - com0com program) to talk with GUI
	// =================================================================================================================

	// Create empty handle to serial port
	HANDLE m_hSerialCommGUI = {0};

	// Serial communication using port COM7
	const wchar_t* m_pszPortNameGUI = ComPortGUI;

	// Configure serial port
	m_hSerialCommGUI = ConfigureSerialPort(m_hSerialCommGUI, m_pszPortNameGUI);



	/*
	// =================================================================================================================
	// Initialization of serial port: COMX to talk to PPOD
	// =================================================================================================================

	*/



	/*
	// =================================================================================================================
	// Camera configuration
	// =================================================================================================================
	int numOfPorts, error, i, numProperties;
	const char *pErr;
	//long width, height;
	float fVal;

	//port of camera
	//camPort = 0;

	
	// ************ Camera configuration **************************** //

	// Init of pfConfig
	numOfPorts = pfGetNumPort();


	// Open Camera on port camPort (Port0)
	printf("Open camera on port %d...\n", nBoard);
	error = pfOpenPort(nBoard, true);
	if(error != 0){
		pErr = pfGetErrorMsg(nBoard);
		printf("Opening camera port failed: %s \nSorry we must quit.\n", pErr);
		return 0;
	}

	printf("Camera configuration...");


	// Set ROI and offset
	//width = 104;
	//height = 175;

	error = pfSetCameraPropertyI(nBoard, "Window.W", WIDTH);
	error = pfSetCameraPropertyI(nBoard, "Window.X", 0);			//no offset
	error = pfSetCameraPropertyI(nBoard, "Window.H", HEIGHT);
	error = pfSetCameraPropertyI(nBoard, "Window.Y", 0);			// no offset


	// Attempt to set Free Running Trigger
	int pp = 1;
	error = pfSetCameraPropertyI(nBoard, "Trigger.Source.Free", pp);	


	// Set exposure time
	fVal = EXPOSURE; //ms
	error = pfSetCameraPropertyF(nBoard, "ExposureTime", fVal);

	// Set frame time for 1,750 fps
	fVal = FRAMETIME; //ms
	error = pfSetCameraPropertyF(nBoard, "FrameTime", fVal);

	
	// Close camera
	printf("\nClose camera on port %d...\n", nBoard);
	error = pfClosePort(nBoard);
	*/

	

	// =================================================================================================================
	// Initialization of frame grabber and camera properties
	// =================================================================================================================



	int nr_of_buffer	=	BUFLEN;						// Number of memory buffer
	//int nBoard			=	selectBoardDialog();	// Board number
	int nCamPort		=	NCAMPORT;					// Port (PORT_A / PORT_B)
	int status = 0;
	Fg_Struct *fg;
	
	const char *dllName = NULL;

	int boardType = 0;

	boardType = Fg_getBoardType(nBoard);

	printf("\r\nBoard ID  %d: MicroEnable III (a3)\r\n", nBoard);
	printf("\r\n=====================================\r\n\r\n");
	
	//dllName = selectDll(boardType, dll_list_me3, dll_list_me3xxl, dll_list_me4_dual, dll_list_me4_single);
	dllName = "SingleAreaGray.dll";		//Uncomment to selectDLL


	/*
	// =================================================================================================================
	// Initialization of the microEnable frame grabber
	// =================================================================================================================
	if((fg = Fg_Init(dllName,nBoard)) == NULL) {
		status = ErrorMessageWait(fg);
		return status;
	}
	fprintf(stdout,"Init Grabber: ok\n");



	// =================================================================================================================
	// Setting the image size and camera format
	// =================================================================================================================
	int width	= WIDTH;
	int height	= HEIGHT;
	int CamTyp	= CAMTYP;

	if (Fg_setParameter(fg,FG_WIDTH,&width,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}
	if (Fg_setParameter(fg,FG_HEIGHT,&height,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}
	if (Fg_setParameter(fg,FG_CAMERA_LINK_CAMTYP,&CamTyp,nCamPort) < 0) {
		status = ErrorMessageWait(fg);
		return status;
	}

	fprintf(stdout,"Set Image Size on port %d (w: %d,h: %d): ok\n",nCamPort,width,height);
	fprintf(stdout,"Set CameraLink Format on port %d to %d (108 == 8-bit Dual Tap): ok\n",nCamPort,CamTyp);



	// =================================================================================================================
	// Memory allocation
	// =================================================================================================================
	int format;
	Fg_getParameter(fg,FG_FORMAT,&format,nCamPort);
	size_t bytesPerPixel = 1;
	switch(format){
	case FG_GRAY:	bytesPerPixel = 1; break;
	case FG_GRAY16:	bytesPerPixel = 2; break;
	case FG_COL24:	bytesPerPixel = 3; break;
	case FG_COL32:	bytesPerPixel = 4; break;
	case FG_COL30:	bytesPerPixel = 5; break;
	case FG_COL48:	bytesPerPixel = 6; break;
	}
	size_t totalBufSize = width*height*nr_of_buffer*bytesPerPixel;
	dma_mem *pMem0;
	if((pMem0 = Fg_AllocMemEx(fg,totalBufSize,nr_of_buffer)) == NULL){
		status = ErrorMessageWait(fg);
		return status;
	} else {
		fprintf(stdout,"%d framebuffer allocated for port %d: ok\n",nr_of_buffer,nCamPort);
	}

	unsigned char * buf[NUMIMAGES];
	for(int i=0;i<NUMIMAGES;i++)
	{
		buf[i] = (unsigned char *) malloc(width*height);
	}



	
	// =================================================================================================================
	// Setting the trigger and grabber mode for Free Run
	// =================================================================================================================
	int	nTriggerMode		= FREE_RUN;

	if(Fg_setParameter(fg,FG_TRIGGERMODE,&nTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	
	

	// =================================================================================================================
	// Setting the trigger and grabber mode for Extern Trigger
	// =================================================================================================================
	int		nTriggerMode		= TRIGMODE;	
	int	    nExposureInMicroSec	= EXPOSUREMS;
	int		nTrgSource			= TRIGSOURCE;				//0-3: Trigger Source 0-3 (respectively)
	int		nExSyncEnable		= 0;						//0: OFF, 1: ON

	if(Fg_setParameter(fg,FG_TRIGGERMODE,&nTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_EXPOSURE,&nExposureInMicroSec,nCamPort)<0)	{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_TRIGGERINSRC,&nTrgSource,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	
	
	if((Fg_AcquireEx(fg,nCamPort,GRAB_INFINITE,ACQ_STANDARD,pMem0)) < 0){
		status = ErrorMessageWait(fg);
		return status;
	}
	*/


	
	/*
	// =============================================================================================
	// Calculate DELAYTIME to delay droplet creation to achieve phase relationship
	// =============================================================================================

	// Determine the vertical sine wave amplitude based on the shaking amplitude/frequency
	//		rho = A*w^2/g
	//		Sine_Amplitude = rho*g/w^2
	//		Sine_Amplitude = rho*g/(2*pi*f)^2
	double SineAmplitude = (AMPL) / (4.0*M_PI*M_PI*FREQ*FREQ);
	//printf("\nSine Amplitude (mm): %f\n", SineAmplitude*1000.0);

	// Calculate the height the droplet will fall before contacting the bath
	double DropHeight = (FIXEDDIST/1000.0) - SineAmplitude*sin( 2.0*M_PI*(PHASE/100.0) );
	//printf("\nDrop Height (mm): %f\n", DropHeight*1000.0);

	// Once know the drop height, determine the drop time
	//		H = (1/2)*g*t^2
	double DropTime, Temp;
	Temp = (2.0*DropHeight)/GRAVITY;
	DropTime = sqrt( Temp );
	//printf("\nDrop Time (sec): %f\n", DropTime);
	
	// Calculate the corresponding number of frames droplet will be falling before contacting sine wave
	int FramesFalling = FPS * DropTime;
	//printf("\nFrames Falling: %d\n", FramesFalling);

	// With number of frames droplet will be falling, determine how many frames from the beginning to create the droplet
	DelayFrames = (PHASE/100.0)*(FPS/FREQ) - FramesFalling - 0.25*(FPS/FREQ) - ADJUST;

	while (DelayFrames < 0) {
		DelayFrames = DelayFrames + (FPS/FREQ);
	}
	//printf("\nDelay Frames: %d\n", DelayFrames);

	// Convert DelayFrames to DELAYTIME based on fps of camera
	float DELAYTIME = DelayFrames/FPS;
	*/


	/* 
	// Turning ExSync Enable ON to start receiving ExSync Trigger
	nExSyncEnable		= 1;
	if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	*/


	// =================================================================================================================
	// Initialization of variables
	// ================================================================================================================

	// Counter and DELAYTIME for phase relationship



	int currentTest = 0, currentSample = 0;

	// =================================================================================================================
	// Read serial port: COM6 [FPS, NUMIMAGES, PULSETIME, DELAYTIME_START, DELAYTIME_INCR, SAMPLES]
	// =================================================================================================================

	// Read line from serial port - GUI
	std::string GUIRead;
	printf("Waiting to read from GUI.\r\n");
	GUIRead = ReadSerialPort (m_hSerialCommGUI);

	// Store message into variables
	int FPS, NUMIMAGES, PULSETIME, SAMPLES;
	float DELAYTIME, DELAYTIMEINCR;
	const char * c = GUIRead.c_str();

	sscanf(c, "%d%*c %d%*c %d%*c %f%*c %f%*c %d", &FPS, &NUMIMAGES, &PULSETIME, &DELAYTIME, &DELAYTIMEINCR, &SAMPLES);

	printf("[FPS, NUMIMAGES, PULSETIME, DELAYTIME_START, DELAYTIME_INCR, SAMPLES]: %d, %d, %d, %f, %f, %d\r\n", FPS, NUMIMAGES, PULSETIME, DELAYTIME, DELAYTIMEINCR, SAMPLES);

	

	// Variables used for reading from frame grabber and storing pointer to images in buf[i]
	int lastPicNr, tempCounter;

	// Initialize serial command
	char message[50];	

	// Define properties for VideoWriter
	bool isColor = ISCOLOR;
	int fps     = VIDEOFPS;
	int frameW  = WIDTH;
	int frameH  = HEIGHT;
	Size size2 = Size(frameW, frameH);
	int codec = CV_FOURCC('H', 'F', 'Y', 'U'); 	// Usable: HFYU, MJPG(missing some information) 
												// Not usable: MPEG, MPG4, YUV8, YUV9, _RGB, 8BPS, DUCK, MRLE, PIMJ, PVW2, RGBT, RLE4, RLE8, RPZA
	char filename[64];


	// =================================================================================================================
	// Start of automated testing - Increment DELAYTIME by 5 ms each run, for one period of shaking
	// ================================================================================================================
	while( currentTest < 1 ) {
	//while( (DELAYTIME*FPS) < (FPS/FREQ) ) {

		currentSample = 0;

		while (currentSample < SAMPLES) {

			// =================================================================================================================
			// Initialization of the microEnable frame grabber
			// =================================================================================================================
			if((fg = Fg_Init(dllName,nBoard)) == NULL) {
				status = ErrorMessageWait(fg);
				return status;
			}
			fprintf(stdout,"\r\nInit Grabber: ok\n");
		


			// =================================================================================================================
			// Setting the image size and camera format
			// =================================================================================================================
			int width	= WIDTH;
			int height	= HEIGHT;
			int CamTyp	= CAMTYP;

			if (Fg_setParameter(fg,FG_WIDTH,&width,nCamPort) < 0) {
				status = ErrorMessageWait(fg);
				return status;
			}
			if (Fg_setParameter(fg,FG_HEIGHT,&height,nCamPort) < 0) {
				status = ErrorMessageWait(fg);
				return status;
			}
			if (Fg_setParameter(fg,FG_CAMERA_LINK_CAMTYP,&CamTyp,nCamPort) < 0) {
				status = ErrorMessageWait(fg);
				return status;
			}

			fprintf(stdout,"Set Image Size on port %d (w: %d,h: %d): ok\n",nCamPort,width,height);
			fprintf(stdout,"Set CameraLink Format on port %d to %d (108 == 8-bit Dual Tap): ok\n",nCamPort,CamTyp);



			// =================================================================================================================
			// Memory allocation
			// =================================================================================================================
			int format;
			Fg_getParameter(fg,FG_FORMAT,&format,nCamPort);
			size_t bytesPerPixel = 1;
			switch(format){
			case FG_GRAY:	bytesPerPixel = 1; break;
			case FG_GRAY16:	bytesPerPixel = 2; break;
			case FG_COL24:	bytesPerPixel = 3; break;
			case FG_COL32:	bytesPerPixel = 4; break;
			case FG_COL30:	bytesPerPixel = 5; break;
			case FG_COL48:	bytesPerPixel = 6; break;
			}
			size_t totalBufSize = width*height*nr_of_buffer*bytesPerPixel;
			dma_mem *pMem0;
			if((pMem0 = Fg_AllocMemEx(fg,totalBufSize,nr_of_buffer)) == NULL){
				status = ErrorMessageWait(fg);
				return status;
			} else {
				fprintf(stdout,"%d framebuffer allocated for port %d: ok\n",nr_of_buffer,nCamPort);
			}

			// Allocate memory for buf array to store pointers to images
			unsigned char * buf[10000];
			for(int i=0;i<10000;i++)
			{
				buf[i] = (unsigned char *) malloc(width*height);
			}



			// =================================================================================================================
			// Setting the trigger and grabber mode for Extern Trigger
			// =================================================================================================================
			int		nTriggerMode		= TRIGMODE;	
			int	    nExposureInMicroSec	= EXPOSUREMS;
			int		nTrgSource			= TRIGSOURCE;				//0-3: Trigger Source 0-3 (respectively)
			int		nExSyncEnable		= 0;						//0: OFF, 1: ON

			if(Fg_setParameter(fg,FG_TRIGGERMODE,&nTriggerMode,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
			if(Fg_setParameter(fg,FG_EXPOSURE,&nExposureInMicroSec,nCamPort)<0)	{ status = ErrorMessageWait(fg); return status;}
			if(Fg_setParameter(fg,FG_TRIGGERINSRC,&nTrgSource,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
			if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}



			// =================================================================================================================
			// Starts a continuous grabbing
			// =================================================================================================================
			if((Fg_AcquireEx(fg,nCamPort,GRAB_INFINITE,ACQ_STANDARD,pMem0)) < 0){
				status = ErrorMessageWait(fg);
				return status;
			}



			// =================================================================================================================
			// Enable ExSync Trigger mode
			// =================================================================================================================
			nExSyncEnable		= 1;
			if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
		


			// =================================================================================================================
			// Send serial command to PPOD to adjust shaking parameters
			// =================================================================================================================
		
			// To Do



			// =================================================================================================================
			// Send serial command to PIC for [NUMIMAGES, FPS, PULSETIME, DELAYTIME]
			// =================================================================================================================
			sprintf(message,"%d, %d, %d, %f", NUMIMAGES, FPS, PULSETIME, DELAYTIME);
			WriteSerialPort(m_hSerialCommPIC, message);
			printf("[NUMIMAGES, FPS, PULSETIME, DELAYTIME]: %d, %d, %d, %f\r\n", NUMIMAGES, FPS, PULSETIME, DELAYTIME);



			// =================================================================================================================
			// When PIC receives serial command, will create ExSync Trigger and delay droplet production
			// =================================================================================================================



			// =================================================================================================================
			// Obtain frames from TrackCam using ExSycn Trigger and store pointer to image in buf
			// =================================================================================================================
			lastPicNr = 0, tempCounter = 0;

			// Grab frames until NUMIMAGES frames
			while((lastPicNr<NUMIMAGES)) {
			
				// Obtain frame number and store pointer to image in buf[]
				lastPicNr = Fg_getLastPicNumberBlockingEx(fg,lastPicNr+1,nCamPort,10,pMem0); //Waits a maximum of 10 seconds

				/* Can cause speed issues and grab a few less frames
				if(lastPicNr == 1){ 
					printf("\r\nFrames starting to be grabbed from TrackCam.\r\n");
				}
				*/

				if(lastPicNr <0){
					status = ErrorMessageWait(fg);
					Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
					Fg_FreeMemEx(fg,pMem0);
					Fg_FreeGrabber(fg);
					return status;

				}

				buf[tempCounter] = (unsigned char *) Fg_getImagePtrEx(fg,lastPicNr,0,pMem0);
				tempCounter++;
			}
			printf("\r\nSuccessful Frames Grabbed: %d/%d\r\n", tempCounter, NUMIMAGES);

		

			// =================================================================================================================
			// Writing avi file using each frame
			// =================================================================================================================

			// Create unique name for each AVI object
			sprintf(filename, "%fDelayTime_%dFPS_%dPulseTime_%d.avi", DELAYTIME, FPS, PULSETIME, currentSample);

			VideoWriter writer2(filename, codec, fps, size2, isColor);
			writer2.open(filename, codec, fps, size2, isColor);

			// Check to see if VideoWriter properties are usable
			if (writer2.isOpened()) 
			{
				printf("\r\nStarting to write frames to AVI file.\r\n");
				for(int i=0;i<NUMIMAGES;i++) {

					// Load frame from buf[i] to TempImg for writing to video
					Mat TempImg(height, width, CV_8UC1, buf[i]);

					// Flip image vertically to compensate for camera upside down
					Mat FlippedImg;
					flip(TempImg, FlippedImg, 0);

					// Write the frame to the video
					writer2.write(FlippedImg);

				}
			}
			else {
				cout << "ERROR while opening" << endl;
			}
			printf("AVI file written.\r\n");



			// =================================================================================================================
			// Turning ExSync Enable OFF since finished receiving triggers
			// =================================================================================================================
			nExSyncEnable		= 0;
			if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}



			// =================================================================================================================
			// Freeing the grabber resource
			// =================================================================================================================
			Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
			Fg_FreeMemEx(fg,pMem0);
			Fg_FreeGrabber(fg);


			// Increment current sample number
			currentSample++;

		}


		// Increment DELAYTIME by DELAYTIMEINCR
		DELAYTIME = DELAYTIME + DELAYTIMEINCR;

		// Increment current test number
		currentTest++;
	}


	/*
	// Turning ExSync Enable OFF since finished receiving triggers
	nExSyncEnable		= 0;
	if(Fg_setParameter(fg,FG_EXSYNCON,&nExSyncEnable,nCamPort)<0)		{ status = ErrorMessageWait(fg); return status;}
	*/
	
	/*
	// =================================================================================================================
	// Freeing the grabber resource and closing the serial ports
	// =================================================================================================================
	Fg_stopAcquireEx(fg,nCamPort,pMem0,0);
	Fg_FreeMemEx(fg,pMem0);
	Fg_FreeGrabber(fg);
	*/

	// Close the serial ports
	CloseSerialPort(m_hSerialCommPIC);
	CloseSerialPort(m_hSerialCommGUI);



	return FG_OK;
}
