//-----------------------------------------------------------------//
// Name        | SC2_SDKAddendum.h           | Type: ( ) source    //
//-------------------------------------------|       (*) header    //
// Project     | PCO                         |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | PC                                                //
//-----------------------------------------------------------------//
// Environment | Visual 'C++'                                      //
//-----------------------------------------------------------------//
// Purpose     | PCO - SC2 Camera DLL Functions                    //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    |  rev. 1.06 rel. 1.06                              //
//-----------------------------------------------------------------//

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
//  1.02     | 04.05.2004 | new file added to SDK, FRE, MBL        //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.04     | 16.06.2005 | some defines MBL                       //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.05     | 27.02.2006 |  Added PCO_GetCameraName, FRE          //
//-----------------------------------------------------------------//
//  1.06     | 29.09.2008 |  Added PCO_GIGE_TRANSFER_PARAM, FRE    //
//-----------------------------------------------------------------//

#if !defined SC2_SDKADDENDUM_H

typedef struct _PCO1394_ISO_PARAMS {
   unsigned int bandwidth_bytes;         // 0...4096; recommended: 2000 (default 4096)
   unsigned int speed_of_isotransfer;    // 1,2,4; recommended: 4 (default 4)
   unsigned int number_of_isochannel;    // -1...7; -1 automatic search, 0...7 manual selection
   unsigned int number_of_iso_buffers;   // 16...256; recommended: 128 (default 128)
   unsigned int byte_per_isoframe;       // 0...4096; recommended: 2000 (default 4096)
}PCO1394_ISO_PARAM;

#define PCO_1394_AUTO_CHANNEL   0x200
#define PCO_1394_HOLD_CHANNEL   0x100

#define PCO_1394_DEFAULT_BANDWIDTH 4096
#define PCO_1394_DEFAULT_SPEED 4
#define PCO_1394_DEFAULT_CHANNEL 0x00
#define PCO_1394_DEFAULT_ISOBUFFER 128
#define PCO_1394_DEFAULT_ISOFARME_BYTE 4096


typedef struct _PCO_SC2_CL_TRANSFER_PARAMS {
   unsigned int   baudrate;              // serial baudrate: 9600, 19200, 38400
   unsigned int   ClockFrequency;        // Pixelclock in Hz: 40000000,66000000,80000000
   unsigned int   CCline;    // Usage of CameraLink CC1-CC4 lines, use value returned by Get 
   unsigned int   DataFormat;// register with Testpattern and Datasplitter switch, use value retunred by Get
   unsigned int   Transmit;  // single or continuous transmitting images, 0-single, 1-continuous
}PCO_SC2_CL_TRANSFER_PARAM;

#define PCO_CL_DEFAULT_BAUDRATE 9600
#define PCO_CL_PIXELCLOCK_40MHZ 40000000
#define PCO_CL_PIXELCLOCK_66MHZ 66000000
#define PCO_CL_PIXELCLOCK_80MHZ 80000000
#define PCO_CL_PIXELCLOCK_32MHZ 32000000
#define PCO_CL_PIXELCLOCK_64MHZ 64000000

#define PCO_CL_CCLINE_LINE1_TRIGGER           0x01
#define PCO_CL_CCLINE_LINE2_ACQUIRE           0x02
#define PCO_CL_CCLINE_LINE3_HANDSHAKE         0x04
#define PCO_CL_CCLINE_LINE4_TRANSMIT_ENABLE   0x08

#define PCO_CL_DATAFORMAT_MASK   0x0F
#define PCO_CL_DATAFORMAT_1x16   0x01
#define PCO_CL_DATAFORMAT_2x12   0x02
#define PCO_CL_DATAFORMAT_3x8    0x03
#define PCO_CL_DATAFORMAT_4x16   0x04
#define PCO_CL_DATAFORMAT_5x16   0x05
#define PCO_CL_DATAFORMAT_5x12   0x07     //extract data to 12bit
#define PCO_CL_DATAFORMAT_10x8   0x08
#define PCO_CL_DATAFORMAT_5x12L  0x09     //extract data to 16Bit
#define PCO_CL_DATAFORMAT_5x12R  0x0A     //without extract


