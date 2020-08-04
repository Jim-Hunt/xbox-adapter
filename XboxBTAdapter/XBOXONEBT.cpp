#include "XBOXONEBT.h"

XBOXONEBT::XBOXONEBT(XBOXONEBTSSP *p, bool pair) : BluetoothService(p), Data(0x00)
{
   pBtdssp->pairWithHIDDevice = pair;
   Reset();
}

void XBOXONEBT::Reset()
{
   l2cap_event_flag   = 0;
   l2cap_state        = L2CAP_WAIT;
   control_dcid[0]    = L2CAP_CONTROL_DCID;
   control_dcid[1]    = L2CAP_CONTROL_DCID >> 8;
   interrupt_dcid[0]  = L2CAP_INTERRUPT_DCID;
   interrupt_dcid[1]  = L2CAP_INTERRUPT_DCID >> 8;
   activeConnection   = false;
   connected          = false;
   XboxOneBTConnected = false;
}

void XBOXONEBT::disconnect()
{
   pBtdssp->l2cap_disconnection_request(hci_handle,
                                        ++identifier,
                                        interrupt_scid,
                                        interrupt_dcid);
   Reset();
   l2cap_state = L2CAP_INTERRUPT_DISCONNECT;
}

void XBOXONEBT::ACLData(uint8_t *l2capinbuf)
{
   if (!pBtdssp->l2capConnectionClaimed &&
       pBtdssp->incomingHIDDevice &&
       !connected &&
       !activeConnection &&
       l2capinbuf[8] == L2CAP_CMD_CONNECTION_REQUEST &&
       l2capinbuf[12] | (l2capinbuf[13] << 8) == HID_CTRL_PSM) {
      pBtdssp->incomingHIDDevice      = false;
      pBtdssp->l2capConnectionClaimed = true;
      activeConnection                = true;
      hci_handle                      = pBtdssp->hci_handle;
      l2cap_state                     = L2CAP_WAIT;
   }

   if (checkHciHandle(l2capinbuf, hci_handle)) {
      if (l2capinbuf[6] | (l2capinbuf[7] << 8) == L2CAP_STANDARD_CID) {
         switch (l2capinbuf[8]) {
            case L2CAP_CMD_COMMAND_REJECT:
               // Serial1.println(F("L2CAP_CMD_COMMAND_REJECT:ANY"));
               break;

            case L2CAP_CMD_CONNECTION_REQUEST:
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == HID_CTRL_PSM) {
                  // Serial1.println(F("L2CAP_CMD_CONNECTION_REQUEST:HID_CTRL_PSM"));
                  identifier      = l2capinbuf[9];
                  control_scid[0] = l2capinbuf[14];
                  control_scid[1] = l2capinbuf[15];
                  l2cap_set_flag(L2CAP_FLAG_CONNECTION_CONTROL_REQUEST);
               }
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == HID_INTR_PSM) {
                  // Serial1.println(F("L2CAP_CMD_CONNECTION_REQUEST:HID_INTR_PSM"));
                  identifier        = l2capinbuf[9];
                  interrupt_scid[0] = l2capinbuf[14];
                  interrupt_scid[1] = l2capinbuf[15];
                  l2cap_set_flag(L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST);
               }
               break;

            case L2CAP_CMD_CONNECTION_RESPONSE:
               if (l2capinbuf[16] | (l2capinbuf[17] << 8) == 0x0000 &&
                   l2capinbuf[18] | (l2capinbuf[19] << 8) == SUCCESSFUL) {
                  if (l2capinbuf[14] | (l2capinbuf[15] << 8) == L2CAP_CONTROL_DCID) {
                     // Serial1.println(F("HID control connection complete"));
                     identifier      = l2capinbuf[9];
                     control_scid[0] = l2capinbuf[12];
                     control_scid[1] = l2capinbuf[13];
                     l2cap_set_flag(L2CAP_FLAG_CONTROL_CONNECTED);
                  }
                  if (l2capinbuf[14] | (l2capinbuf[15] << 8) == L2CAP_INTERRUPT_DCID) {
                     // Serial1.println(F("HID interrupt connection complete"));
                     identifier        = l2capinbuf[9];
                     interrupt_scid[0] = l2capinbuf[12];
                     interrupt_scid[1] = l2capinbuf[13];
                     l2cap_set_flag(L2CAP_FLAG_INTERRUPT_CONNECTED);
                  }
               }
               break;

            case L2CAP_CMD_CONFIG_REQUEST:
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_CONTROL_DCID) {
                  // Serial1.println(F("L2CAP_CMD_CONFIG_REQUEST:L2CAP_CONTROL_DCID"));
                  pBtdssp->l2cap_config_response(hci_handle,
                                                 l2capinbuf[9],
                                                 control_scid);
               }
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_INTERRUPT_DCID) {
                  // Serial1.println(F("L2CAP_CMD_CONFIG_REQUEST:L2CAP_INTERRUPT_DCID"));
                  pBtdssp->l2cap_config_response(hci_handle,
                                                 l2capinbuf[9],
                                                 interrupt_scid);
               }
               break;

            case L2CAP_CMD_CONFIG_RESPONSE:
               if (l2capinbuf[16] | (l2capinbuf[17] << 8) == 0x0000) {
                  if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_CONTROL_DCID) {
                     // Serial1.println(F("L2CAP_CMD_CONFIG_RESPONSE:L2CAP_CONTROL_DCID"));
                     identifier = l2capinbuf[9];
                     l2cap_set_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS);
                  }
                  if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_INTERRUPT_DCID) {
                     // Serial1.println(F("L2CAP_CMD_CONFIG_RESPONSE:L2CAP_INTERRUPT_DCID"));
                     identifier = l2capinbuf[9];
                     l2cap_set_flag(L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS);
                  }
               }
               break;

            case L2CAP_CMD_DISCONNECT_REQUEST:
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_CONTROL_DCID) {
                  // Serial1.println(F("L2CAP_CMD_DISCONNECT_REQUEST:L2CAP_CONTROL_DCID"));
                  identifier = l2capinbuf[9];
                  pBtdssp->l2cap_disconnection_response(hci_handle,
                                                        identifier,
                                                        control_dcid,
                                                        control_scid);
                  Reset();
               }
               if (l2capinbuf[12] | (l2capinbuf[13] << 8) == L2CAP_INTERRUPT_DCID) {
                  // Serial1.println(F("L2CAP_CMD_DISCONNECT_REQUEST:L2CAP_INTERRUPT_DCID"));
                  identifier = l2capinbuf[9];
                  pBtdssp->l2cap_disconnection_response(hci_handle,
                                                        identifier,
                                                        interrupt_dcid,
                                                        interrupt_scid);
                  Reset();
               }
               break;

            case L2CAP_CMD_DISCONNECT_RESPONSE:
               if (l2capinbuf[12] == control_scid[0] &&
                   l2capinbuf[13] == control_scid[1]) {
                  // Serial1.println(F("L2CAP_CMD_DISCONNECT_RESPONSE:CONTROL"));
                  identifier = l2capinbuf[9];
                  l2cap_set_flag(L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE);
               }
               if (l2capinbuf[12] == interrupt_scid[0] &&
                   l2capinbuf[13] == interrupt_scid[1]) {
                  // Serial1.println(F("L2CAP_CMD_DISCONNECT_RESPONSE:INTERRUPT"));
                  identifier = l2capinbuf[9];
                  l2cap_set_flag(L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE);
               }
               break;
         }
      }

      if (l2capinbuf[6] | (l2capinbuf[7] << 8) == L2CAP_INTERRUPT_DCID &&
          l2capinbuf[8] == HID_DATA_REQUEST_IN) {

         // // Serial.println(l2capinbuf[0], HEX); /* 0x26 HCI handle with PB,BC flag LSB */
         // // Serial.println(l2capinbuf[1], HEX); /* 0x20 HCI handle with PB,BC flag */
         // // Serial.println(l2capinbuf[2], HEX); /* gamepad controls=0x16 (22) LSB */
         // // Serial.println(l2capinbuf[3], HEX); /* 0x00 total length */
         // // Serial.println(l2capinbuf[4], HEX); /* gamepad controls=0x12 (18) data length */
         // // Serial.println(l2capinbuf[5], HEX); /* 0x00 data length */
         // // Serial.println(l2capinbuf[6], HEX); /* 0x71 interrupt destination channel identifier (DCID) LSB */
         // // Serial.println(l2capinbuf[7], HEX); /* 0x00 interrupt destination channel identifier (DCID) */
         // // Serial.println(l2capinbuf[8], HEX); /* 0xA1 ? HID BT DATA_request (0xA0) | Report Type (Input 0x01) ? */
         // // Serial.println(l2capinbuf[9], HEX); /* 0x01=gamepad controls */
         // // Serial.println(l2capinbuf[10], BIN); /* LHAT X LSB */
         // // Serial.println(l2capinbuf[11], BIN); /* LHAT X */
         // // Serial.println(l2capinbuf[12], BIN); /* LHAT Y LSB */
         // // Serial.println(l2capinbuf[13], BIN); /* LHAT Y */
         // // Serial.println(l2capinbuf[14], BIN); /* RHAT X LSB */
         // // Serial.println(l2capinbuf[15], BIN); /* RHAT X */
         // // Serial.println(l2capinbuf[16], BIN); /* RHAT Y LSB */
         // // Serial.println(l2capinbuf[17], BIN); /* RHAT Y */
         // // Serial.println(l2capinbuf[18], BIN); /* LT MSB */
         // // Serial.println(l2capinbuf[19], BIN); /* LT */
         // // Serial.println(l2capinbuf[20], BIN); /* RT MSB */
         // // Serial.println(l2capinbuf[21], BIN); /* RT */
         // // Serial.println(l2capinbuf[22], BIN); /* UP=1, RIGHT=11, DOWN=101, LEFT=111 */
         // // Serial.println(l2capinbuf[23], BIN); /* A=1, B=10, X=1000, Y=10000, LS=1000000, RS=10000000*/
         // // Serial.println(l2capinbuf[24], BIN); /* START=1000, XBOX=10000, LHAT=100000, RHAT=1000000 */
         // // Serial.println(l2capinbuf[25], BIN); /* BACK=1 */

         Data = (XboxOneBT_Data_t *)&l2capinbuf[10];
      }

      L2CAP_task();
   }
}

