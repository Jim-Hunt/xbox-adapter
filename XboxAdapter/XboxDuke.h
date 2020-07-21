#ifndef _XBOX_DUKE_H_
#define _XBOX_DUKE_H_

#include "LUFAConfig.h"
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>
#include <avr/power.h>

#define D_UP         0b00000001
#define D_DOWN       0b00000010
#define D_LEFT       0b00000100
#define D_RIGHT      0b00001000
#define D_START      0b00010000
#define D_BACK       0b00100000
#define D_LEFT_HAT   0b01000000
#define D_RIGHT_HAT  0b10000000

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
   uint8_t DigitalButtons;
   uint8_t A;
   uint8_t B;
   uint8_t X;
   uint8_t Y;
   uint8_t Black;
   uint8_t White;
   uint8_t LeftTrigger;
   uint8_t RightTrigger;
   int16_t LeftHatX;
   int16_t LeftHatY;
   int16_t RightHatX;
   int16_t RightHatY;
   uint8_t LeftRumble;
   uint8_t RightRumble;
   bool UpdateRumble;
   bool XboxDukeConnected;
} XboxDuke_Data_t;

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_StartOfFrame(void);
uint16_t CALLBACK_USB_GetDescriptor(uint16_t const wValue,
                                    uint16_t const wIndex,
                                    void const **const DescriptorAddress) 
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                                         uint8_t *const ReportID,
                                         uint8_t const ReportType,
                                         void *ReportData,
                                         uint16_t *const ReportSize);
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                                          uint8_t const ReportID,
                                          uint8_t const ReportType,
                                          void const *ReportData,
                                          uint16_t const ReportSize);
void XboxDuke_Init(void);
void XboxDuke_Attach(void);
void XboxDuke_Detach(void);
void XboxDuke_USBTask(void);

extern XboxDuke_Data_t XboxDuke;


#ifdef __cplusplus
}
#endif

#endif