//-----------------------------------------------------------------//
// Name        | SC2_defs.h                  | Type: ( ) source    //
//-------------------------------------------|       (*) header    //
// Project     | PCO                         |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Embedded System, PC                               //
//-----------------------------------------------------------------//
// Environment | All 'C'-compiler used at PCO                      //
//-----------------------------------------------------------------//
// Purpose     | Defines, constants for use with SDK commands for  //
//             | pco.camera (SC2)                                  //
//-----------------------------------------------------------------//
// Author      | LWA, MBL, PCO AG                                  //
//-----------------------------------------------------------------//
// Revision    | Rev. 0.25                                         //
//-----------------------------------------------------------------//
// Notes       |                                                   //
//-----------------------------------------------------------------//
// (c) 2004 PCO AG  *  Donaupark 11 *  D-93309 Kelheim / Germany   //
// *  Phone: +49 (0)9441 / 2005-0  *                               //
// *  Fax:   +49 (0)9441 / 2005-20 *  Email: info@pco.de           //
//-----------------------------------------------------------------//


//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  0.01     | 30.07.2003 |  new file, LWA                         //
//-----------------------------------------------------------------//
//  0.02     | 19.08.2003 |  MBL all changed to uppercase          //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.03     | 01.06.2004 |  LWA:                                  //
//           |            |  FPS_EXPOSURE_MODE_OFF/ON added.       //
//           | 28.06.2004 |  LWA:  BIT_ALIGNMENT_MSB/LSB added.    //
//-----------------------------------------------------------------//
//  0.20     | 23.07.2004 |  LWA:  Defines for internal use moved  //
//           |            |        to SC2_DEFS_INTERN.H            //
//           | 14.09.2004 |  LWA:  CAMERATYPE_ROCHEHTC added       //
//           | 26.09.2004 |  LWA:  TEMPERATURE_NOT_AVAILABLE added //
//-----------------------------------------------------------------//
//  0.21     | 17.03.2005 |  LWA:  CAMERATYPE_284XS added          //
//           |            |                                        //
//           |            |  LWA  added:                           //
//           |            |                                        //
//           |            |    NOISE_FILTER_MODE_ON                //
//           |            |    NOISE_FILTER_MODE_OFF               //
//           |            |    NOISE_FILTER_MODE_REMOVE_HOT_DARK   //
//-----------------------------------------------------------------//
//  0.21     | 04.05.2005 |  LWA:  TIMESTAMP_MODE_ASCII added.     //
//           |            |        GENERALCAPS1_... defines added. //
//           | 08.06.2005 |  FRE:  GENERALCAPS1_DATAFORMAT2X12 ad. //
//           | 27.06.2005 |  LWA:  GENERALCAPS1_RECORD_STOP added. //
//           |            |        RECORD_STOP_EVENT_OFF           //
//           |            |        RECORD_STOP_EVENT_STOP_BY_SW    //
//           |            |        RECORD_STOP_EVENT_STOP_EXTERNAL //
//-----------------------------------------------------------------//
//  0.22     | 06.03.2006 |  LWA:  CAMERATYPE_KODAK1300OEM added.  //
//           |            |                                        //
//           | 09.03.2006 |  LWA:  added defines:                  //
//           |            |                                        //
//           |            |    HOT_PIXEL_CORRECTION_OFF            //
//           |            |    HOT_PIXEL_CORRECTION_ON             //
//           |            |    HOT_PIXEL_CORRECTION_TEST           //
//           |            |    GENERALCAPS1_HOT_PIXEL_CORRECTION   //
//-----------------------------------------------------------------//
//  0.23     | 01.06.2006 |  Preparation for modulation mode:      //
//           |   (FRe)    |  Added modulation mode parameters and  //
//           |            |  telegrams:                            //
//           |            |    do_S(G)ET_MODULATION_MODE           //
//           |            |  Added second descriptor, flags and    //
//           |            |  telegram:                             //
//           |            |    do_GET_DESCRIPTION_EX               //
//           |            |  Changed header to local               //
//           |            |  c:\pco_include\include in order to    //
//           |            |   support header file repository       //
//-----------------------------------------------------------------//
//  0.24     | 19.09.2007 |  FRE:Added defines for GET_INFO_STRING //
//-----------------------------------------------------------------//
//  0.25     | 31.01.2008 |  FRE:Added defines for                 //
//           |            |  GENERALCAPS1_NO_EXTEXPCTRL            //
//           |            |  GENERALCAPS1_NO_TIMESTAMP             //
//           |            |  GENERALCAPS1_NO_ACQUIREMODE           //
//-----------------------------------------------------------------//
//  0.26     | 31.08.2010 |  FRE:Added defines for                 //
//           |            |  Lookup Table commands                 //
//           |            |  Fairchild color sensor                //
//-----------------------------------------------------------------//

// Do not change any values after release! Only additions are allowed!

