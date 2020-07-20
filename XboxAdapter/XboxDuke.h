#ifndef _XBOX_DUKE_H_
#define _XBOX_DUKE_H_

#include "LUFAConfig.h"
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>
#include <avr/power.h>


#define DPAD_UP      0b00000001
#define DPAD_DOWN    0b00000010
#define DPAD_LEFT    0b00000100
#define DPAD_RIGHT   0b00001000
#define START_BTN    0b00010000
#define BACK_BTN     0b00100000
#define LHAT_BTN     0b01000000
#define RHAT_BTN     0b10000000


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   uint8_t dButtons;
   uint8_t A;
   uint8_t B;
   uint8_t X;
   uint8_t Y;
   uint8_t BLACK;
   uint8_t WHITE;
   uint8_t L;
   uint8_t R;
   int16_t leftHatX;
   int16_t leftHatY;
   int16_t rightHatX;
   int16_t rightHatY;
   uint8_t leftRumble;
   uint8_t rightRumble;
   bool updateRumble;
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