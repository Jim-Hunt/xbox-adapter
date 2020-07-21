#include "XboxDuke.h"
#include <XBOXONE.h>

#define LED_BLINK_FAST 500
#define LED_BLINK_SLOW 4000


USB UsbHost;
XBOXONE XboxOne(&UsbHost);
XboxDuke_Data_t XboxDuke;


void setup()
{
   pinMode(LED_BUILTIN, OUTPUT);
   XboxDuke_Init();
   UsbHost.Init();
}


void loop()
{  
   UsbHost.Task();

   bool gamepadConnected = false;

   if (XboxOne.XboxOneConnected) {
      /* Prepare stick, trigger and button values from the controller. */
      uint8_t digitalButtons = 0b00000000;

      if (XboxOne.getButtonPress(UP))     digitalButtons |= D_UP;
      if (XboxOne.getButtonPress(DOWN))   digitalButtons |= D_DOWN;
      if (XboxOne.getButtonPress(LEFT))   digitalButtons |= D_LEFT;
      if (XboxOne.getButtonPress(RIGHT))  digitalButtons |= D_RIGHT;
      if (XboxOne.getButtonPress(START))  digitalButtons |= D_START;
      if (XboxOne.getButtonPress(BACK))   digitalButtons |= D_BACK;
      if (XboxOne.getButtonPress(L3))     digitalButtons |= D_LEFT_HAT;
      if (XboxOne.getButtonPress(R3))     digitalButtons |= D_RIGHT_HAT;

      XboxDuke.DigitalButtons = digitalButtons;
      XboxDuke.A              = XboxOne.getButtonPress(A)  ? 0xFF : 0x00;
      XboxDuke.B              = XboxOne.getButtonPress(B)  ? 0xFF : 0x00;
      XboxDuke.X              = XboxOne.getButtonPress(X)  ? 0xFF : 0x00;
      XboxDuke.Y              = XboxOne.getButtonPress(Y)  ? 0xFF : 0x00;
      XboxDuke.Black          = XboxOne.getButtonPress(R1) ? 0xFF : 0x00;
      XboxDuke.White          = XboxOne.getButtonPress(L1) ? 0xFF : 0x00;
      XboxDuke.LeftTrigger    = XboxOne.getButtonPress(L2) >> 2;
      XboxDuke.RightTrigger   = XboxOne.getButtonPress(R2) >> 2;
      XboxDuke.LeftHatX       = XboxOne.getAnalogHat(LeftHatX);
      XboxDuke.LeftHatY       = XboxOne.getAnalogHat(LeftHatY);
      XboxDuke.RightHatX      = XboxOne.getAnalogHat(RightHatX);
      XboxDuke.RightHatY      = XboxOne.getAnalogHat(RightHatY);

      /* Pass rumble vaules to the controller. */
      if (XboxDuke.UpdateRumble) {
         if (XboxDuke.LeftRumble == 0x00 && XboxDuke.RightRumble == 0x00) {
            XboxOne.setRumbleOff();
         } else {
            XboxOne.setRumbleOn(0x00, 0x00, XboxDuke.LeftRumble, XboxDuke.RightRumble);
         }
         XboxDuke.UpdateRumble = false;
      }

      gamepadConnected = true;
   }

   if (gamepadConnected && !XboxDuke.XboxDukeConnected) XboxDuke_Attach();
   if (!gamepadConnected && XboxDuke.XboxDukeConnected) XboxDuke_Detach();

   XboxDuke_USBTask();
   
   blink();
}


void blink() 
{
   uint32_t static timer = 0;

   if (millis() > timer) {
      timer += (XboxDuke.XboxDukeConnected) ? LED_BLINK_SLOW : LED_BLINK_FAST;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
   }
}