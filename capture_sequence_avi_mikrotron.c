/*
 *  INSTRUCTIONS:
 *
 *  1)	Set 'define' options below according to the intended camera
 *	and video format.
 *
 *	For PIXCI(R) SV2, SV3, SV4, SV5, SV5A, SV5B, SV5L, and SV6 frame grabbers
 *	common choices are RS-170, NSTC, NTSC/YC, CCIR, PAL, or PAL/YC.
 *	(The SV5A and SV5B do not support NTSC/YC or PAL/YC).
 *	For PIXCI(R) SV7 frame grabbers
 *	common choices are RS-170, NSTC, CCIR, or PAL.
 *	For PIXCI(R) SV8 frame grabbers
 *	common choices are RS-170, NSTC, NTSC/YC, CCIR, PAL, PAL/YC.
 *
 *	For PIXCI(R) A, CL1, CL2, CL3SD, D, D24, D32, D2X, D3X, D3XE, E1, E1DB, E4, E4DB, E8, E8CAM, E8DB, e104x4
*	EB1, EB1-POCL, EB1mini, EC1, ECB1, ECB1-34, ECB2, EL1, EL1DB, ELS2, SI, SI1, SI2, and SI4
 *	frame grabbers, use "default" to select the default format for the camera
 *	for which the PIXCI(R) frame grabber is intended.
 *	For non default formats, use XCAP to save the video set-up to a
 *	file, and set FORMAT to the saved file's name.
 *	For camera's with RS-232 control, note that the saved
 *	video set-up only resets the PIXCI(R) frame grabber's
 *	settings, but XCLIB does not reset the camera's settings.
 *
 *	Alternately, this could be modified to use any
 *	other convention chosen by the programmer to allow
 *	run time selection of the video format and resolution.
 *
 */
#if !defined(FORMAT) && !defined(FORMATFILE)
  #define FORMATFILE	"ExTrigger_104_174_0_05ms.fmt"	  // using format file saved by XCAP
#endif


/*
 *  2) Set number of expected PIXCI(R) image boards.
 *  This example expects only one unit.
 */
#if !defined(UNITS)
    #define UNITS	1
#endif
#define UNITSMAP    ((1<<UNITS)-1)  // shorthand - bitmap of all units


/*
 *  3) Optionally, set driver configuration parameters.
 *  These are normally left to the default, "".
 *
 *  Note: Under Linux, the image frame buffer memory can't be set as
 *  a run time option. It MUST be set via insmod so the memory can
 *  be reserved during Linux's initialization.
 */
#if !defined(DRIVERPARMS)
  //#define DRIVERPARMS "-QU 0"       // don't use interrupts
    #define DRIVERPARMS ""	      // default
#endif


/*
 *  4)	Choose whether the optional PXIPL Image Processing Library
 *	is available.
 */
#if !defined(USE_PXIPL)
    #define USE_PXIPL	0
#endif


/*
 *  5a) Compile with GCC w/out PXIPL for 32 bit Linux as:
 *
 *	    gcc -DC_GNU32=400 -DOS_LINUX -I../.. triggertest1.c ../../xclib_i386.a -lm
 *
 *	Compile with GCC with PXIPL for 32 bit Linux as :
 *
 *	    gcc -DC_GNU32=400 -DOS_LINUX -I../.. triggertest1.c ../../pxipl_i386.a ../../xclib_i386.a -lm
 *
 *	Compile with GCC w/out PXIPL for 64 bit Linux as:
 *
 *	    gcc `pkg-config --cflags opencv` `pkg-config --libs opencv` -DC_GNU64=400 -DOS_LINUX -I../.. capture_avi_sequence.c ../../xclib_x86_64.a -lm
 *
 *	Compile with GCC with PXIPL for 64 bit Linux as :
 *
 *	    gcc -DC_GNU64=400 -DOS_LINUX -I../.. triggertest1.c ../../pxipl_x86_64.a ../../xclib_x86_64.a -lm
 *
 *
 *  5b) Run the output file from GCC:
 *
 *	    ./a.out
 *
 */