void XBOXONEBT::L2CAP_task()
{
   switch (l2cap_state) {
      case L2CAP_CONTROL_SUCCESS:
         if (l2cap_check_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)) {
            // Serial1.println(F("HID control successfully configured."));
            setProtocol();
            l2cap_state = L2CAP_INTERRUPT_SETUP;
         }
         break;

      case L2CAP_INTERRUPT_SETUP:
         if (l2cap_check_flag(L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST)) {
            // Serial1.println(F("HID interrupt incoming connection request."));
            pBtdssp->l2cap_connection_response(hci_handle,
                                               identifier,
                                               interrupt_dcid,
                                               interrupt_scid,
                                               PENDING);
            delay(1);
            pBtdssp->l2cap_connection_response(hci_handle,
                                               identifier,
                                               interrupt_dcid,
                                               interrupt_scid,
                                               SUCCESSFUL);
            identifier++;
            delay(1);
            pBtdssp->l2cap_config_request(hci_handle,
                                          identifier,
                                          interrupt_scid);

            l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
         }
         break;

      case L2CAP_CONTROL_CONNECT_REQUEST:
         if (l2cap_check_flag(L2CAP_FLAG_CONTROL_CONNECTED)) {
            // Serial1.println(F("Send HID control config request."));
            identifier++;
            pBtdssp->l2cap_config_request(hci_handle,
                                          identifier,
                                          control_scid);
            l2cap_state = L2CAP_CONTROL_CONFIG_REQUEST;
         }
         break;

      case L2CAP_CONTROL_CONFIG_REQUEST:
         if (l2cap_check_flag(L2CAP_FLAG_CONFIG_CONTROL_SUCCESS)) {
            setProtocol();
            delay(1);
            // Serial1.println(F("Send HID interrupt connection request."));
            identifier++;
            pBtdssp->l2cap_connection_request(hci_handle,
                                              identifier,
                                              interrupt_dcid,
                                              HID_INTR_PSM);
            l2cap_state = L2CAP_INTERRUPT_CONNECT_REQUEST;
         }
         break;

      case L2CAP_INTERRUPT_CONNECT_REQUEST:
         if (l2cap_check_flag(L2CAP_FLAG_INTERRUPT_CONNECTED)) {
            // Serial1.printlnF(("Send HID interrupt config request."));
            identifier++;
            pBtdssp->l2cap_config_request(hci_handle,
                                          identifier,
                                          interrupt_scid);
            l2cap_state = L2CAP_INTERRUPT_CONFIG_REQUEST;
         }
         break;

      case L2CAP_INTERRUPT_CONFIG_REQUEST:
         if (l2cap_check_flag(L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS)) {
            // Serial1.println(F("HID channels established."));
            pBtdssp->connectToHIDDevice = false;
            pBtdssp->pairWithHIDDevice  = false;
            connected                   = true;
            l2cap_state                 = L2CAP_DONE;
            onInit();
         }
         break;

      case L2CAP_DONE:
         break;

      case L2CAP_INTERRUPT_DISCONNECT:
         if (l2cap_check_flag(L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE)) {
            // Serial1.println(F("Disconnected interrupt channel."));
            identifier++;
            pBtdssp->l2cap_disconnection_request(hci_handle,
                                                 identifier,
                                                 control_scid,
                                                 control_dcid);
            l2cap_state = L2CAP_CONTROL_DISCONNECT;
         }
         break;

      case L2CAP_CONTROL_DISCONNECT:
         if (l2cap_check_flag(L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE)) {
            // Serial1.println(F("Disconnected control channel."));
            pBtdssp->hci_disconnect(hci_handle);
            hci_handle       = -1;
            l2cap_event_flag = 0;
            l2cap_state      = L2CAP_WAIT;
         }
         break;
   }
}