#ifndef SC2_DEFS_H
#define SC2_DEFS_H


// ------------------------------------------------------------------------ //
// -- Defines for Get Camera Type Command: -------------------------------- //
// ------------------------------------------------------------------------ //

// pco.camera types
#define CAMERATYPE_PCO1200HS     0x0100
#define CAMERATYPE_PCO1300       0x0200
#define CAMERATYPE_PCO1600       0x0220
#define CAMERATYPE_PCO2000       0x0240
#define CAMERATYPE_PCO4000       0x0260

// pco.1300 types
#define CAMERATYPE_ROCHEHTC      0x0800 // Roche OEM
#define CAMERATYPE_284XS         0x0800
#define CAMERATYPE_KODAK1300OEM  0x0820 // Kodak OEM

// pco.1400 types
#define CAMERATYPE_PCO1400       0x0830
#define CAMERATYPE_NEWGEN        0x0840 // Roche OEM

// pco.usb.pixelfly
#define CAMERATYPE_PCO_USBPIXELFLY        0x0900


// pco.dimax types
#define CAMERATYPE_PCO_DIMAX_STD          0x1000
#define CAMERATYPE_PCO_DIMAX_TV           0x1010
#define CAMERATYPE_PCO_DIMAX_AUTOMOTIVE   0x1020

// pco.sensicam types                   // tbd., all names are internal ids
#define CAMERATYPE_SC3_SONYQE    0x1200 // SC3 based - Sony 285
#define CAMERATYPE_SC3_EMTI      0x1210 // SC3 based - TI 285SPD
#define CAMERATYPE_SC3_KODAK4800 0x1220 // SC3 based - Kodak KAI-16000

// pco.edge types
#define CAMERATYPE_PCO_EDGE      0x1300 // SCMOS pco edge
#define CAMERATYPE_PCO_EDGE_GL   0x1310 // SCMOS pco edge, global shutter

//#define CAMERATYPE_PCOUPDATE     0xFFFF   // indicates Camera in update mode!

// ------------------------------------------------------------------------ //
// -- Defines for Interfaces ---------------------------------------------- //
// ------------------------------------------------------------------------ //
// These defines are camera internal defines and are not SDK related!
#define INTERFACE_FIREWIRE       0x0001
#define INTERFACE_CAMERALINK     0x0002
#define INTERFACE_USB            0x0003
#define INTERFACE_ETHERNET       0x0004
#define INTERFACE_SERIAL         0x0005

// ------------------------------------------------------------------------ //
// -- Defines for CameraLink DataFormat  ---------------------------------- //
// ------------------------------------------------------------------------ //

// Obsolete. Use defines in SC2_SdkAddendum.h
#define CL_DATAFORMAT     0x0F
#define CL_FORMAT_1x16    0x01
#define CL_FORMAT_2x12    0x02
#define CL_FORMAT_3x8     0x03
#define CL_FORMAT_4x16    0x04
#define CL_FORMAT_5x16    0x05
#define CL_FORMAT_5x12    0x07

#define CL_TESTPATTERN    0xF0
#define CL_TESTPATTERN_1  0x10  
#define CL_TESTPATTERN_2  0x20  
#define CL_TESTPATTERN_3  0x30  

// ------------------------------------------------------------------------ //
// -- Bitmask Defines for CameraLink Transmit------------------------------ //
// ------------------------------------------------------------------------ //

#define CL_TRANSMIT_ENABLE          0x01
#define CL_TRANSMIT_LONGGAP         0x02

// ------------------------------------------------------------------------ //
// -- Defines for CameraLink CCLines     ---------------------------------- //
// ------------------------------------------------------------------------ //

#define CL_CCLINE_LINE1_TRIGGER           0x01
#define CL_CCLINE_LINE2_ACQUIRE           0x02
#define CL_CCLINE_LINE3_HANDSHAKE         0x04
#define CL_CCLINE_LINE4_TRANSMIT_ENABLE   0x08



// ------------------------------------------------------------------------ //
// -- Defines for Get Camera Health Status Command: ----------------------- //
// ------------------------------------------------------------------------ //

// mask bits: evaluate as follows: if (stat & ErrorSensorTemperature) ... //

#define WARNING_POWERSUPPLYVOLTAGERANGE 0x00000001
#define WARNING_POWERSUPPLYTEMPERATURE  0x00000002
#define WARNING_CAMERATEMPERATURE       0x00000004
#define WARNING_SENSORTEMPERATURE       0x00000008

#define ERROR_POWERSUPPLYVOLTAGERANGE   0x00000001
#define ERROR_POWERSUPPLYTEMPERATURE    0x00000002
#define ERROR_CAMERATEMPERATURE         0x00000004
#define ERROR_SENSORTEMPERATURE         0x00000008

