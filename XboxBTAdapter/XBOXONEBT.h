#ifndef _XBOXONEBT_h_
#define _XBOXONEBT_h_

#include "XBOXONEBTSSP.h"
#include <xboxEnums.h>

#define USB_HID_BOOT_PROTOCOL 0x00
#define HID_RPT_PROTOCOL      0x01
#define L2CAP_STANDARD_CID    0x0001
#define L2CAP_CONTROL_DCID    0x0070
#define L2CAP_INTERRUPT_DCID  0x0071
#define HID_DATA_REQUEST_IN   0xA1
#define HID_DATA_REQUEST_OUT  0xA2

class XBOXONEBT : public BluetoothService
{
public:
   XBOXONEBT(XBOXONEBTSSP *p, bool pair = false);
   void disconnect();
   void pair(void);
   uint16_t getButtonPress(ButtonEnum b);
   int16_t getAnalogHat(AnalogHatEnum a);
   void setRumbleOn(uint8_t leftTrigger,
                    uint8_t rightTrigger,
                    uint8_t leftMotor,
                    uint8_t rightMotor);
   void setRumbleOff();
   bool XboxOneBTConnected;

protected:
   void ACLData(uint8_t *ACLData);
   void Run();
   void Reset();
   void onInit();

private:
   void setProtocol();
   void L2CAP_task();
   uint8_t interrupt_dcid[2];
   uint8_t control_dcid[2];
   uint8_t interrupt_scid[2];
   uint8_t control_scid[2];
   uint8_t l2cap_state;
   bool activeConnection;
   bool connected;

   struct XboxOneBT_Data_t {
      uint16_t LeftHatX;
      uint16_t LeftHatY;
      uint16_t RightHatX;
      uint16_t RightHatY;
      uint16_t LeftTrigger;
      uint16_t RightTrigger;
      uint32_t DigitalButtons;
   };
   XboxOneBT_Data_t *Data;
};

#endif