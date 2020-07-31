#include "XboxDuke.h"
#include "XBOXONEBT.h"
#include <SPI.h>

#define LED_BLINK_FAST 500
#define LED_BLINK_SLOW 4000

USB UsbHost;
XBOXONEBTSSP XboxOneBTSSP(&UsbHost);
/* This will start an inquiry and then pair with your device. */
// XBOXONEBT XboxOneBT(&XboxOneBTSSP, PAIR);
/* After that you can simply create the instance like so. */
XBOXONEBT XboxOneBT(&XboxOneBTSSP);

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

   if (XboxOneBT.connected) {
      /* Button, trigger and hat values FROM the gamepad. */
      uint8_t digitalButtons = 0b00000000;

      if (XboxOneBT.getButtonPress(UP)) digitalButtons |= D_UP;
      if (XboxOneBT.getButtonPress(DOWN)) digitalButtons |= D_DOWN;
      if (XboxOneBT.getButtonPress(LEFT)) digitalButtons |= D_LEFT;
      if (XboxOneBT.getButtonPress(RIGHT)) digitalButtons |= D_RIGHT;
      if (XboxOneBT.getButtonPress(START)) digitalButtons |= D_START;
      if (XboxOneBT.getButtonPress(BACK)) digitalButtons |= D_BACK;
      if (XboxOneBT.getButtonPress(L3)) digitalButtons |= D_LEFT_HAT;
      if (XboxOneBT.getButtonPress(R3)) digitalButtons |= D_RIGHT_HAT;

      XboxDuke.DigitalButtons = digitalButtons;
      XboxDuke.A              = XboxOneBT.getButtonPress(A) ? 0xFF : 0x00;
      XboxDuke.B              = XboxOneBT.getButtonPress(B) ? 0xFF : 0x00;
      XboxDuke.X              = XboxOneBT.getButtonPress(X) ? 0xFF : 0x00;
      XboxDuke.Y              = XboxOneBT.getButtonPress(Y) ? 0xFF : 0x00;
      XboxDuke.Black          = XboxOneBT.getButtonPress(R1) ? 0xFF : 0x00;
      XboxDuke.White          = XboxOneBT.getButtonPress(L1) ? 0xFF : 0x00;
      XboxDuke.LeftTrigger    = XboxOneBT.getButtonPress(L2) >> 2;
      XboxDuke.RightTrigger   = XboxOneBT.getButtonPress(R2) >> 2;
      XboxDuke.LeftHatX       = XboxOneBT.getAnalogHat(LeftHatX);
      XboxDuke.LeftHatY       = XboxOneBT.getAnalogHat(LeftHatY);
      XboxDuke.RightHatX      = XboxOneBT.getAnalogHat(RightHatX);
      XboxDuke.RightHatY      = XboxOneBT.getAnalogHat(RightHatY);

      /* Rumble values TO the gamepad. */
      // if (XboxDuke.UpdateRumble) {
      //    if (XboxDuke.LeftRumble == 0x00 && XboxDuke.RightRumble == 0x00)
      //       XboxOneBT.setRumbleOff();
      //    else
      //       XboxOneBT.setRumbleOn(0x00, 0x00, XboxDuke.LeftRumble, XboxDuke.RightRumble);
      //    XboxDuke.UpdateRumble = false;
      // }

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