#define ERROR_CAMERAINTERFACE           0x00010000
#define ERROR_CAMERARAM                 0x00020000
#define ERROR_CAMERAMAINBOARD           0x00040000
#define ERROR_CAMERAHEADBOARD           0x00080000

#define STATUS_DEFAULT_STATE            0x00000001
#define STATUS_SETTINGS_VALID           0x00000002
#define STATUS_RECORDING_ON             0x00000004
#define STATUS_READ_IMAGE_ON            0x00000008
#define STATUS_FRAMERATE_VALID          0x00000010
#define STATUS_SEQ_STOP_TRIGGERED       0x00000020
#define STATUS_LOCKED_TO_EXTSYNC        0x00000040



// ------------------------------------------------------------------------ //
// -- Defines for Get Camera Description Command: ------------------------- //
// ------------------------------------------------------------------------ //

  // Description type

#define DESCRIPTION_STANDARD   0x0000         // Standard Descripton
#define DESCRIPTION_2          0x0001         // Descripton nr. 2

// ------------------------------------------------------------------------ //
// -- Sensor type definitions --------------------------------------------- //
// ------------------------------------------------------------------------ //
  // Sensor Type 
  // ATTENTION: Lowest bit is reserved for COLOR CCDs
  // In case a new color CCD is added the lowest bit MUST be set!!!
#define SENSOR_ICX285AL           0x0010      // Sony
#define SENSOR_ICX285AK           0x0011      // Sony
#define SENSOR_ICX263AL           0x0020      // Sony
#define SENSOR_ICX263AK           0x0021      // Sony
#define SENSOR_ICX274AL           0x0030      // Sony
#define SENSOR_ICX274AK           0x0031      // Sony
#define SENSOR_ICX407AL           0x0040      // Sony
#define SENSOR_ICX407AK           0x0041      // Sony
#define SENSOR_ICX414AL           0x0050      // Sony
#define SENSOR_ICX414AK           0x0051      // Sony

#define SENSOR_KAI2000M           0x0110      // Kodak
#define SENSOR_KAI2000CM          0x0111      // Kodak
#define SENSOR_KAI2001M           0x0120      // Kodak
#define SENSOR_KAI2001CM          0x0121      // Kodak
#define SENSOR_KAI2002M           0x0122      // Kodak slow roi
#define SENSOR_KAI2002CM          0x0123      // Kodak slow roi

#define SENSOR_KAI4010M           0x0130      // Kodak
#define SENSOR_KAI4010CM          0x0131      // Kodak
#define SENSOR_KAI4011M           0x0132      // Kodak slow roi
#define SENSOR_KAI4011CM          0x0133      // Kodak slow roi

#define SENSOR_KAI4020M           0x0140      // Kodak
#define SENSOR_KAI4020CM          0x0141      // Kodak
#define SENSOR_KAI4021M           0x0142      // Kodak slow roi
#define SENSOR_KAI4021CM          0x0143      // Kodak slow roi

#define SENSOR_KAI11000M          0x0150      // Kodak
#define SENSOR_KAI11000CM         0x0151      // Kodak
#define SENSOR_KAI11002M          0x0152      // Kodak slow roi
#define SENSOR_KAI11002CM         0x0153      // Kodak slow roi

#define SENSOR_KAI16000AXA        0x0160      // Kodak t:4960x3324, e:4904x3280, a:4872x3248
#define SENSOR_KAI16000CXA        0x0161      // Kodak

#define SENSOR_MV13BW             0x1010      // Micron
#define SENSOR_MV13COL            0x1011      // Micron

#define SENSOR_CIS2051_V1_FI_BW   0x2000      //Fairchild front illuminated
#define SENSOR_CIS2051_V1_FI_COL  0x2001
#define SENSOR_CIS2051_V1_BI_BW   0x2010      //Fairchild back illuminated

//obsolete #define SENSOR_CCD87           0x2010         // E2V
//obsolete #define SENSOR_TC253           0x2110         // TI
#define SENSOR_TC285SPD           0x2120      // TI 285SPD

#define SENSOR_CYPRESS_RR_V1_BW   0x3000      // CYPRESS RoadRunner V1 B/W
#define SENSOR_CYPRESS_RR_V1_COL  0x3001      // CYPRESS RoadRunner V1 Color


// ------------------------------------------------------------------------ //
// -- Defines for Get Info String Command: -------------------------------- //
// ------------------------------------------------------------------------ //
typedef struct
{
 WORD  wTypdef;
 char  szName[40];
}PCO_SENSOR_TYPE_DEF;

