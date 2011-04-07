//-----------------------------------------------------------------//
// Name        | Pco_ConvStructures.h        | Type: ( ) source    //
//-------------------------------------------|       (*) header    //
// Project     | PCO                         |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | PC                                                //
//-----------------------------------------------------------------//
// Environment | Visual 'C++'                                      //
//-----------------------------------------------------------------//
// Purpose     | PCO - Convert DLL structure definitions           //
//-----------------------------------------------------------------//
// Author      | MBL, FRE, HWI, PCO AG                             //
//-----------------------------------------------------------------//
// Revision    |  rev. 2.00 rel. 2.00                              //
//-----------------------------------------------------------------//
// Notes       |                                                   //
//-----------------------------------------------------------------//
// (c) 2002 PCO AG * Donaupark 11 *                                //
// D-93309      Kelheim / Germany * Phone: +49 (0)9441 / 2005-0 *  //
// Fax: +49 (0)9441 / 2005-20 * Email: info@pco.de                 //
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  1.10     | 03.07.2003 |  gamma, alignement added, FRE          //
//-----------------------------------------------------------------//
//  1.13     | 16.03.2005 |  PCO_CNV_COL_SET added, FRE            //
//-----------------------------------------------------------------//
//  1.15     | 23.10.2007 |  PCO_CNV_COL_SET removed, FRE          //
//           |            |  Adapted all structures due to merging //
//           |            |  the data sets of the dialoges         //
//-----------------------------------------------------------------//
//  2.0      | 08.04.2009 |  New file, FRE                         //
//-----------------------------------------------------------------//

// @ver1.000

// defines for Lut's ...
// local functions

//--------------------
#ifndef PCO_CONVERT_STRUCT_H
#define PCO_CONVERT_STRUCT_H

#ifndef PCO_STRUCTURES_H
#error "Please include PCO_Structures.h before including PCO_ConvStructures.h."
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef dword
#define dword DWORD
#endif
#ifndef word
#define word WORD
#endif
#ifndef byte
#define byte BYTE
#endif

#define PCO_BW_CONVERT      1
#define PCO_COLOR_CONVERT   2
#define PCO_PSEUDO_CONVERT  3
#define PCO_COLOR16_CONVERT 4

typedef struct  {
	WORD          wSize;
	WORD          wDummy;
	int           iScale_maxmax;          // Maximum value for max 
	int           iScale_min;             // Lowest value for processing
	int           iScale_max;             // Highest value for processing
	int           iColor_temp;            // Color temperature  3500...20000
	int           iColor_tint;            // Color correction  -100...100 // 5 int
	int           iColor_saturation;      // Color saturation  -100...100
	int           iColor_vibrance;        // Color dynamic saturation  -100...100
	int           iContrast;              // Contrast  -100...100
	int           iGamma;                 // Gamma  -100...100
	int           iSRGB;				  // sRGB mode
  unsigned char *pucLut;                // Pointer auf Lookup-Table // 10 int
	DWORD     dwzzDummy1[52];             // 64 int
}PCO_Display;

#define BAYER_UPPER_LEFT_IS_RED        0x000000000
#define BAYER_UPPER_LEFT_IS_GREEN_RED  0x000000001
#define BAYER_UPPER_LEFT_IS_GREEN_BLUE 0x000000002
#define BAYER_UPPER_LEFT_IS_BLUE       0x000000003

typedef struct  {
	WORD  wSize;
	WORD  wDummy;
	int   iKernel;                       // Selection of upper left pixel R-Gr-Gb-B
  int   iColorMode;                    // Mode parameter of sensor: 0: Bayer pattern // 3 int
  DWORD dwzzDummy1[61];                // 64 int
}PCO_Bayer;

typedef struct  {
	WORD  wSize;
	WORD  wDummy;
	int   iMode;                         // Noise reduction mode
	int   iType;                         // Noise reduction type // 3 int
	int   iSharpen;                      // Amount sharpen
	DWORD dwzzDummy1[60]; 
}PCO_Filter;

