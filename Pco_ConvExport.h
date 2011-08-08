//-----------------------------------------------------------------//
// Name        | Pco_ConvExport.h            | Type: ( ) source    //
//-------------------------------------------|       (*) header    //
// Project     | PCO                         |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | PC                                                //
//-----------------------------------------------------------------//
// Environment | Visual 'C++'                                      //
//-----------------------------------------------------------------//
// Purpose     | PCO - Convert DLL function API definitions        //
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
//  1.12     | 16.03.2005 |  PCO_CNV_COL_SET added, FRE            //
//-----------------------------------------------------------------//
//  1.14     | 12.05.2006 |  conv24 and conv16 added, FRE          //
//-----------------------------------------------------------------//
//  1.15     | 23.10.2007 |  PCO_CNV_COL_SET removed, FRE          //
//           |            |  Adapted all structures due to merging //
//           |            |  the data sets of the dialoges         //
//-----------------------------------------------------------------//
//  2.0      | 08.04.2009 |  New file, FRE                         //
//-----------------------------------------------------------------//
/*
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PCOCNV_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PCOCNV_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
*/


#include "Pco_ConvStructures.h"

#ifdef PCOCONVERT_EXPORTS
// __declspec(dllexport) + .def file funktioniert nicht mit 64Bit Compiler
#ifdef _WIN64
  #define PCOCONVERT_API
#else
  #define PCOCONVERT_API __declspec(dllexport) WINAPI
#endif
#else
#define PCOCONVERT_API __declspec(dllimport) WINAPI
#endif
 

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */




int PCOCONVERT_API PCO_ConvertCreate(HANDLE* ph, PCO_SensorInfo *strSensor, int iConvertType);
//creates a structure PCO_Convert
//allocates memory for the structure
// iConvertType: 1->bw, 2->rgb8, 3->Pseudo, 4->rgb16

int PCOCONVERT_API PCO_ConvertDelete(HANDLE ph);
//delete the LUT 
//free all allocated memory

int PCOCONVERT_API PCO_ConvertGet(HANDLE ph, PCO_Convert* pstrConvert);

int PCOCONVERT_API PCO_ConvertGetDisplay(HANDLE ph, PCO_Display* pstrDisplay);
// Gets the PCO_Display structure
int PCOCONVERT_API PCO_ConvertSetDisplay(HANDLE ph, PCO_Display* pstrDisplay);
// Sets the PCO_Display structure

int PCOCONVERT_API PCO_ConvertSetBayer(HANDLE ph, PCO_Bayer* pstrBayer);
int PCOCONVERT_API PCO_ConvertSetFilter(HANDLE ph, PCO_Filter* pstrFilter);
int PCOCONVERT_API PCO_ConvertSetSensorInfo(HANDLE ph, PCO_SensorInfo* pstrSensorInfo);

int PCOCONVERT_API PCO_LoadPseudoLut(HANDLE ph, int format, char *filename);
//load the three pseudolut color tables of plut
//from the file filename
//which includes data in the following formats

//plut:   PSEUDOLUT to write data in 
//filename: name of file with data
//format: 0 = binary 256*RGB
//        1 = binary 256*R,256*G,256*R
//        2 = ASCII  256*RGB 
//        3 = ASCII  256*R,256*G,256*R

 
int PCOCONVERT_API PCO_Convert16TO8(HANDLE ph, int mode, int icolmode, int width,int height, word *b16, byte *b8);
//convert picture data in b16 to 8bit data in b8 (grayscale)
int PCOCONVERT_API PCO_Convert16TO24(HANDLE ph, int mode, int icolmode, int width,int height, word *b16, byte *b24);
//convert picture data in b16 to 24bit data in b24 (grayscale)
int PCOCONVERT_API PCO_Convert16TOCOL(HANDLE ph, int mode, int icolmode, int width, int height, word *b16, byte *b8);
//convert picture data in b16 to RGB data in b8 (color)
int PCOCONVERT_API PCO_Convert16TOPSEUDO(HANDLE ph, int mode, int icolmode, int width, int height, word *b16, byte *b8);
//convert picture data in b16 to pseudo color data in b8 (color)
int PCOCONVERT_API PCO_Convert16TOCOL16(HANDLE ph, int mode, int icolmode, int width, int height, word *b16in, word *b16out);
//convert picture data in b16 to RGB data in b16 (color)
//through table in structure of PCO_Convert
//mode:   0       = normal
//        bit0: 1 = flip
//        bit1: 1 = mirror  
//width:  width of picture
//height: height of picture
//b12:    pointer to picture data array
//b8:     pointer to byte data array (bw: 1 byte per pixel, rgb: 3 byte pp)
//b24:    pointer to byte data array (RGB, 3 byte per pixel, grayscale)

int PCOCONVERT_API PCO_GetWhiteBalance(HANDLE ph, int *color_temp, int *tint, int mode, int width, int height, word *gb12, int x_min, int y_min, int x_max, int y_max);

int PCOCONVERT_API PCO_GetMaxLimit(float *r_max, float *g_max, float *b_max, float temp, float tint, int output_bits);

int PCOCONVERT_API PCO_GetColorValues(float *pfColorTemp, float *pfColorTint, int iRedMax, int iGreenMax, int iBlueMax);

#ifdef __cplusplus
}
#endif
