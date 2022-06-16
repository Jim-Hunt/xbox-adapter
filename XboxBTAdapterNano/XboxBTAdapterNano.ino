
#include <hardware/flash.h>
#include <hardware/irq.h>
#include <pico/bootrom.h>
#include <pico/mutex.h>
#include <pico/time.h>
#include <mbed.h>
#include "src/Bluepad32/Bluepad32.h"
#include "src/TinyUSB/tusb.h"

#define USB_TASK_INTERVAL 1000
#define USB_TASK_IRQ 31
#define get_unique_id(_serial) flash_get_unique_id(_serial)

#include "xid_duke.h"

GamepadPtr myGamepad;

USB_XboxGamepad_InReport_t xpad_data;
USB_XboxGamepad_OutReport_t xpad_rumble;

mutex_t usb_mutex;
static mbed::Ticker usb_ticker;

static void ticker_task(void) {
  irq_set_pending(USB_TASK_IRQ); 
}

static void setup_periodic_usb_handler(uint64_t us) {
  usb_ticker.attach(ticker_task, (std::chrono::microseconds)us);
}

static void usb_irq() {
  // if the mutex is already owned, then we are in user code
  // in this file which will do a tud_task itself, so we'll just do nothing
  // until the next tick
  if (mutex_try_enter(&usb_mutex, NULL)) {
    tud_task();
    mutex_exit(&usb_mutex);
  }
}

void setup() {
  mutex_init(&usb_mutex);

  irq_set_enabled(USBCTRL_IRQ, false);
  irq_handler_t current_handler = irq_get_exclusive_handler(USBCTRL_IRQ);
  if (current_handler) {
    irq_remove_handler(USBCTRL_IRQ, current_handler);
  }

  tusb_init();
  tud_disconnect();

  // soft irq for periodically task runner
  irq_set_exclusive_handler(USB_TASK_IRQ, usb_irq);
  irq_set_enabled(USB_TASK_IRQ, true);
  setup_periodic_usb_handler(USB_TASK_INTERVAL);

  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  pinMode(LED_BUILTIN, OUTPUT);
}

void onConnectedGamepad(GamepadPtr gp) {
  myGamepad = gp;
  tud_connect();
  digitalWrite(LED_BUILTIN, HIGH);
}

void onDisconnectedGamepad(GamepadPtr gp) {
  myGamepad = nullptr;
  tud_disconnect();
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  BP32.update();

  if (xid_send_report_ready() && myGamepad && myGamepad->isConnected()) {
    memset(&xpad_data, 0x00, sizeof(xpad_data));
    xpad_data.bLength = sizeof(xpad_data);

    xpad_data.A     = myGamepad->a() ? 0xFF : 0x00;
    xpad_data.B     = myGamepad->b() ? 0xFF : 0x00;
    xpad_data.X     = myGamepad->x() ? 0xFF : 0x00;
    xpad_data.Y     = myGamepad->y() ? 0xFF : 0x00;
    xpad_data.WHITE = myGamepad->l1() ? 0xFF : 0x00;
    xpad_data.BLACK = myGamepad->r1() ? 0xFF : 0x00;
    xpad_data.L     = myGamepad->brake() >> 2;
    xpad_data.R     = myGamepad->throttle() >> 2;

    xpad_data.leftStickX  = myGamepad->axisX() << 6;
    xpad_data.leftStickY  = -max(myGamepad->axisY() << 6, INT16_MIN + 1);
    xpad_data.rightStickX = myGamepad->axisRX() << 6;
    xpad_data.rightStickY = -max(myGamepad->axisRY() << 6, INT16_MIN + 1);
    
    if (myGamepad->dpad() & Gamepad::DPAD_UP) xpad_data.dButtons |= XID_DUP;
    if (myGamepad->dpad() & Gamepad::DPAD_DOWN) xpad_data.dButtons |= XID_DDOWN;
    if (myGamepad->dpad() & Gamepad::DPAD_LEFT) xpad_data.dButtons |= XID_DLEFT;
    if (myGamepad->dpad() & Gamepad::DPAD_RIGHT) xpad_data.dButtons |= XID_DRIGHT;
    if (myGamepad->thumbL()) xpad_data.dButtons |= XID_LS;
    if (myGamepad->thumbR()) xpad_data.dButtons |= XID_RS;
    if (myGamepad->miscHome()) xpad_data.dButtons |= XID_START;
    if (myGamepad->miscBack()) xpad_data.dButtons |= XID_BACK;
    
    xid_send_report(&xpad_data, sizeof(xpad_data));
  }

  static uint32_t timer = 0;
  static uint16_t old_rumble_l, old_rumble_r;
  if (millis() - timer > 40 &&
      myGamepad && myGamepad->isConnected() &&
      xid_get_report(&xpad_rumble, sizeof(xpad_rumble)))
  {
    if (xpad_rumble.lValue != old_rumble_l || xpad_rumble.rValue != old_rumble_r)
    {
      old_rumble_l = xpad_rumble.lValue;
      old_rumble_r = xpad_rumble.rValue;
      
      // Xbox rumble 
      uint8_t l_rumble, r_rumble, duration;
      l_rumble = xpad_rumble.lValue >> 8;
      r_rumble = xpad_rumble.rValue >> 8;
      duration = (l_rumble > 0x00 || r_rumble > 0x00) ? 0xFF : 0x00;

      // Bluepad32 takes just one rumble value sending it to all rumble motors.
      // Firmware + library update needed to independently set left and right.
      myGamepad->setRumble2(l_rumble, r_rumble, duration);

      timer = millis();
    }
  }

  // I don't know why this is needed, but it makes a huge difference to the 
  // stability of something
  delay(1);

}