// Flags for actual input data
#define CONVERT_SENSOR_COLORIMAGE     0x0001 // Input data is a color image (see Bayer struct)
#define CONVERT_SENSOR_UPPERALIGNED   0x0002 // Input data is upper aligned
typedef struct {
  WORD wSize;
  WORD wDummy;
  int  iConversionFactor;              // Conversion factor of sensor in 1/100 e/count
  int  iDataBits;                      // Bit resolution of sensor
  int  iSensorInfoBits;                // Flags:
                                       // 0x00000001: Input is a color image (see Bayer struct!)
                                       // 0x00000002: Input is upper aligned
  int  iDarkOffset;                    // Hardware dark offset
  DWORD dwzzDummy0;                    
  SRGBCOLCORRCOEFF strColorCoeff;      // 9 double -> 18int // 24 int
  int  iCamNum;                        // Camera number (enumeration of cameras controlled by app)
  HANDLE hCamera;                      // Handle of the camera loaded, future use; set to zero.
  DWORD dwzzDummy1[38];
}PCO_SensorInfo;


// Flags for output data, directly controlled by the convert mode parameter
#define CONVERT_MODE_OUT_MASK          0xFFFF0000
#define CONVERT_MODE_OUT_FLIPIMAGE     0x0001 // Flip image (top->bottom; bottom->top)
#define CONVERT_MODE_OUT_MIRRORIMAGE   0x0002 // Mirror image (left->right; right->left)
//#define CONVERT_MODE_OUT_TURNIMAGEL   0x0004 // Turn image 90 degree non counterclockwise
//#define CONVERT_MODE_OUT_TURNIMAGER   0x0008 // Turn image 90 degree counterclockwise
#define CONVERT_MODE_OUT_RGB32         0x0100 // Produce 32bit (not 24bit)
#define CONVERT_MODE_EXT_FILTER_FLAGS  0x1000 // Enables the control of the internal flags


// Process internal flags, controlled by the dialog
#define CONVERT_MODE_OUT_DOPOSTPROC    0x010000 // Post processing is enabled (e.g. 16->8, filter, etc.)
#define CONVERT_MODE_OUT_DOLOWPASS     0x020000 // Resulting image will be low pass filtered
#define CONVERT_MODE_OUT_DOBLUR        0x040000 // Resulting color image will be blurred
#define CONVERT_MODE_OUT_DOMEDIAN      0x080000 // Resulting color image will be median filtered
#define CONVERT_MODE_OUT_DOSHARPEN     0x100000 // Resulting color image will sharpened
#define CONVERT_MODE_OUT_DOADSHARPEN   0x200000 // Resulting color image will 'adaptive' sharpened

typedef struct 
{
  WORD  wSize;
  WORD  wDummy[3];
  PCO_Display         str_Display;     // Process settings for output image // 66 int
  PCO_Bayer           str_Bayer;       // Bayer processing settings // 130 int
  PCO_Filter          str_Filter;      // Filter processing settings // 198 int
  PCO_SensorInfo      str_SensorInfo;  // Sensor parameter // 258 int
  int                 iData_Bits_Out;  // Bit resolution of output:
                                       // BW only: 8: 8bit bw
                                       // RGB / BW:
                                       //   Color sensor: 24: 8bit RGB, 32: 8bit RGBA
                                       //   BW sensor: Call to Convert16TO24
                                       //              24: 8bit RGB bw (R=B=G)
                                       //              32: 8bit RGBA bw (R=B=G)
                                       //   BW sensor: Call to Convert16TOPSEUDO
                                       //              24: 8bit RGB (Pseudo color)
                                       //              32: 8bit RGBA (Pseudo color)
                                       // RGB only: 48bit: 16bit RGB, not available with bw sensor
  DWORD               dwModeBitsDataOut;// Flags setting different modes:
                                       // 0x00000001: Flip image
                                       // 0x00000002: Mirror image
                                       // 0x00000010: Produce RGB color output image
                                       // 0x00010000: Do post processing
                                       // 0x00020000: Do low pass filtering
                                       // 0x00040000: Do color blur
  int                 iGPU_Processing_mode;// Mode for processing: 0->CPU, 1->GPU // 261 int
  int                 iConvertType;    // 1: BW, 2: RGB, 3: Pseudo, 4: 16bit-RGB
  DWORD               dwzzDummy1[58];  // 320 int
}PCO_Convert;

#endif /* PCO_CONVERT_STRUCT_H */
