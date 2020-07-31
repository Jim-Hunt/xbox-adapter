#ifndef _XBOXONEBT_h_
#define _XBOXONEBT_h_

#include "XBOXONEBTSSP.h"
#include <xboxEnums.h>

#define USB_HID_BOOT_PROTOCOL 0x00
#define HID_RPT_PROTOCOL      0x01

#define L2CAP_STANDARD_CID   0x0001
#define L2CAP_CONTROL_DCID   0x0070
#define L2CAP_INTERRUPT_DCID 0x0071
#define HID_DATA_REQUEST_IN  0xA1
#define HID_DATA_REQUEST_OUT 0xA2

// #define D_UP    0b00000001
// #define D_RIGHT 0b00000011
// #define D_DOWN  0b00000101
// #define D_LEFT  0b00000111
// #define D_A     0b0000000000000001
// #define D_B     0b0000000000000010
// #define D_X     0b0000000000000100
// #define D_Y     0b0000000000001000
// #define D_LS    0b0000000000010000
// #define D_RS    0b0000000000100000
// #define D_BACK  0b0000000001000000
// #define D_START 0b0000000010000000
// #define D_LHAT  0b0000000100000000
// #define D_RHAT  0b0000001000000000

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
   bool connected;
   
   typedef struct {
      uint16_t LeftHatX;
      uint16_t LeftHatY;
      uint16_t RightHatX;
      uint16_t RightHatY;
      uint16_t LeftTrigger;
      uint16_t RightTrigger;
      uint8_t DigitalPad;
      uint16_t DigitalButtons;
   } XboxOneBT_Data_t;
   XboxOneBT_Data_t *Data;

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

   uint16_t ButtonState;
   int16_t hatValue[4];
   uint16_t triggerValue[2];
};

#endif