// ================================================================================================
// NECESSARY INCLUDES: 
// ================================================================================================

// C library
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// UNIX standard function definitions
#include <unistd.h> 

// Function prototypes for Simple C Programming (SCP)
#include "xcliball.h"

#if USE_PXIPL
  #include "pxipl.h"
#endif

// OpenCV2
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// UDP communication
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVERA "129.105.69.140"    // server IP address (Machine A - Windows & TrackCam)
#define PORTA    9090               // port on which to send data (Machine A)
#define SERVERB "129.105.69.220"    // server IP address (Machine B - Linux & Mikrotron)
#define PORTB    51717              // port on which to listen for incoming data (Machine B)
#define BUFLEN   512                // max length of buffer




// Create UDP socket structures for Machine A and Machine B
struct sockaddr_in AddrMachineA, AddrMachineB;




// ================================================================================================
// SUPPORT STUFF:
// Catch CTRL+C and floating point exceptions so that once opened, the PIXCI(R) driver
// and frame grabber are always closed before exit
// ================================================================================================
void sigintfunc(int sig)
{
    pxd_PIXCIclose();
    exit(1);
}



// ================================================================================================
// Video 'interrupt' callback function 
// ================================================================================================
int fieldirqcount = 0;
void videoirqfunc(int sig)
{
    fieldirqcount++;
}



// ================================================================================================
// Socket error
// ================================================================================================
void die(char *sock)
{
    perror(sock);
    exit(1);
}



// ================================================================================================
// Create UDP socket, set up Machine A structure, bind socket to port for Machine B
// ================================================================================================
int InitializeUDP(int sock)
{
    // Create a UDP socket
    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    printf("Socket created.\r\n");


    // Zero out the structure for Machine A
    memset((char *) &AddrMachineA, 0, sizeof(AddrMachineA));

    AddrMachineA.sin_family = AF_INET;
    AddrMachineA.sin_port = htons(PORTA);

    // IP address of the receiver
    if (inet_aton(SERVERA, &AddrMachineA.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    printf("Machine A structure set up with IP address of the receiever and the specified port.\r\n");


    // Zero out the structure for Machine B
    memset((char *) &AddrMachineB, 0, sizeof(AddrMachineB));

    AddrMachineB.sin_family = AF_INET;
    AddrMachineB.sin_port = htons(PORTB);
    AddrMachineB.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind UDP socket to port for Machine B
    if( bind(sock, (struct sockaddr*)&AddrMachineB, sizeof(AddrMachineB) ) == -1 )
    {
        die("bind");
    }
    printf("UDP socket binded to any address and the specified port.\r\n\n");

    return sock;
}



// ================================================================================================
// Receive message from Machine A
// ================================================================================================
void ReceiveSocket(int sock, char buf[], int slen)
{
    // Try to receive some data from Machine A (blocking call)
    printf("---------------------------------------------------------------------------------------\r\n\n");
    printf("Waiting for data...\r\n");
    fflush(stdout);

    int recv_len;
    if( (recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &AddrMachineA, &slen)) == -1 )
    {
        die("recvfrom()");
    }
}



// ================================================================================================
// Send message to Machine A
// ================================================================================================
void SendSocket(int sock, char message[], int slen)
{
    // Send the message to Machine A
    if( sendto(sock, message, strlen(message) , 0 , (struct sockaddr *) &AddrMachineA, slen) == -1 )
    {
        die("sendto()");
    }
    printf("Message sent to Machine A.\r\n\n");
}



// ================================================================================================
// Close UDP socket
// ================================================================================================
void CloseSocket(int sock)
{
    // Close socket
    close(sock);
    printf("Closed socket.\r\n");
}



// ================================================================================================
// Open and initialize frame grabber
// ================================================================================================
int InitializationFrameGrabber(void)
{
    // Open the XCLIB C Library for use
    int i;

    printf("Opening EPIX(R) PIXCI(R) Frame Grabber, ");

    // Either FORMAT or FORMATFILE should have been selected above
    #if defined(FORMAT)
	printf("using predefined format '%s'.\n", FORMAT);
	i = pxd_PIXCIopen(DRIVERPARMS, FORMAT, "");
    #elif defined(FORMATFILE)
	printf("using format file '%s'.\n", FORMATFILE);
	i = pxd_PIXCIopen(DRIVERPARMS, "", FORMATFILE);
    #endif
    if (i < 0) {
	printf("Open Error %d\a\a\n", i);
	pxd_mesgFault(UNITSMAP);
	return(i);
    }

    printf("Open Okay.\r\n");


    // Report image resolution
    printf("Image resolution: %d X %d\r\n\n", pxd_imageXdim(), pxd_imageYdim());

    return(0);
}



// ================================================================================================
// Capture sequence AVI
// ================================================================================================
void CaptureSequenceAVI(int NUMIMAGES, int FPS, int PULSETIME, float DELAYTIME, int AMPL, int FREQ, int currentSample, int sock)
{

    // Allocate memory for buf array to store pointers to images
    unsigned char* buf[10000];
    int i;
    for(i=0; i<10000; i++)
    {
        buf[i] = (unsigned char*)malloc( pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char) );
    }


    // Send UDP message to Machine A to start sequence AVI - otherwise Machine A starts before Machine B is ready to capture
    int slen=sizeof(AddrMachineA);
    char message[BUFLEN];

    AddrMachineA.sin_port = htons(PORTA);    // specify PortA to send message back to
    strcpy(message, "Start sequence AVI.");
    SendSocket(sock, message, slen);


    // For a camera in asynchronous trigger mode, with an external trigger, sequence capture is simply:
    printf("Ready to capture sequence AVI.\r\n\n");
    pxd_goLiveSeq(UNITSMAP,         // select PIXCI(R) unit 1
                  1,                // select start frame buffer
                  (NUMIMAGES-1),    // select last frame buffer
                  1,                // incrementing by one buffer
                  (NUMIMAGES-1),    // for this many captures
                  1);               // advancing to next buffer after each 1 frame


    // Wait for capture to cease
    while (pxd_goneLive(UNITSMAP, 0)) {
        ;
    }
    printf("Sequence AVI captured.\r\n");


    // Store pointers to images in buf[]
    int j;
    for(j=0; j<(NUMIMAGES-1); j++)
    {
        //j+1th frame -> buf[j]
        pxd_readuchar(UNITSMAP, j+1, 0, 0, -1, -1, buf[j], pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char), "Grey"); 
    }
    printf("Pointers to frame buffers stored in buf[].\r\n\n");
    

