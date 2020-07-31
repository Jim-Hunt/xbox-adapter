#include "XboxDuke.h"

typedef struct {
   uint8_t startByte;
   uint8_t byteLength;
   uint8_t DigitalButtons;
   uint8_t reserved;
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
} USB_XboxDuke_Data_t;

USB_XboxDuke_Data_t XboxDukePrevReportINBuffer;

USB_ClassInfo_HID_Device_t XboxDuke_HID_Interface = {
   .Config = {
      .InterfaceNumber  = 0x00,
      .ReportINEndpoint = {
         .Address = 0x81,
         .Size    = sizeof(USB_XboxDuke_Data_t),
         .Banks   = 1},
      .PrevReportINBuffer     = &XboxDukePrevReportINBuffer,
      .PrevReportINBufferSize = sizeof(USB_XboxDuke_Data_t),
   }};

uint8_t const DUKE_USB_DESCRIPTOR_DEVICE[] PROGMEM = {
   0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x20,
   0x5E, 0x04, 0x89, 0x02, 0x21, 0x01, 0x00, 0x00,
   0x00, 0x01};

uint8_t const DUKE_USB_DESCRIPTOR_CONFIGURATION[] PROGMEM = {
   0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80,
   0xFA, 0x09, 0x04, 0x00, 0x00, 0x02, 0x58, 0x42,
   0x00, 0x00, 0x07, 0x05, 0x81, 0x03, 0x20, 0x00,
   0x04, 0x07, 0x05, 0x02, 0x03, 0x20, 0x00, 0x04};