#if defined PCO_SENSOR_CREATE_OBJECT
const PCO_SENSOR_TYPE_DEF far pco_sensor[] =
{
               // Sony sensor types
               SENSOR_ICX285AL, "Sony ICX285AL",
               SENSOR_ICX285AK, "Sony ICX285AK",
               SENSOR_ICX263AL, "Sony ICX263AL",
               SENSOR_ICX263AK, "Sony ICX263AK",
               SENSOR_ICX274AL, "Sony ICX274AL",
               SENSOR_ICX274AK, "Sony ICX274AK",
               SENSOR_ICX407AL, "Sony ICX407AL",
               SENSOR_ICX407AK, "Sony ICX407AK",
               SENSOR_ICX414AL, "Sony ICX414AL",
               SENSOR_ICX414AK, "Sony ICX414AK",
               // Kodak sensor types
               SENSOR_KAI2000M,   "Kodak KAI2000M",
               SENSOR_KAI2000CM,  "Kodak KAI2000CM",
               SENSOR_KAI2001M,   "Kodak KAI2001M",
               SENSOR_KAI2001CM,  "Kodak KAI2001CM",
               SENSOR_KAI2002M,   "Kodak KAI2002M",
               SENSOR_KAI2002CM,  "Kodak KAI2002CM",
               SENSOR_KAI4010M,   "Kodak KAI4010M",
               SENSOR_KAI4010CM,  "Kodak KAI4010CM",
               SENSOR_KAI4011M,   "Kodak KAI4011M",
               SENSOR_KAI4011CM,  "Kodak KAI4011CM",
               SENSOR_KAI4020M,   "Kodak KAI4020M",
               SENSOR_KAI4020CM,  "Kodak KAI4020CM",
               SENSOR_KAI4021M,   "Kodak KAI4021M",
               SENSOR_KAI4021CM,  "Kodak KAI4021CM",
               SENSOR_KAI11000M,  "Kodak KAI11000M",
               SENSOR_KAI11000CM, "Kodak KAI11000CM",
               SENSOR_KAI11002M,  "Kodak KAI11002M",
               SENSOR_KAI11002CM, "Kodak KAI11002CM",
               SENSOR_KAI16000AXA,"Kodak KAI16000AXA",
               SENSOR_KAI16000CXA,"Kodak KAI16000CXA",
               // Mircon sensor types
               SENSOR_MV13BW,  "Micron MV13BW",
               SENSOR_MV13COL, "Micron MV13COL",
               // Other sensor types
               SENSOR_TC285SPD, "TI TC285SPD",
			   
			   SENSOR_CYPRESS_RR_V1_BW,  "Cypress Roadrunner V1 BW",
			   SENSOR_CYPRESS_RR_V1_COL, "Cypress Roadrunner V1 Color",

               SENSOR_CIS2051_V1_FI_BW, "Fairchild CIS2051 V1 I-Front BW",
               SENSOR_CIS2051_V1_BI_BW, "Fairchild CIS2051 V1 I-Back BW"

};

const int far PCO_SENSOR_TYPE_DEF_NUM = sizeof(pco_sensor) / sizeof(pco_sensor[0]);

#else
extern const PCO_SENSOR_TYPE_DEF far pco_sensor[];
extern const int far PCO_SENSOR_TYPE_DEF_NUM;
#endif

#define INFO_STRING_CAMERA  1   // Camera name
#define INFO_STRING_SENSOR  2   // Sensor name

  // these are defines for interpreting the dwGeneralCaps1 member of the
  // Camera Description structure.
  //
  // How to use the member:
  //
  // if (CameraDescription.dwGeneralCaps1 & GENERALCAPS1_NOISE_FILTER)
  // {
  //   noise filter can be used! ...
  //   ...

#define GENERALCAPS1_NOISE_FILTER                      0x00000001
#define GENERALCAPS1_HOTPIX_FILTER                     0x00000002
#define GENERALCAPS1_HOTPIX_ONLY_WITH_NOISE_FILTER     0x00000004
#define GENERALCAPS1_TIMESTAMP_ASCII_ONLY              0x00000008

#define GENERALCAPS1_DATAFORMAT2X12                    0x00000010
#define GENERALCAPS1_RECORD_STOP                       0x00000020 // Record stop event mode
#define GENERALCAPS1_HOT_PIXEL_CORRECTION              0x00000040
#define GENERALCAPS1_NO_EXTEXPCTRL                     0x00000080 // Ext. Exp. ctrl not possible

#define GENERALCAPS1_NO_TIMESTAMP                      0x00000100
#define GENERALCAPS1_NO_ACQUIREMODE                    0x00000200
#define GENERALCAPS1_DATAFORMAT4X16                    0x00000400
#define GENERALCAPS1_DATAFORMAT5X16                    0x00000800

#define GENERALCAPS1_NO_RECORDER                       0x00001000 // Camera has no internal memory
#define GENERALCAPS1_FAST_TIMING                       0x00002000 // Camera can be set to fast timing mode (PIV)
#define GENERALCAPS1_METADATA                          0x00004000 // Camera can produce metadata