/*
    // Save BMP image using XCLIB
    char *name1 = "/usr/local/xclib3_8/examples/C/image1.bmp";
    pxd_saveBmp(UNITSMAP, name1, 1, 0, 0, -1, -1, 0, 0);    // 3rd argument is frame buffer

    // Reads an image from a buffer in memory.
    IplImage* TempImgMat;
    TempImgMat = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);
    int i, j, counter=0;
    CvScalar s;
    for(i=0; i < pxd_imageYdim(); i++){
        for (j=0; j < pxd_imageXdim(); j++) {
            s.val[0] = buf[counter];
            cvSet2D(TempImgMat, i, j, s); // set the (i,j) pixel value
            counter++;
        }
    }

    // Saves an image from image buffer to a specified file.
    const char* filename3 = "/usr/local/xclib3_8/examples/C/image1_from_buffer.bmp";
    const int* params=0;
    cvSaveImage(filename3, TempImgMat, params);
    printf("Image1 from buffer -> saved.\r\n");
*/

    // Create VideoWriter using Huffyuv encoding at 5 fps - save to Documents->High Speed Videos
    char filename[64];
    sprintf(filename, "/home/maciver/Documents/High Speed Videos/%dA_%dHz_%fDelayTime_%dFPS_%dPulseTime_%d.avi", AMPL, FREQ, DELAYTIME, FPS, PULSETIME, currentSample);
    int fourcc = CV_FOURCC('H','F','Y','U');
    double fps = 5;
    CvSize frame_size;
    frame_size = cvSize( pxd_imageXdim(), pxd_imageYdim() );
    int is_color = 0;

    CvVideoWriter* writer;
    writer = cvCreateVideoWriter(filename, fourcc, fps, frame_size, is_color);
    printf("VideoWriter created.\r\n");


    // Loop through each frame, writing them to the avi file
    printf("Starting to write frames to AVI file.\r\n");
    int k;
    for(k=0; k<(NUMIMAGES-1); k++)
    {
        // Create IplImage* TempImg in order to write the frame buffers to avi object
        IplImage* TempImg;
        TempImg = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);

        // CvScalar tuple for setting pixel values from frame buffer
        CvScalar s;

        // Reads each image from respective buffers in memory, storing as IplImage* (necessary for cvWriteFrame)
        int l, m, counter=0;
        for(l=0; l < pxd_imageYdim(); l++){
            for (m=0; m < pxd_imageXdim(); m++) {
                s.val[0] = buf[k][counter];
                cvSet2D(TempImg, l, m, s); // set the (l,m) pixel value using s.val[0]
                counter++;
            }
        }

        // Write frame to avi object
        cvWriteFrame(writer, TempImg);
    }


    // Release VideoWriter
    cvReleaseVideoWriter(&writer);



    // Check for faults, such as erratic sync or insufficient PCI bus bandwidth
    pxd_mesgFault(UNITSMAP);
    printf("AVI file written.\r\n\n");
}



