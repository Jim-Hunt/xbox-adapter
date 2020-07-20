#include "XboxDuke.h"
#include <XBOXONE.h>

#define LED_INTERVAL_FAST 500
#define LED_INTERVAL_SLOW 4000

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
      uint8_t dButtons = 0b00000000;

      if (XboxOne.getButtonPress(UP))     dButtons |= DPAD_UP;
      if (XboxOne.getButtonPress(DOWN))   dButtons |= DPAD_DOWN;
      if (XboxOne.getButtonPress(LEFT))   dButtons |= DPAD_LEFT;
      if (XboxOne.getButtonPress(RIGHT))  dButtons |= DPAD_RIGHT;
      if (XboxOne.getButtonPress(START))  dButtons |= START_BTN;
      if (XboxOne.getButtonPress(BACK))   dButtons |= BACK_BTN;
      if (XboxOne.getButtonPress(L3))     dButtons |= LHAT_BTN;
      if (XboxOne.getButtonPress(R3))     dButtons |= RHAT_BTN;

      XboxDuke.dButtons    = dButtons;
      XboxDuke.A           = XboxOne.getButtonPress(A)  ? 0xFF : 0x00;
      XboxDuke.B           = XboxOne.getButtonPress(B)  ? 0xFF : 0x00;
      XboxDuke.X           = XboxOne.getButtonPress(X)  ? 0xFF : 0x00;
      XboxDuke.Y           = XboxOne.getButtonPress(Y)  ? 0xFF : 0x00;
      XboxDuke.BLACK       = XboxOne.getButtonPress(R1) ? 0xFF : 0x00;
      XboxDuke.WHITE       = XboxOne.getButtonPress(L1) ? 0xFF : 0x00;
      XboxDuke.L           = XboxOne.getButtonPress(L2) >> 2;
      XboxDuke.R           = XboxOne.getButtonPress(R2) >> 2;
      XboxDuke.leftHatX    = XboxOne.getAnalogHat(LeftHatX);
      XboxDuke.leftHatY    = XboxOne.getAnalogHat(LeftHatY);
      XboxDuke.rightHatX   = XboxOne.getAnalogHat(RightHatX);
      XboxDuke.rightHatY   = XboxOne.getAnalogHat(RightHatY);

      /* Pass rumble vaules to the controller. */
      if (XboxDuke.updateRumble) {
         if (XboxDuke.leftRumble == 0x00 && XboxDuke.rightRumble == 0x00) {
            XboxOne.setRumbleOff();
         } else {
            XboxOne.setRumbleOn(0x00, 
                                0x00, 
                                XboxDuke.leftRumble, 
                                XboxDuke.rightRumble);
         }
         XboxDuke.updateRumble = false;
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
      timer += (XboxDuke.XboxDukeConnected) ? LED_INTERVAL_SLOW : LED_INTERVAL_FAST;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
   }
}