#define SCCMOS_FORMAT_MASK                                        0xFF00
#define SCCMOS_FORMAT_TOP_BOTTOM                                  0x0000  //Mode E 
#define SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER                    0x0100  //Mode A
#define SCCMOS_FORMAT_CENTER_TOP_CENTER_BOTTOM                    0x0200  //Mode B
#define SCCMOS_FORMAT_CENTER_TOP_BOTTOM_CENTER                    0x0300  //Mode C
#define SCCMOS_FORMAT_TOP_CENTER_CENTER_BOTTOM                    0x0400  //Mode D 

#define PCO_CL_TRANSMIT_ENABLE  0x01
#define PCO_CL_TRANSMIT_LONGGAP 0x02


typedef struct _PCO_USB_TRANSFER_PARAM {
   unsigned int   MaxNumUsb;           // defines packet size 
   unsigned int   ClockFrequency;      // Pixelclock in Hz: 40000000,66000000,80000000
   unsigned int   Transmit;            // single or continuous transmitting images, 0-single, 1-continuous
   unsigned int   UsbConfig;           // 0=bulk_image, 1=iso_image
   unsigned int   Img12Bit;			   // 1: 12Bit Image 0: 14Bit Image
}PCO_USB_TRANSFER_PARAM;

#define PCO_GIGE_PAKET_RESEND    0x00000001
#define PCO_GIGE_BURST_MODE      0x00000002
#define PCO_GIGE_MAXSPEED_MODE   0x00000004
#define PCO_GIGE_DEBUG_MODE		 0x00000008
#define PCO_GIGE_BW_SAME2ALL	 0x00000000
#define PCO_GIGE_BW_ALL2MAX		 0x00000010
#define PCO_GIGE_BW_2ACTIVE		 0x00000020
#define PCO_GIGE_DATAFORMAT_1x8  0x01080001
#define PCO_GIGE_DATAFORMAT_1x16 0x01100007
#define PCO_GIGE_DATAFORMAT_3x8  0x02180015
#define PCO_GIGE_DATAFORMAT_4x8  0x02200016

typedef struct _PCO_GIGE_TRANSFER_PARAM
{
  DWORD dwPacketDelay;                 // delay between image stream packets (number of clockticks of a 100MHz clock;
                                       // default: 2000 -> 20us, range: 0 ... 8000 -> 0 ... 80us)
  DWORD dwResendPercent;               // Number of lost packets of image in percent. If more packets got lost,
                                       // complete image will be resent or image transfer is failed (default 30).
  DWORD dwFlags;                       // Bit 0:   Set to enable packet resend
                                       // Bit 1:   Set to enable Burst_mode
									   // Bit 2:   Set to enable Max Speed Modus
									   // Bit 3:   Set to enable Camera Debug Mode
                                       // Bit 4-7: Reserved
								       // Bit 8-11:0: Bandwidth is devided by number of connected cameras. PCO_GIGE_BW_SAME2ALL
									   //	       1: Max-Speed-Mode is allways active regardless how many cameras are connected. PCO_GIGE_BW_ALL2MAX
									   //          2: Maximal possible Bandwidth is used for active camera. Just one active camera is allowed. PCO_GIGE_BW_2ACTIVE
									   // Bit 12-31: Reserved
                                       // (LSB; default 0x00000001).      
  DWORD dwDataFormat;                  // DataFormat: default:  0x01100007
                                       // supported types:  Mono, 8Bit:  0x01080001
                                       //                   Mono, 16Bit: 0x01100007
                                       //                   RGB,  24Bit: 0x02180015  (R=G=B=8Bit)
                                       //                   RGB,  32Bit: 0x02200016  (R=G=B=a=8Bit)
  DWORD dwCameraIPAddress;             // Current Ip Address of the Camera
									   //  (can not be changed with Set_Transfer_Param() routine )
  DWORD dwUDPImgPcktSize;			   // Size of an UDP Image packet
									   //  (can not be changed with Set_Transfer_Param() routine )
  UINT64 ui64MACAddress;               // Settings are attached to this interface

}PCO_GIGE_TRANSFER_PARAM; 



//loglevels for interface dll
#define ERROR_M     0x0001
#define INIT_M      0x0002
#define BUFFER_M    0x0004
#define PROCESS_M   0x0008

#define COC_M       0x0010
#define INFO_M      0x0020
#define COMMAND_M   0x0040

#define PCI_M       0x0020

#define TIME_M      0x1000 
#define TIME_MD     0x2000 
 

#endif