#define GENERALCAPS1_SETFRAMERATE_ENABLED              0x00008000 // Camera allows Set/GetFrameRate cmd
#define GENERALCAPS1_CDI_MODE                          0x00010000 // Camera has Correlated Double Image Mode
#define GENERALCAPS1_CCM                               0x00020000 // Camera has CCM
#define GENERALCAPS1_EXTERNAL_SYNC                     0x00040000 // Camera can be synced externally

//#define GENERALCAPS_ENHANCE_DESCRIPTOR_x             0x10000000 // reserved for future desc.
//#define GENERALCAPS_ENHANCE_DESCRIPTOR_x             0x20000000 // reserved for future desc.
#define GENERALCAPS1_HW_IO_SIGNAL_DESCRIPTOR           0x40000000
#define GENERALCAPS1_ENHANCED_DESCRIPTOR_2             0x80000000


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Camera Temperature Command: --------------------- //
// ------------------------------------------------------------------------ //

#define TEMPERATURE_NOT_AVAILABLE 0x8000


// ------------------------------------------------------------------------ //
// -- Defines for Get / Set Camera Setup: --------------------------------- //
// ------------------------------------------------------------------------ //
// Each bit sets a corresponding switch
  // Camera setup type

  // Camera setup parameter for pco.edge:
#define PCO_EDGE_SETUP_ROLLING_SHUTTER 0x00000001         // rolling shutter
#define PCO_EDGE_SETUP_GLOBAL_SHUTTER  0x00000002         // global shutter

// ------------------------------------------------------------------------ //
// -- Defines for Read/Write Mailbox & Get Mailbox Status Commands: ------- //
// ------------------------------------------------------------------------ //

#define MAILBOX_READ_STATUS_NO_VALID_MESSAGE                0x0000
#define MAILBOX_READ_STATUS_MESSAGE_VALID                   0x0001
#define MAILBOX_READ_STATUS_MESSAGE_HAS_BEEN_READ           0x0003

#define MAILBOX_STATUS_NO_VALID_MESSAGE                     0x0000
#define MAILBOX_STATUS_MESSAGE_VALID                        0x0001
#define MAILBOX_STATUS_MESSAGE_HAS_BEEN_READ                0x0003


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Binning Command: -------------------------------- //
// ------------------------------------------------------------------------ //

#define BINNING_STEPPING_BINARY 0
#define BINNING_STEPPING_LINEAR 1


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Sensor Format Command: -------------------------- //
// ------------------------------------------------------------------------ //

#define SENSORFORMAT_STANDARD 0
#define SENSORFORMAT_EXTENDED 1


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set ADC Operation: ---------------------------------- //
// ------------------------------------------------------------------------ //

#define ADC_MODE_SINGLE 1
#define ADC_MODE_DUAL   2

// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Pixelrate Operation: ---------------------------- //
// ------------------------------------------------------------------------ //

#define PIXELRATE_10MHZ 10000000
#define PIXELRATE_20MHZ 20000000
#define PIXELRATE_40MHZ 40000000
#define PIXELRATE_5MHZ   5000000


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set OffsetMode: ------------------------------------- //
// ------------------------------------------------------------------------ //

#define OFFSET_MODE_AUTO 0
#define OFFSET_MODE_OFF  1


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Noise Filter Mode: ------------------------------ //
// ------------------------------------------------------------------------ //

#define NOISE_FILTER_MODE_OFF              0x0000
#define NOISE_FILTER_MODE_ON               0x0001
#define NOISE_FILTER_MODE_REMOVE_HOT_DARK  0x0100


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Hot Pixel Correction: --------------------------- //
// ------------------------------------------------------------------------ //

#define HOT_PIXEL_CORRECTION_OFF           0x0000
#define HOT_PIXEL_CORRECTION_ON            0x0001
#define HOT_PIXEL_CORRECTION_TEST          0x0100  // for test purposes only!


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set DSNU Adjust Mode: ------------------------------- //
// ------------------------------------------------------------------------ //

#define DSNU_ADJUST_MODE_OFF               0x0000
#define DSNU_ADJUST_MODE_AUTO              0x0001
#define DSNU_ADJUST_MODE_USER              0x0002
  //only for internal use!
#define DSNU_ADJUST_MODE_CONT              0x4000
#define DSNU_ADJUST_MODE_STOP              0x8000


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set CDI Mode: --------------------------------------- //
// ------------------------------------------------------------------------ //

#define CDI_MODE_OFF                       0x0000
#define CDI_MODE_ON                        0x0001


// ------------------------------------------------------------------------ //
// -- Defines for Init DSNU Adjustment: ----------------------------------- //
// ------------------------------------------------------------------------ //

#define INIT_DSNU_ADJUSTMENT_OFF           0x0000
#define INIT_DSNU_ADJUSTMENT_ON            0x0001
#define INIT_DSNU_ADJUSTMENT_DARK_MODE     0x0002
#define INIT_DSNU_ADJUSTMENT_AUTO_MODE     0x0003


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Timebase Command: ------------------------------- //
// ------------------------------------------------------------------------ //