void XBOXONEBT::Run()
{
   if (l2cap_state == L2CAP_WAIT) {
      if (pBtdssp->connectToHIDDevice &&
          !pBtdssp->l2capConnectionClaimed &&
          !connected &&
          !activeConnection) {
         // Serial1.println(F("Send HID control connection request."));
         pBtdssp->l2capConnectionClaimed = true;
         activeConnection                = true;
         hci_handle                      = pBtdssp->hci_handle;
         l2cap_event_flag                = 0;
         identifier                      = 0;
         pBtdssp->l2cap_connection_request(hci_handle,
                                           identifier,
                                           control_dcid,
                                           HID_CTRL_PSM);
         l2cap_state = L2CAP_CONTROL_CONNECT_REQUEST;
         return;
      }

      if (l2cap_check_flag(L2CAP_FLAG_CONNECTION_CONTROL_REQUEST)) {
         // Serial1.println(F("HID control incoming connection request."));
         pBtdssp->l2cap_connection_response(hci_handle,
                                            identifier,
                                            control_dcid,
                                            control_scid,
                                            PENDING);
         delay(1);
         pBtdssp->l2cap_connection_response(hci_handle,
                                            identifier,
                                            control_dcid,
                                            control_scid,
                                            SUCCESSFUL);
         identifier++;
         delay(1);
         pBtdssp->l2cap_config_request(hci_handle,
                                       identifier,
                                       control_scid);
         l2cap_state = L2CAP_CONTROL_SUCCESS;
         return;
      }
   }
}