// ================================================================================================
// Close the PIXCI(R) frame grabber
// ================================================================================================
void CloseFrameGrabber(void)
{
    pxd_PIXCIclose();
    printf("PIXCI(R) frame grabber closed.\r\n\n\n");
}








// ================================================================================================
// Main function
// ================================================================================================
main(void)
{
    // Catch signals
    signal(SIGINT, sigintfunc);
    signal(SIGFPE, sigintfunc);


    // Local variables used for UDP communication
    int sock, slen=sizeof(AddrMachineA);
    char buf[BUFLEN];
    char message[BUFLEN];


    // Local variables used for checking to see if still running tests
    int Run_Flag = 1;


    // Local variables used for capturing sequence AVI
    int NUMIMAGES_Side, FPS_Side, PULSETIME, AMPL, FREQ, currentSample;
    float DELAYTIME;

    int statusFrameGrabber;


    // Initialize UDP socket
    sock = InitializeUDP(sock);



    // Continue to capture sequence AVI's while Run_Flag is ON (1)
    while( Run_Flag ) {

        // Receive message from Machine A - [AMPL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, currentSample, DELAYTIME]
        ReceiveSocket(sock, buf, slen);

        printf("Received packet from %s: %d\n", inet_ntoa(AddrMachineA.sin_addr), ntohs(AddrMachineA.sin_port));

        sscanf(buf, "%d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f", &AMPL, &FREQ, &FPS_Side, &NUMIMAGES_Side, &PULSETIME, &currentSample, &DELAYTIME);
        printf("[AMPL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, currentSample, DELAYTIME]: %d, %d, %d, %d, %d, %d, %f\r\n\n", AMPL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, currentSample, DELAYTIME);


        // Open and initialize frame grabber
        statusFrameGrabber = InitializationFrameGrabber();
        if (statusFrameGrabber < 0)
            return(1);


        // Capture sequence AVI
        CaptureSequenceAVI(NUMIMAGES_Side, FPS_Side, PULSETIME, DELAYTIME, AMPL, FREQ, currentSample, sock);


        // Close frame grabber
        CloseFrameGrabber();


        // Check to see if still running tests from Machine A
        ReceiveSocket(sock, buf, slen);
        sscanf(buf, "%d", &Run_Flag);
        printf("Run_Flag: %d\r\n\n", Run_Flag);


        // Send UDP message to Machine A to notify receival
        memset(&message[0], 0, sizeof(message));

        AddrMachineA.sin_port = htons(PORTA);    // specify PortA to send message back to
        strcpy(message, "Message received.");
        SendSocket(sock, message, slen);        
    }


    // Close UDP socket
    CloseSocket(sock);


    return(0);
}