#define TIMEBASE_NS 0x0000
#define TIMEBASE_US 0x0001
#define TIMEBASE_MS 0x0002



// ------------------------------------------------------------------------ //
// -- Defines for Get/Set FPS Exposure Mode: ------------------------------ //
// ------------------------------------------------------------------------ //

#define FPS_EXPOSURE_MODE_OFF 0x0000
#define FPS_EXPOSURE_MODE_ON  0x0001


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Framerate: -------------------------------------- //
// ------------------------------------------------------------------------ //

#define SET_FRAMERATE_MODE_AUTO                            0x0000
#define SET_FRAMERATE_MODE_FRAMERATE_HAS_PRIORITY          0x0001
#define SET_FRAMERATE_MODE_EXPTIME_HAS_PRIORITY            0x0002
#define SET_FRAMERATE_MODE_STRICT                          0x0003

#define SET_FRAMERATE_STATUS_OK                            0x0000
#define SET_FRAMERATE_STATUS_FPS_LIMITED_BY_READOUT        0x0001
#define SET_FRAMERATE_STATUS_FPS_LIMITED_BY_EXPTIME        0x0002
#define SET_FRAMERATE_STATUS_EXPTIME_CUT_TO_FRAMETIME      0x0004
#define SET_FRAMERATE_STATUS_NOT_YET_VALIDATED             0x8000
#define SET_FRAMERATE_STATUS_ERROR_SETTINGS_INCONSISTENT   0x8001


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Delay Exposure Timetable Command: --------------- //
// ------------------------------------------------------------------------ //

#define MAX_TIMEPAIRS   16    // max size of time table for 
                              


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Trigger Mode Command: --------------------------- //
// ------------------------------------------------------------------------ //

#define TRIGGER_MODE_AUTOTRIGGER               0x0000
#define TRIGGER_MODE_SOFTWARETRIGGER           0x0001
#define TRIGGER_MODE_EXTERNALTRIGGER           0x0002
#define TRIGGER_MODE_EXTERNALEXPOSURECONTROL   0x0003
#define TRIGGER_MODE_SOURCE_HDSDI              0x0102
#define TRIGGER_MODE_EXTERNAL_SYNCHRONIZED     0x0004


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Camera Sync Mode Command: ----------------------- //
// ------------------------------------------------------------------------ //

#define CAMERA_SYNC_MODE_STANDALONE               0x0000
#define CAMERA_SYNC_MODE_MASTER                   0x0001
#define CAMERA_SYNC_MODE_SLAVE                    0x0002


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Power Down Mode Command: ------------------------ //
// ------------------------------------------------------------------------ //

#define POWERDOWN_MODE_AUTO   0
#define POWERDOWN_MODE_USER   1


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Storage Mode Command: --------------------------- //
// ------------------------------------------------------------------------ //

#define STORAGE_MODE_RECORDER      0
#define STORAGE_MODE_FIFO_BUFFER   1


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Recorder Submode Command: ----------------------- //
// ------------------------------------------------------------------------ //
#define RECORDER_SUBMODE_SEQUENCE     0
#define RECORDER_SUBMODE_RINGBUFFER   1



// ------------------------------------------------------------------------ //
// -- Defines for Set Record Stop Mode: ----------------------------------- //
// ------------------------------------------------------------------------ //

#define RECORD_STOP_EVENT_OFF            0x0000    // no delayed stop poss.
#define RECORD_STOP_EVENT_STOP_BY_SW     0x0001    // stop only by sw command
#define RECORD_STOP_EVENT_STOP_EXTERNAL  0x0002    // stop by signat at Acq.


// ------------------------------------------------------------------------ //
// -- Defines for Set Event Monitor Configuration: ------------------------ //
// ------------------------------------------------------------------------ //

#define EVENT_CONFIG_EXPTRIG_RISING            0x0001   
#define EVENT_CONFIG_EXPTRIG_FALLING           0x0002   
#define EVENT_CONFIG_ACQENBL_RISING            0x0004   
#define EVENT_CONFIG_ACQENBL_FALLING           0x0008   


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Acquire Mode Command: --------------------------- //
// ------------------------------------------------------------------------ //

#define ACQUIRE_MODE_AUTO                    0     // normal auto mode
#define ACQUIRE_MODE_EXTERNAL                1     // ext. as enable signal
#define ACQUIRE_MODE_EXTERNAL_FRAME_TRIGGER  2     // ext. as frame trigger


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Timestamp Mode Command: ------------------------- //
// ------------------------------------------------------------------------ //

#define TIMESTAMP_MODE_OFF              0
#define TIMESTAMP_MODE_BINARY           1
#define TIMESTAMP_MODE_BINARYANDASCII   2
#define TIMESTAMP_MODE_ASCII            3


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Metadata Mode: ---------------------------------- //
// ------------------------------------------------------------------------ //