void XBOXONEBT::setProtocol()
{
   // Serial1.println(F("Set protocol mode."));
   uint8_t command = 0x70 | HID_RPT_PROTOCOL;
   // Serial1.println(command, HEX);
   pBtdssp->L2CAP_Command(hci_handle,
                          &command,
                          1,
                          control_scid[0],
                          control_scid[1]);
}

void XBOXONEBT::pair(void)
{
   if (pBtdssp) pBtdssp->pairWithHID();
};

void XBOXONEBT::onInit()
{
   /* References used to get a working rumble over Bluetooth:
      https://github.com/felis/USB_Host_Shield_2.0/blob/master/BTHID.cpp
      https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXONE.cpp
      https://github.com/chromium/chromium/blob/master/device/gamepad/xbox_hid_controller.cc */

   // Serial1.println(F("Trying to send a rumble."));

   uint8_t buf[10];
   buf[0] = 0xA2; /* HID BT DATA_request (0xA0) | Report Type (Output 0x02) */
   buf[1] = 0x03; /* Report id */
   buf[2] = 0x03; /* Enable rumble motors, disable trigger haptics */
   buf[3] = 0x00; /* L trigger force */
   buf[4] = 0x00; /* R trigger force */
   buf[5] = 0x20; /* L actuator force */
   buf[6] = 0x20; /* R actuator force */
   buf[7] = 0x80; /* Duration */
   buf[8] = 0x00; /* Delay */
   buf[9] = 0x01; /* Loop */

   pBtdssp->L2CAP_Command(hci_handle,
                          buf,
                          sizeof(buf),
                          interrupt_scid[0],
                          interrupt_scid[1]);

   XboxOneBTConnected = true;
};