uint8_t const DUKE_XID_DESCRIPTOR[] PROGMEM = {
   0x10, 0x42, 0x00, 0x01, 0x01, 0x02, 0x14, 0x06,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t const DUKE_XID_CAPABILITIES_IN[] PROGMEM = {
   0x00, 0x14, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF};

uint8_t const DUKE_XID_CAPABILITIES_OUT[] PROGMEM = {
   0x00, 0x06, 0xFF, 0xFF, 0xFF, 0xFF};

void EVENT_USB_Device_Connect(void)
{
}

void EVENT_USB_Device_Disconnect(void)
{
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
   HID_Device_ConfigureEndpoints(&XboxDuke_HID_Interface);

   /* Host Out endpoint opened manually for Duke. */
   Endpoint_ConfigureEndpoint(0x02, EP_TYPE_INTERRUPT, 0x06, 1);

   /* Enable Start-Of-Frame events. */
   USB_Device_EnableSOFEvents();
}

void EVENT_USB_Device_ControlRequest(void)
{
   /* The Xbox Controller is a HID device, however it has custom vendor requests
      required for the controller to actually work on the console. Some games 
      are more picky than others. These are caught and processed here instead of 
      going to the standard HID driver. See GET_DESCRIPTOR and GET_CAPABILITIES
      on http://xboxdevwiki.net/Xbox_Input_Devices. */

   if (USB_ControlRequest.bmRequestType == 0xC1 &&
       USB_ControlRequest.bRequest == 0x06 &&
       USB_ControlRequest.wValue == 0x4200) {
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(&DUKE_XID_DESCRIPTOR,
                                        sizeof(DUKE_XID_DESCRIPTOR));
      Endpoint_ClearOUT();
      return;
   }

   if (USB_ControlRequest.bmRequestType == 0xC1 &&
       USB_ControlRequest.bRequest == 0x01 &&
       USB_ControlRequest.wValue == 0x0100) {
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(&DUKE_XID_CAPABILITIES_IN,
                                        sizeof(DUKE_XID_CAPABILITIES_IN));
      Endpoint_ClearOUT();
      return;
   }

   if (USB_ControlRequest.bmRequestType == 0xC1 &&
       USB_ControlRequest.bRequest == 0x01 &&
       USB_ControlRequest.wValue == 0x0200) {
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(&DUKE_XID_CAPABILITIES_OUT,
                                        sizeof(DUKE_XID_CAPABILITIES_OUT));
      Endpoint_ClearOUT();
      return;
   }

   /* If it's a standard HID control request have the LUFA library handle it. */
   HID_Device_ProcessControlRequest(&XboxDuke_HID_Interface);
}

void EVENT_USB_Device_StartOfFrame(void)
{
   HID_Device_MillisecondElapsed(&XboxDuke_HID_Interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                                         uint8_t *const ReportID,
                                         uint8_t const ReportType,
                                         void *ReportData,
                                         uint16_t *const ReportSize)
{
   USB_XboxDuke_Data_t *Data = (USB_XboxDuke_Data_t *)ReportData;

   Data->startByte      = 0x00;
   Data->byteLength     = sizeof(USB_XboxDuke_Data_t);
   Data->DigitalButtons = XboxDuke.DigitalButtons;
   Data->reserved       = 0x00;
   Data->A              = XboxDuke.A;
   Data->B              = XboxDuke.B;
   Data->X              = XboxDuke.X;
   Data->Y              = XboxDuke.Y;
   Data->Black          = XboxDuke.Black;
   Data->White          = XboxDuke.White;
   Data->LeftTrigger    = XboxDuke.LeftTrigger;
   Data->RightTrigger   = XboxDuke.RightTrigger;
   Data->LeftHatX       = XboxDuke.LeftHatX;
   Data->LeftHatY       = XboxDuke.LeftHatY;
   Data->RightHatX      = XboxDuke.RightHatX;
   Data->RightHatY      = XboxDuke.RightHatY;

   *ReportSize = sizeof(USB_XboxDuke_Data_t);

   return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                                          uint8_t const ReportID,
                                          uint8_t const ReportType,
                                          void const *ReportData,
                                          uint16_t const ReportSize)
{
   /* Only expect one HID report from the host and this is the rumble levels 
      which is 6 bytes long. See http://euc.jp/periphs/xbox-controller.en.html. */
   if (ReportSize == 0x06) {
      XboxDuke.LeftRumble   = ((uint8_t *)ReportData)[3];
      XboxDuke.RightRumble  = ((uint8_t *)ReportData)[5];
      XboxDuke.UpdateRumble = true;
   }
}

uint16_t CALLBACK_USB_GetDescriptor(uint16_t const wValue,
                                    uint16_t const wIndex,
                                    void const **const DescriptorAddress)
{
   uint8_t const DescriptorType = wValue >> 8;

   switch (DescriptorType) {
      case DTYPE_Device:
         *DescriptorAddress = &DUKE_USB_DESCRIPTOR_DEVICE;
         return sizeof(DUKE_USB_DESCRIPTOR_DEVICE);

      case DTYPE_Configuration:
         *DescriptorAddress = &DUKE_USB_DESCRIPTOR_CONFIGURATION;
         return sizeof(DUKE_USB_DESCRIPTOR_CONFIGURATION);

      default:
         *DescriptorAddress = NULL;
         return NO_DESCRIPTOR;
   }
}

void XboxDuke_Init(void)
{
   /* Setup hardware for LUFA. */
   MCUSR &= ~(1 << WDRF);
   wdt_disable();
   clock_prescale_set(clock_div_1);
   interrupts();
   USB_Init();

   memset(&XboxDuke, 0x00, sizeof(USB_XboxDuke_Data_t));

   USB_Attach();
   XboxDuke.XboxDukeConnected = true;
}

void XboxDuke_Attach(void)
{
   USB_Attach();
   XboxDuke.XboxDukeConnected = true;
}

void XboxDuke_Detach(void)
{
   USB_Detach();
   XboxDuke.XboxDukeConnected = false;
}

void XboxDuke_USBTask(void)
{
   HID_Device_USBTask(&XboxDuke_HID_Interface);
   USB_USBTask();

   /* THPS 2X, Outrun 2 Coast-to-Coast and probably others send rumble commands 
      to the USB OUT pipe instead of the control pipe. So unfortunately need to 
      manually read the out pipe and update the rumble values. */
   uint8_t const ep = Endpoint_GetCurrentEndpoint();
   Endpoint_SelectEndpoint(0x02);

   if (Endpoint_IsOUTReceived()) {
      uint8_t ReportData[0x06];
      Endpoint_Read_Stream_LE(ReportData, sizeof(ReportData), NULL);
      Endpoint_ClearOUT();

      if (ReportData[1] == 0x06) {
         XboxDuke.LeftRumble   = ReportData[3];
         XboxDuke.RightRumble  = ReportData[5];
         XboxDuke.UpdateRumble = true;
      }
   }

   Endpoint_SelectEndpoint(ep);

   /* Turn off rumble on an in-game-reset button combo. */
   // if (XboxDuke.DigitalButtons & D_START &&
   //     XboxDuke.DigitalButtons & D_BACK &&
   //     XboxDuke.LeftTrigger > 0x00 &&
   //     XboxDuke.RightTrigger > 0x00) {
   //    XboxDuke.LeftRumble     = 0x00;
   //    XboxDuke.RightRumble    = 0x00;
   //    XboxDuke.UpdateRumble   = true;
   // }
}