#define METADATA_MODE_OFF               0x0000
#define METADATA_MODE_ON                0x0001
#define METADATA_MODE_TEST              0x8000


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set PIV Mode Command: ------------------------------- //
// ------------------------------------------------------------------------ //

#define FAST_TIMING_MODE_OFF            0x0000
#define FAST_TIMING_MODE_ON             0x0001     



// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Bit Alignment: ---------------------------------- //
// ------------------------------------------------------------------------ //

#define BIT_ALIGNMENT_MSB               0
#define BIT_ALIGNMENT_LSB               1


// ------------------------------------------------------------------------ //
// -- Defines for GetSensorSignalStatus: ---------------------------------- //
// ------------------------------------------------------------------------ //

#define SIGNAL_STATE_BUSY               0x00000001
#define SIGNAL_STATE_IDLE               0x00000002
#define SIGNAL_STATE_EXP                0x00000004
#define SIGNAL_STATE_READ               0x00000008


// ------------------------------------------------------------------------ //
// -- Defines for Play Images from Segment Modes: ------------------------- //
// ------------------------------------------------------------------------ //

#define PLAY_IMAGES_MODE_OFF                                  0x0000
#define PLAY_IMAGES_MODE_FAST_FORWARD                         0x0001
#define PLAY_IMAGES_MODE_FAST_REWIND                          0x0002
#define PLAY_IMAGES_MODE_SLOW_FORWARD                         0x0003
#define PLAY_IMAGES_MODE_SLOW_REWIND                          0x0004
#define PLAY_IMAGES_MODE_REPLAY_AT_END                        0x0100

#define PLAY_IMAGES_MODE_IS_FORWARD                           0x0001

#define PLAY_POSITION_STATUS_NO_PLAY_ACTIVE                   0x0000
#define PLAY_POSITION_STATUS_VALID                            0x0001


// ------------------------------------------------------------------------ //
// -- Defines for Color Chips    ------------------------------------------ //
// ------------------------------------------------------------------------ //

#define COLOR_RED     0x01
#define COLOR_GREENA  0x02
#define COLOR_GREENBA 0x03
#define COLOR_BLUE    0x04

#define COLOR_CYAN    0x05
#define COLOR_MAGENTA 0x06
#define COLOR_YELLOWA 0x07

#define PATTERN_BAYER 0x01

// ------------------------------------------------------------------------ //
// -- Defines for Modulate mode  ------------------------------------------ //
// ------------------------------------------------------------------------ //

#define MODULATECAPS_MODULATE                 0x00000001
#define MODULATECAPS_MODULATE_EXT_TRIG        0x00000002
#define MODULATECAPS_MODULATE_EXT_EXP         0x00000004
#define MODULATECAPS_MODULATE_ACQ_EXT_FRAME   0x00000008


// ------------------------------------------------------------------------ //
// -- Defines for Get/Set Interface Output Format: ------------------------ //
// ------------------------------------------------------------------------ //

//obsolete: use wInterface definitions below!
#define INTERFACE_HDSDI                                           0x0001
#define INTERFACE_CL_SCCMOS                                       0x0002
#define INTERFACE_USB_PIXELFLY                                    0x0003

//wInterface
#define SET_INTERFACE_HDSDI                                       0x0001 // dimax
#define SET_INTERFACE_CAMERALINK                                  0x0002 // sccmos
#define SET_INTERFACE_USB                                         0x0003 // usb pixelfly
#define SET_INTERFACE_DVI                                         0x0004 // dimax

//wFormat
#define HDSDI_FORMAT_OUTPUT_OFF                                   0x0000
#define HDSDI_FORMAT_1080P25_SINGLE_LINK_RGB                      0x0001
#define HDSDI_FORMAT_1080P25_SINGLE_LINK_RAW10BIT_2_IMAGES        0x0002
#define HDSDI_FORMAT_1080P50_DUAL_LINK_RGB                        0x0003
#define HDSDI_FORMAT_1080P50_DUAL_LINK_RAW10BIT_2_IMAGES          0x0004
#define HDSDI_FORMAT_2048x1536_SINGLE_LINK_RAW12BIT               0x0005
#define HDSDI_FORMAT_2048x1536_DUAL_LINK_RAW12BIT                 0x0006
#define HDSDI_FORMAT_720P50_SINGLE_LINK_RGB                       0x0007

#define HDSDI_FORMAT_720P50_SINGLE_LINK_RAW10BIT_2_IMAGES         0x0008
#define HDSDI_FORMAT_1080P25_SINGLE_LINK_RAW10BIT_1_IMAGE         0x0009
#define HDSDI_FORMAT_720P50_SINGLE_LINK_RAW10BIT_1_IMAGE          0x000A
#define HDSDI_FORMAT_1080P30_SINGLE_LINK_RGB                      0x000B
#define HDSDI_FORMAT_1080P2997_SINGLE_LINK_RGB                    0x000C
#define HDSDI_FORMAT_1080P24_SINGLE_LINK_RGB                      0x000D
#define HDSDI_FORMAT_1080P2398_SINGLE_LINK_RGB                    0x000E
#define HDSDI_FORMAT_1080P60_SINGLE_LINK_RAW10BIT_2_IMAGES        0x000F