uint16_t XBOXONEBT::getButtonPress(ButtonEnum b)
{
   switch (b) {
      case L2:
         return Data->LeftTrigger;

      case R2:
         return Data->RightTrigger;

      /* UP=1, RIGHT=11, DOWN=101, LEFT=111 */
      case UP:
         return (lowByte(Data->DigitalButtons) == 0b00000001);

      case RIGHT:
         return (lowByte(Data->DigitalButtons) == 0b00000011);

      case DOWN:
         return (lowByte(Data->DigitalButtons) == 0b00000101);

      case LEFT:
         return (lowByte(Data->DigitalButtons) == 0b00000111);

      /* A=1, B=10, X=1000, Y=10000, LS=1000000, RS=10000000 */
      case A:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00000001 << 8);

      case B:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00000010 << 8);

      case X:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00001000 << 8);

      case Y:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00010000 << 8);

      case L1:
         return (bool)(Data->DigitalButtons & (uint32_t)0b01000000 << 8);

      case R1:
         return (bool)(Data->DigitalButtons & (uint32_t)0b10000000 << 8);

      /* START=1000, XBOX=10000, LHAT=100000, RHAT=1000000 */
      case START:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00001000 << 16);

      case XBOX:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00010000 << 16);

      case L3:
         return (bool)(Data->DigitalButtons & (uint32_t)0b00100000 << 16);

      case R3:
         return (bool)(Data->DigitalButtons & (uint32_t)0b01000000 << 16);

         /* BACK=1 */

      case BACK:
         return (bool)(Data->DigitalButtons & ((uint32_t)0b00000001 << 24));
   }
}

int16_t XBOXONEBT::getAnalogHat(AnalogHatEnum a)
{
   switch (a) {
      case LeftHatX:
         return (int16_t)(Data->LeftHatX + INT16_MIN);

      case LeftHatY:
         return (int16_t)(INT16_MAX - Data->LeftHatY);

      case RightHatX:
         return (int16_t)(Data->RightHatX + INT16_MIN);

      case RightHatY:
         return (int16_t)(INT16_MAX - Data->RightHatY);
   }
}

void XBOXONEBT::setRumbleOff()
{
   uint8_t buf[10];

   buf[0] = 0xA2; /* HID BT DATA_request (0xA0) | Report Type (Output 0x02) */
   buf[1] = 0x03; /* Report id */
   buf[2] = 0x0F; /* Rumble mask (0000 lT rT L R) */
   buf[3] = 0x00; /* L trigger force */
   buf[4] = 0x00; /* R trigger force */
   buf[5] = 0x00; /* L actuator force */
   buf[6] = 0x00; /* R actuator force */
   buf[7] = 0x00; /* Duration */
   buf[8] = 0x00; /* Delay */
   buf[9] = 0x00; /* Repeat */

   pBtdssp->L2CAP_Command(hci_handle,
                          buf,
                          sizeof(buf),
                          interrupt_scid[0],
                          interrupt_scid[1]);
}

void XBOXONEBT::setRumbleOn(uint8_t leftTrigger,
                            uint8_t rightTrigger,
                            uint8_t leftMotor,
                            uint8_t rightMotor)
{
   uint8_t buf[10];

   buf[0] = 0xA2;         /* HID BT DATA_request (0xA0) | Report Type (Output 0x02) */
   buf[1] = 0x03;         /* Report id */
   buf[2] = 0x0F;         /* Rumble mask (0000 lT rT L R) */
   buf[3] = leftTrigger;  /* L trigger force */
   buf[4] = rightTrigger; /* R trigger force */
   buf[5] = leftMotor;    /* L actuator force */
   buf[6] = rightMotor;   /* R actuator force */
   buf[7] = 0xFF;         /* Duration */
   buf[8] = 0x00;         /* Delay */
   buf[9] = 0xFF;         /* Repeat */

   pBtdssp->L2CAP_Command(hci_handle,
                          buf,
                          sizeof(buf),
                          interrupt_scid[0],
                          interrupt_scid[1]);
}