#define HDSDI_FORMAT_1080P5994_SINGLE_LINK_RAW10BIT_2_IMAGES      0x0010
#define HDSDI_FORMAT_1080P48_SINGLE_LINK_RAW10BIT_2_IMAGES        0x0011
#define HDSDI_FORMAT_1080P4795_SINGLE_LINK_RAW10BIT_2_IMAGES      0x0012
#define HDSDI_FORMAT_1080P48_DUAL_LINK_RGB                        0x0013
#define HDSDI_FORMAT_1080P4795_DUAL_LINK_RGB                      0x0014
#define HDSDI_FORMAT_1080P96_DUAL_LINK_RAW10BIT_2_IMAGES          0x0015
#define HDSDI_FORMAT_1080P9550_DUAL_LINK_RAW10BIT_2_IMAGES        0x0016
#define HDSDI_FORMAT_720P24_SINGLE_LINK_RGB                       0x0017

#define HDSDI_FORMAT_720P2398_SINGLE_LINK_RGB                     0x0018
#define HDSDI_FORMAT_720P48_SINGLE_LINK_RAW10BIT_2_IMAGES         0x0019
#define HDSDI_FORMAT_720P4795_SINGLE_LINK_RAW10BIT_2_IMAGES       0x001A
#define HDSDI_FORMAT_1080P30_SINGLE_LINK_RAW10BIT_1_IMAGE         0x001B
#define HDSDI_FORMAT_1080P2997_SINGLE_LINK_RAW10BIT_1_IMAGE       0x001C
#define HDSDI_FORMAT_1080P24_SINGLE_LINK_RAW10BIT_1_IMAGE         0x001D
#define HDSDI_FORMAT_1080P2398_SINGLE_LINK_RAW10BIT_1_IMAGE       0x001E
#define HDSDI_FORMAT_1080P60_DUAL_LINK_RGB                        0x001F

#define HDSDI_FORMAT_1080P120_DUAL_LINK_RAW10BIT_2_IMAGES         0x0020
#define HDSDI_FORMAT_720P2398_SINGLE_LINK_RAW10BIT_1_IMAGE        0x0021
#define HDSDI_FORMAT_720P24_SINGLE_LINK_RAW10BIT_1_IMAGE          0x0022
#define HDSDI_FORMAT_720P25_SINGLE_LINK_RAW10BIT_1_IMAGE          0x0023
#define HDSDI_FORMAT_720P25_SINGLE_LINK_RAW10BIT_2_IMAGES         0x0024
#define HDSDI_FORMAT_720P2997_SINGLE_LINK_RAW10BIT_1_IMAGE        0x0025
#define HDSDI_FORMAT_720P2997_SINGLE_LINK_RAW10BIT_2_IMAGES       0x0026
#define HDSDI_FORMAT_720P5994_SINGLE_LINK_RAW10BIT_1_IMAGE        0x0027
#define HDSDI_FORMAT_720P5994_SINGLE_LINK_RAW10BIT_2_IMAGES       0x0028


#define SCCMOS_FORMAT_TOP_BOTTOM                                  0x0000  //Mode E
#define SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER                    0x0100  //Mode A
#define SCCMOS_FORMAT_CENTER_TOP_CENTER_BOTTOM                    0x0200  //Mode B
#define SCCMOS_FORMAT_CENTER_TOP_BOTTOM_CENTER                    0x0300  //Mode C
#define SCCMOS_FORMAT_TOP_CENTER_CENTER_BOTTOM                    0x0400  //Mode D

#define USB_FORMAT_14BIT						                  0x0000
#define USB_FORMAT_12BIT						                  0x0001

// ------------------------------------------------------------------------ //
// -- Defines for Get Image Timing: --------------------------------------- //
// ------------------------------------------------------------------------ //

#define IMAGE_TIMING_NOT_APPLICABLE                           0xFFFFFFFF

// ------------------------------------------------------------------------ //
// -- Defines for Lookup Table: ------------------------------------------- //
// ------------------------------------------------------------------------ //

#define LOOKUPTABLE_FORMAT_8BIT   0x0001
#define LOOKUPTABLE_FORMAT_12BIT  0x0002
#define LOOKUPTABLE_FORMAT_16BIT  0x0004
#define LOOKUPTABLE_FORMAT_24BIT  0x0008
#define LOOKUPTABLE_FORMAT_32BIT  0x0010
#define LOOKUPTABLE_FORMAT_AUTO   0x8000

#endif

// ------------------------------< end of file >--------------------------- //
