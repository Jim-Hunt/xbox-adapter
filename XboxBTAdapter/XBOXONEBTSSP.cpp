#include "XBOXONEBTSSP.h"

const uint8_t XBOXONEBTSSP::BTDSSP_CONTROL_PIPE = 0;
const uint8_t XBOXONEBTSSP::BTDSSP_EVENT_PIPE   = 1;
const uint8_t XBOXONEBTSSP::BTDSSP_DATAIN_PIPE  = 2;
const uint8_t XBOXONEBTSSP::BTDSSP_DATAOUT_PIPE = 3;

XBOXONEBTSSP::XBOXONEBTSSP(USB *p) : connectToHIDDevice(false),
                                     pairWithHIDDevice(false),
                                     pairedDevice(false),
                                     pUsb(p),          /* Pointer to USB class instance - mandatory */
                                     bAddress(0),      /* Device address - mandatory */
                                     bNumEP(1),        /* If config descriptor needs to be parsed */
                                     qNextPollTime(0), /* Reset NextPollTime */
                                     pollInterval(0),
                                     bPollEnable(false) /* Don't start polling before dongle is connected */
{
   for (uint8_t i = 0; i < BTDSSP_NUM_SERVICES; i++)
      btService[i] = NULL;

   Initialize();

   if (pUsb) pUsb->RegisterDeviceClass(this);
}

uint8_t XBOXONEBTSSP::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed)
{
   const uint8_t constBufSize = sizeof(USB_DEVICE_DESCRIPTOR);
   uint8_t buf[constBufSize];
   USB_DEVICE_DESCRIPTOR *udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR *>(buf);
   UsbDevice *p               = NULL;
   EpInfo *oldep_ptr          = NULL;
   uint8_t rcode;

   Initialize();

   /* Get memory address of USB device address pool */
   AddressPool &addrPool = pUsb->GetAddressPool();
   // Serial1.println(F("BTDSSP configure device."));

   /* Check if address has already been assigned to an instance */
   if (bAddress) {
      // Serial1.println(F("Address in use."));
      return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
   }

   /* Get pointer to pseudo device with address 0 assigned */
   p = addrPool.GetUsbDevicePtr(0);
   if (!p) {
      // Serial1.println(F("Address not found."));
      return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
   }

   if (!p->epinfo) {
      // Serial1.println(F("Epinfo is null."));
      return USB_ERROR_EPINFO_IS_NULL;
   }

   /* Save old pointer to EP_RECORD of address 0 */
   oldep_ptr = p->epinfo;

   /* Temporary assign new pointer to epInfo to p->epinfo in order to avoid 
      toggle inconsistency */
   p->epinfo   = epInfo;
   p->lowspeed = lowspeed;

   /* Get device descriptor - addr, ep, nbytes, data */
   rcode = pUsb->getDevDescr(0, 0, constBufSize, (uint8_t *)buf);

   /* Restore p->epinfo */
   p->epinfo = oldep_ptr;

   if (rcode) goto FailGetDevDescr;

   /* Allocate new address according to device class */
   bAddress = addrPool.AllocAddress(parent, false, port);

   if (!bAddress) {
      // Serial1.println(F("Out of address space."));
      return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;
   }

   /* Some dongles have an USB hub inside */
   if (udd->bDeviceClass == 0x09) goto FailHub;

   /* Extract max packet size from device descriptor */
   epInfo[0].maxPktSize = udd->bMaxPacketSize0;
   /* Steal and abuse from epInfo structure to save memory */
   epInfo[1].epAddr = udd->bNumConfigurations;

   VID = udd->idVendor;
   PID = udd->idProduct;

   return USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET;

FailHub:
   // Serial1.println(F("Create a hub instance in your code."));
   /* Reset address */
   pUsb->setAddr(bAddress, 0, 0);
   rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;
   Release();
   return rcode;

FailGetDevDescr:
#ifdef DEBUG_USB_HOST
   NotifyFailGetDevDescr(rcode);
#endif
   if (rcode != hrJERR)
      rcode = USB_ERROR_FailGetDevDescr;
   Release();
   return rcode;
};

uint8_t XBOXONEBTSSP::Init(uint8_t parent __attribute__((unused)),
                           uint8_t port __attribute__((unused)),
                           bool lowspeed)
{
   uint8_t rcode;
   uint8_t num_of_conf = epInfo[1].epAddr;
   epInfo[1].epAddr    = 0;

   AddressPool &addrPool = pUsb->GetAddressPool();
   // Serial1.println(F("BTDSSP Init."));

   /* Get pointer to assigned address record */
   UsbDevice *p = addrPool.GetUsbDevicePtr(bAddress);

   if (!p) {
      // Serial1.println(F("Address not found."));
      return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
   }

   delay(300);

   /* Assign new address to the device */
   rcode = pUsb->setAddr(0, 0, bAddress);
   if (rcode) {
      // Serial1.print(F("setAddr: "));
      // Serial1.println(rcode, HEX);
      p->lowspeed = false;
      goto Fail;
   }

   // Serial1.print(F("Addr: "));
   // Serial1.println(bAddress, HEX);

   p->lowspeed = false;

   /* Get pointer to assigned address record */
   p = addrPool.GetUsbDevicePtr(bAddress);
   if (!p) {
      // Serial1.println(F("Address not found."));
      return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
   }

   p->lowspeed = lowspeed;

   /* Assign epInfo to epinfo pointer - only EP0 is known */
   rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
   if (rcode)
      goto FailSetDevTblEntry;

   for (uint8_t i = 0; i < num_of_conf; i++) {
      /* Set class id according to the specification */
      ConfigDescParser<USB_CLASS_WIRELESS_CTRL,
                       WI_SUBCLASS_RF,
                       WI_PROTOCOL_BT,
                       CP_MASK_COMPARE_ALL>
         confDescrParser(this);

      rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);

      /* Check error code */
      if (rcode)
         goto FailGetConfDescr;
      if (bNumEP >= BTDSSP_MAX_ENDPOINTS)
         break;
   }

   if (bNumEP < BTDSSP_MAX_ENDPOINTS)
      goto FailUnknownDevice;

   /* Assign epInfo to epinfo pointer - this time all 3 endpoints */
   rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

   if (rcode) goto FailSetDevTblEntry;

   /* Set Configuration Value */
   rcode = pUsb->setConf(bAddress, epInfo[BTDSSP_CONTROL_PIPE].epAddr, bConfNum);

   if (rcode) goto FailSetConfDescr;

   /* only loop 100 times before trying to send the hci reset command */
   hci_num_reset_loops  = 100;
   hci_counter          = 0;
   hci_state            = HCI_INIT_STATE;
   waitingForConnection = false;
   bPollEnable          = true;

   // Serial1.println(F("Bluetooth Dongle Initialized."));

   /* Successful configuration */
   return 0;

   /* Diagnostic messages */
FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
   NotifyFailSetDevTblEntry();
#endif
   goto Fail;

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
   NotifyFailGetConfDescr();
#endif
   goto Fail;

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
   NotifyFailSetConfDescr();
#endif
   goto Fail;

FailUnknownDevice:
#ifdef DEBUG_USB_HOST
   NotifyFailUnknownDevice(VID, PID);
#endif
   /* Reset address */
   pUsb->setAddr(bAddress, 0, 0);
   rcode = USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

Fail:
   // Serial1.println(F("BTDSSP init failed, error code:"));
   // Serial1.println(rcode, HEX);
   Release();
   return rcode;
}

void XBOXONEBTSSP::Initialize()
{
   uint8_t i;
   for (i = 0; i < BTDSSP_MAX_ENDPOINTS; i++) {
      epInfo[i].epAddr      = 0;
      epInfo[i].maxPktSize  = (i) ? 0 : 8;
      epInfo[i].bmSndToggle = 0;
      epInfo[i].bmRcvToggle = 0;
      epInfo[i].bmNakPower  = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
   }

   /* Reset all Bluetooth services */
   for (i = 0; i < BTDSSP_NUM_SERVICES; i++) {
      if (btService[i])
         btService[i]->Reset();
   }

   connectToHIDDevice = false;
   incomingHIDDevice  = false;
   pollInterval       = 0;
   qNextPollTime      = 0;     /* Reset next poll time */
   bPollEnable        = false; /* Don't start polling before dongle is connected */
   bAddress           = 0;     /* Clear device address */
   bNumEP             = 1;     /* Must have to be reset to 1 */
}

/* Extracts interrupt-IN, bulk-IN, bulk-OUT endpoint info from config descriptor */
void XBOXONEBTSSP::EndpointXtract(uint8_t conf,
                                  uint8_t iface __attribute__((unused)),
                                  uint8_t alt,
                                  uint8_t proto __attribute__((unused)),
                                  const USB_ENDPOINT_DESCRIPTOR *pep)
{
   /* Wrong interface - by BT spec, no alt setting */
   if (alt) return;

   bConfNum = conf;
   uint8_t index;

   if ((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_INTERRUPT &&
       (pep->bEndpointAddress & 0x80) == 0x80) {
      /* Interrupt In endpoint found */
      index                    = BTDSSP_EVENT_PIPE;
      epInfo[index].bmNakPower = USB_NAK_NOWAIT;
   } else if ((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_BULK)
      /* Bulk endpoint found */
      index = ((pep->bEndpointAddress & 0x80) == 0x80) ? BTDSSP_DATAIN_PIPE : BTDSSP_DATAOUT_PIPE;
   else
      return;

   /* Fill the rest of endpoint data structure */
   epInfo[index].epAddr     = (pep->bEndpointAddress & 0x0F);
   epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;

   // // Serial1.println(F("Endpoint descriptor."));
   // // Serial1.print(F("Length: "));
   // // Serial1.print(pep->bLength, HEX);
   // // Serial1.print(F("Type: "));
   // // Serial1.print(pep->bDescriptorType, HEX);
   // // Serial1.print(F("Address: "));
   // // Serial1.print(pep->bEndpointAddress, HEX);
   // // Serial1.print(F("Attributes: "));
   // // Serial1.print(pep->bmAttributes, HEX);
   // // Serial1.print(F("MaxPktSize: "));
   // // Serial1.print(pep->wMaxPacketSize, HEX);
   // // Serial1.print(F("Poll Interval: "));
   // // Serial1.print(pep->bInterval, HEX);

   /* Set the polling interval as the largest obtained from endpoints */
   if (pollInterval < pep->bInterval) pollInterval = pep->bInterval;
   bNumEP++;
}

/* Performs a cleanup after failed Init() attempt */
uint8_t XBOXONEBTSSP::Release()
{
   Initialize();
   pUsb->GetAddressPool().FreeAddress(bAddress);
   return 0;
}

uint8_t XBOXONEBTSSP::Poll()
{
   if (!bPollEnable) return 0;

   if ((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) { /* Don't poll if shorter than polling interval */
      qNextPollTime = (uint32_t)millis() + pollInterval;      /* Set new poll time */
      HCI_event_task();                                       /* Poll the HCI event pipe */
      HCI_task();                                             /* HCI state machine */
      ACL_event_task();                                       /* Poll the ACL input pipe too */
   }

   return 0;
}

void XBOXONEBTSSP::disconnect()
{
   for (uint8_t i = 0; i < BTDSSP_NUM_SERVICES; i++) {
      if (btService[i]) btService[i]->disconnect();
   }
};

void XBOXONEBTSSP::HCI_event_task()
{
   /* Request more than 16 bytes anyway, the inTransfer routine will take care of this */
   uint16_t length = BULK_MAXPKTSIZE;

   /* Input on endpoint 1 */
   uint8_t rcode = pUsb->inTransfer(bAddress,
                                    epInfo[BTDSSP_EVENT_PIPE].epAddr,
                                    &length,
                                    hcibuf,
                                    pollInterval);

   /* Check for errors */
   if (!rcode || rcode == hrNAK) {
      /* Switch on event type */
      switch (hcibuf[0]) {
         case EV_COMMAND_COMPLETE:
            /* Check if command succeeded, set command complete flag */
            if (!hcibuf[5]) hci_set_flag(HCI_FLAG_CMD_COMPLETE);
            break;

         case EV_COMMAND_STATUS:
            if (hcibuf[2]) {
               // Serial1.println(F("HCI command failed:"));
               // Serial1.println(hcibuf[2], HEX);
            }
            break;

         case EV_INQUIRY_COMPLETE:
            if (inquiry_counter >= 5 && pairWithHIDDevice) {
               inquiry_counter    = 0;
               connectToHIDDevice = false;
               pairWithHIDDevice  = false;
               hci_state          = HCI_SCANNING_STATE;
               // Serial1.println(F("Couldn't find HID device."));
            }
            inquiry_counter++;
            break;

         case EV_INQUIRY_RESULT:
            /* Check that there is more than zero responses */
            if (hcibuf[2]) {
               // Serial1.print(F("Number of responses: "));
               // Serial1.println(hcibuf[2]);

               for (uint8_t i = 0; i < hcibuf[2]; i++) {
                  uint8_t offset = 8 * hcibuf[2] + 3 * i;
                  for (uint8_t j = 0; j < 3; j++)
                     classOfDevice[j] = hcibuf[j + 4 + offset];

                  // Serial1.print(F("Class of device: "));
                  // Serial1.print(classOfDevice[2], HEX);
                  // Serial1.print(F(" "));
                  // Serial1.print(classOfDevice[1], HEX);
                  // Serial1.print(F(" "));
                  // Serial1.println(classOfDevice[0], HEX);

                  /* Check if it is a gamepad see: 
                     http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html */
                  if (pairWithHIDDevice &&
                      (classOfDevice[2] == 0x00) &&
                      (classOfDevice[1] == 0x05) &&
                      (classOfDevice[0] == 0x08)) {
                     // Serial1.println(F("Xbox gamepad device class found."));

                     for (uint8_t k = 0; k < 6; k++)
                        disc_bdaddr[k] = hcibuf[k + 3 + 6 * i];

                     // Serial1.print(F("Device address: "));
                     // Serial1.print(disc_bdaddr[5], HEX);
                     // Serial1.print(F(":"));
                     // Serial1.print(disc_bdaddr[4], HEX);
                     // Serial1.print(F(":"));
                     // Serial1.print(disc_bdaddr[3], HEX);
                     // Serial1.print(F(":"));
                     // Serial1.print(disc_bdaddr[2], HEX);
                     // Serial1.print(F(":"));
                     // Serial1.print(disc_bdaddr[1], HEX);
                     // Serial1.print(F(":"));
                     // Serial1.println(disc_bdaddr[0], HEX);

                     hci_set_flag(HCI_FLAG_DEVICE_FOUND);
                     break;
                  }
               }
            }
            break;

         case EV_CONNECTION_COMPLETE:
            hci_set_flag(HCI_FLAG_CONNECT_EVENT);
            /* Check if connected OK */
            if (!hcibuf[2]) {
               // Serial1.println(F("Connection established."));
               /* Store the handle for the ACL connection */
               hci_handle = hcibuf[3] | ((hcibuf[4] & 0x0F) << 8);
               hci_set_flag(HCI_FLAG_CONNECT_COMPLETE);
            } else {
               // Serial1.print(F("Connection Failed: "));
               // Serial1.println(hcibuf[2], HEX);
            }
            break;

         case EV_DISCONNECTION_COMPLETE:
            /* Check if disconnected OK */
            if (!hcibuf[2]) {
               // Serial1.println(F("Disconnection complete."));
               hci_set_flag(HCI_FLAG_DISCONNECT_COMPLETE);
               hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE);
            }
            break;

         case EV_REMOTE_NAME_REQUEST_COMPLETE:
            /* Check if reading is OK */
            if (!hcibuf[2]) {
               for (uint8_t i = 0; i < min(sizeof(remote_name), sizeof(hcibuf) - 9); i++) {
                  remote_name[i] = hcibuf[9 + i];
                  /* End of string */
                  if (remote_name[i] == '\0') break;
               }
               // Serial1.print(F("Remote name: "));
               for (uint8_t i = 0; i < strlen(remote_name); i++) {
                  // Serial1.print(remote_name[i]);
               }
               // Serial1.println(F(""));
               hci_set_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
            }
            break;

         case EV_CONNECTION_REQUEST:
            for (uint8_t i = 0; i < 6; i++)
               disc_bdaddr[i] = hcibuf[i + 2];

            for (uint8_t i = 0; i < 3; i++)
               classOfDevice[i] = hcibuf[i + 8];
            // Serial1.println(F("Connection request."));
            // Serial1.print(F("Class of device: "));
            // Serial1.print(classOfDevice[2], HEX);
            // Serial1.print(F(" "));
            // Serial1.print(classOfDevice[1], HEX);
            // Serial1.print(F(" "));
            // Serial1.println(classOfDevice[0], HEX);

            /* Check if it is a mouse, keyboard, gamepad or joystick */
            if ((classOfDevice[1] & 0x05) &&
                (classOfDevice[0] & 0xCC)) {
               if (classOfDevice[0] & 0x80) {
                  // Serial1.println(F("Mouse is connecting."));
               }
               if (classOfDevice[0] & 0x40) {
                  // Serial1.println(F("Keyboard is connecting."));
               }
               if (classOfDevice[0] & 0x08) {
                  // Serial1.println(F("Gamepad is connecting."));
               }
               if (classOfDevice[0] & 0x04) {
                  // Serial1.println(F("Joystick is connecting."));
               }
               incomingHIDDevice = true;
            }
            hci_set_flag(HCI_FLAG_INCOMING_REQUEST);
            break;

         case EV_RETURN_LINK_KEYS:
            break;

         case EV_PIN_CODE_REQUEST:
            break;

         case EV_LINK_KEY_REQUEST:
            /* UseSimplePairing */
            // Serial1.println(F("Link key request."));
            // Serial1.print(F("Pair with HID device: "));
            // Serial1.println(pairWithHIDDevice, HEX);
            if ((!pairWithHIDDevice) || (incomingHIDDevice)) {
               for (uint8_t i = 0; i < 16; i++) {
                  link_key[i] = EEPROM.read(i + 6);
               }
               hci_link_key_request_reply();
            } else {
               hci_link_key_request_negative_reply();
               // Serial1.println(F("Start simple pairing."));
            }
            break;

         case EV_IO_CAPABILITY_REQUEST:
            /* UseSimplePairing */
            // Serial1.println(F("Received IO capability request."));
            hci_io_capability_request_reply();
            break;

         case EV_IO_CAPABILITY_RESPONSE:
            /* UseSimplePairing */
            break;

         case EV_USER_CONFIRMATION_REQUEST:
            // Serial1.print(F("Received user confirmation request: "));
            // Serial1.print(hcibuf[8], HEX);
            // Serial1.print(F(" "));
            // Serial1.print(hcibuf[9], HEX);
            // Serial1.print(F(" "));
            // Serial1.println(hcibuf[10], HEX);

            hci_user_confirmation_request_reply();
            break;

         case EV_SIMPLE_PAIRING_COMPLETE:
            /* UseSimplePairing */
            break;

         case EV_LINK_KEY_NOTIFICATION:
            /* UseSimplePairing */
            // Serial1.println(F("Received link key."));
            // Serial1.println(F("EEPROM write disc_bdaddr."));
            for (uint8_t i = 0; i < 6; i++) {
               EEPROM.write(i, disc_bdaddr[i]);
            }
            for (uint8_t i = 0; i < 16; i++) {
               link_key[i] = hcibuf[8 + i];
            }
            // Serial1.println(F("EEPROM write link_key."));
            for (uint8_t i = 0; i < 16; i++) {
               EEPROM.write(i + 6, link_key[i]);
            }
            break;

         case EV_AUTHENTICATION_COMPLETE:
            if (!hcibuf[2]) {
               // Serial1.println(F("Authentication complete."));
               /* Pairing successful */
               if (pairWithHIDDevice && !connectToHIDDevice) {
                  // Serial1.println(F("Pairing successful with HID device."));
                  /* Used to indicate to the BTHID service, that it should 
                     connect to this device */
                  connectToHIDDevice = true;
               }
               hci_set_connection_encryption(hci_handle);
               /* Clear these flags for a new connection */
               l2capConnectionClaimed  = false;
               sdpConnectionClaimed    = false;
               rfcommConnectionClaimed = false;
               hci_event_flag          = 0;
               hci_state               = HCI_DONE_STATE;
            } else {
               // Serial1.print(F("Pairing Failed: "));
               // Serial1.println(hcibuf[2], HEX);
               hci_disconnect(hci_handle);
               hci_state = HCI_DISCONNECT_STATE;
            }
            break;

         /* We will just ignore the following events */
         case EV_ENCRYPTION_CHANGE:
         case EV_MAX_SLOTS_CHANGE:
         case EV_NUMBER_OF_COMPLETED_PACKETS:
         case EV_ROLE_CHANGED:
         case EV_PAGE_SCAN_REPETITION_MODE_CHANGE:
         case EV_LOOPBACK_COMMAND:
         case EV_DATA_BUFFER_OVERFLOW:
         case EV_CHANGE_CONNECTION_LINK_KEY_COMPLETE:
         case EV_QOS_SETUP_COMPLETE:
         case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
            break;
            // default:
            //    if (hcibuf[0] != 0x00) {
            //       // Serial1.println(F("Unmanaged HCI Event."));
            //    }
            //    break;
      }
   } else {
      // Serial1.print(F("HCI event error: "));
      // Serial1.println(rcode, HEX);
   }
}

/* Poll Bluetooth and print result */
void XBOXONEBTSSP::HCI_task()
{
   switch (hci_state) {
      case HCI_INIT_STATE:
         hci_counter++;
         /* Wait until we have looped x times to clear any old events */
         if (hci_counter > hci_num_reset_loops) {
            hci_reset();
            hci_state   = HCI_RESET_STATE;
            hci_counter = 0;
         }
         break;

      case HCI_RESET_STATE:
         hci_counter++;
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            hci_counter = 0;
            // Serial1.println(F("HCI reset complete."));
            hci_write_class_of_device();
            hci_state = HCI_CLASS_OF_DEVICE_STATE;

         } else if (hci_counter > hci_num_reset_loops) {
            hci_num_reset_loops *= 10;
            if (hci_num_reset_loops > 2000)
               hci_num_reset_loops = 2000;
            // Serial1.println(F("No response to HCI reset."));
            hci_state   = HCI_INIT_STATE;
            hci_counter = 0;
         }
         break;

      case HCI_CLASS_OF_DEVICE_STATE:
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            // Serial1.println(F("Write class of device."));
            if (btdsspName != NULL) {
               hci_write_local_name(btdsspName);
               hci_state = HCI_WRITE_NAME_STATE;
            } else {
               hci_write_simple_pairing_mode();
               hci_state = HCI_WRITE_SIMPLE_PAIRING_STATE;
            }
         }
         break;

      case HCI_WRITE_NAME_STATE:
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            // Serial1.println(F("The name was set to:"));
            // Serial1.println(btdsspName);
            hci_write_simple_pairing_mode();
            hci_state = HCI_WRITE_SIMPLE_PAIRING_STATE;
         }
         break;

      /* Secure Simple Pairing */
      case HCI_WRITE_SIMPLE_PAIRING_STATE:
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            // Serial1.println(F("Simple pairing was enabled."));
            hci_set_event_mask();
            hci_state = HCI_SET_EVENT_MASK_STATE;
         }
         break;

      case HCI_SET_EVENT_MASK_STATE:
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            // Serial1.println(F("Event mask was set."));
            hci_state = HCI_CHECK_DEVICE_SERVICE;
         }
         break;

      case HCI_AUTHENTICATION_REQUESTED_STATE:
      case HCI_LINK_KEY_REQUEST_REPLY_STATE:
      case HCI_LINK_KEY_REQUEST_NEGATIVE_REPLY_STATE:
      case HCI_IO_CAPABILITY_REQUEST_REPLY_STATE:
      case HCI_USER_CONFIRMATION_REQUEST_REPLY_STATE:
      case HCI_SET_CONNECTION_ENCRYPTION_STATE:
         break;

      case HCI_CHECK_DEVICE_SERVICE:
         // Serial1.println(F("Check device service."));
         // Serial1.print(F("Pair with HID device: "));
         // Serial1.println(pairWithHIDDevice, HEX);
         if (pairWithHIDDevice) {
            // Serial1.println(F("Please enable discovery of your device."));
            hci_inquiry();
            hci_state = HCI_INQUIRY_STATE;
         } else {
            hci_state = HCI_SCANNING_STATE;
         }
         break;

      case HCI_INQUIRY_STATE:
         if (hci_check_flag(HCI_FLAG_DEVICE_FOUND)) {
            /* Stop inquiry */
            hci_inquiry_cancel();
            // Serial1.println(F("HID device found."));
            hci_remote_name_request();
            hci_state = HCI_REMOTE_NAME_STATE;
         }
         break;

      case HCI_REMOTE_NAME_STATE:
         if (hci_check_flag(HCI_FLAG_REMOTE_NAME_COMPLETE)) {
            if (strncmp((const char *)remote_name, GAMEPAD_REMOTE_NAME, sizeof(GAMEPAD_REMOTE_NAME) - 1) == 0) {
               if (pairWithHIDDevice) {
                  // Serial1.println("Connect to the new device.");
                  hci_state = HCI_CONNECT_DEVICE_STATE;
               } else {
                  // Serial1.println("Check for paired device address.");
                  hci_state = HCI_CHECK_DISC_BDADDR_STATE;
               }
            } else {
               // Serial1.println(F("Continue searching."));
               hci_state = HCI_CHECK_DEVICE_SERVICE;
            }
         }
         break;

      case HCI_CONNECT_DEVICE_STATE:
         if (hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
            // Serial1.println(F("Connecting to HID device."));
            hci_connect();
            hci_state = HCI_CONNECTED_DEVICE_STATE;
         }
         break;

      case HCI_CONNECTED_DEVICE_STATE:
         if (hci_check_flag(HCI_FLAG_CONNECT_EVENT)) {
            if (hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
               // Serial1.println(F("Connected to HID device."));
               hci_authentication_requested(hci_handle);
               hci_state = HCI_AUTHENTICATION_REQUESTED_STATE;
            } else {
               // Serial1.println(F("Trying to connect one more time."));
               hci_connect();
            }
         }
         break;

      case HCI_SCANNING_STATE:
         if (!connectToHIDDevice && !pairWithHIDDevice) {
            // Serial1.println(F("Scanning, wait for incoming connection request."));
            waitingForConnection = true;
            hci_write_scan_enable();
            hci_state = HCI_CONNECT_IN_STATE;
         }
         break;

      case HCI_CONNECT_IN_STATE:
         if (hci_check_flag(HCI_FLAG_INCOMING_REQUEST)) {
            waitingForConnection = false;
            // Serial1.println(F("Incoming connection request."));
            hci_remote_name_request();
            hci_state = HCI_REMOTE_NAME_STATE;
         }
         break;

      case HCI_CHECK_DISC_BDADDR_STATE:
         // Serial1.print(F("Incoming address: "));
         // Serial1.print(disc_bdaddr[5], HEX);
         // Serial1.print(F(":"));
         // Serial1.print(disc_bdaddr[4], HEX);
         // Serial1.print(F(":"));
         // Serial1.print(disc_bdaddr[3], HEX);
         // Serial1.print(F(":"));
         // Serial1.print(disc_bdaddr[2], HEX);
         // Serial1.print(F(":"));
         // Serial1.print(disc_bdaddr[1], HEX);
         // Serial1.print(F(":"));
         // Serial1.println(disc_bdaddr[0], HEX);

         /* Check if the device is already paired. */
         pairedDevice = true;
         for (uint8_t i = 0; i < 6; i++) {
            if (disc_bdaddr[i] != EEPROM.read(i)) {
               pairedDevice = false;
               break;
            }
         }
         if (!pairedDevice) {
            // Serial1.println(F("Device not paired."));
            hci_reject_connection();
            hci_state = HCI_SCANNING_STATE;
         } else {
            hci_accept_connection();
            hci_state = HCI_CONNECTED_STATE;
         }
         break;

      case HCI_CONNECTED_STATE:
         if (hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
            // Serial1.print(F("Connected to device."));
            /* Clear these flags for a new connection */
            l2capConnectionClaimed  = false;
            sdpConnectionClaimed    = false;
            rfcommConnectionClaimed = false;
            hci_event_flag          = 0;
            hci_state               = HCI_DONE_STATE;
         }
         break;

      case HCI_DONE_STATE:
         hci_counter++;
         /* Wait until we have looped 1000 times to make sure that the L2CAP 
            connection has been started */
         if (hci_counter > 1000) {
            hci_counter = 0;
            hci_state   = HCI_DISCONNECT_STATE;
         }
         break;

      case HCI_DISCONNECT_STATE:
         if (hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE)) {
            // Serial1.println(F("HCI disconnected from device."));
            disconnect();
            /* Clear all flags and reset all buffers */
            hci_event_flag = 0;
            memset(hcibuf, 0, BULK_MAXPKTSIZE);
            memset(l2capinbuf, 0, BULK_MAXPKTSIZE);
            connectToHIDDevice = false;
            incomingHIDDevice  = false;
            pairWithHIDDevice  = false;
            hci_state          = HCI_SCANNING_STATE;
            break;
         }
         break;
   }
}

void XBOXONEBTSSP::ACL_event_task()
{
   uint16_t length = BULK_MAXPKTSIZE;
   /* Input on endpoint 2 */
   uint8_t rcode = pUsb->inTransfer(bAddress,
                                    epInfo[BTDSSP_DATAIN_PIPE].epAddr,
                                    &length,
                                    l2capinbuf,
                                    pollInterval);

   /* Check for errors */
   if (!rcode) {
      /* Check if any data was read */
      if (length > 0) {
         for (uint8_t i = 0; i < BTDSSP_NUM_SERVICES; i++) {
            if (btService[i])
               btService[i]->ACLData(l2capinbuf);
         }
      }
   } else if (rcode != hrNAK) {
      // Serial1.print(F("ACL data in error: "));
      // Serial1.println(rcode, HEX);
   }

   for (uint8_t i = 0; i < BTDSSP_NUM_SERVICES; i++)
      if (btService[i])
         btService[i]->Run();
}

void XBOXONEBTSSP::HCI_Command(uint8_t *data, uint16_t nbytes)
{
   hci_clear_flag(HCI_FLAG_CMD_COMPLETE);
   pUsb->ctrlReq(bAddress,
                 epInfo[BTDSSP_CONTROL_PIPE].epAddr,
                 bmREQ_HCI_OUT,
                 0x00,
                 0x00,
                 0x00,
                 0x00,
                 nbytes,
                 nbytes,
                 data,
                 NULL);
}

void XBOXONEBTSSP::hci_reset()
{
   hci_event_flag = 0;         /* Clear all the flags */
   hcibuf[0]      = 0x03;      /* HCI OCF = 3 */
   hcibuf[1]      = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2]      = 0x00;

   HCI_Command(hcibuf, 3);
}

void XBOXONEBTSSP::hci_write_scan_enable()
{
   hci_clear_flag(HCI_FLAG_INCOMING_REQUEST);

   hcibuf[0] = 0x1A;      /* HCI OCF = 1A */
   hcibuf[1] = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2] = 0x01;      /* parameter length = 1 */
   if (btdsspName != NULL) {
      hcibuf[3] = 0x03;
      // Serial1.println(F("Inquiry scan enabled. Page scan enabled."));
   } else {
      hcibuf[3] = 0x02;
      // Serial1.println(F("Inquiry scan disabled. Page scan enabled."));
   }
   HCI_Command(hcibuf, 4);
}

void XBOXONEBTSSP::hci_write_scan_disable()
{
   hcibuf[0] = 0x1A;      /* HCI OCF = 1A */
   hcibuf[1] = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2] = 0x01;      /* parameter length = 1 */
   hcibuf[3] = 0x00;      /* Inquiry Scan disabled. Page Scan disabled. */

   HCI_Command(hcibuf, 4);
}

void XBOXONEBTSSP::hci_accept_connection()
{
   hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE);

   hcibuf[0] = 0x09;      /* HCI OCF = 0x09 HCI_Accept_Connection_Request */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x07;      /* parameter length 7 */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];
   /* Switch role to master */
   hcibuf[9] = 0x00;

   HCI_Command(hcibuf, 10);
}

void XBOXONEBTSSP::hci_reject_connection()
{
   hcibuf[0] = 0x0A;      /* HCI OCF = 0x0A HCI_Reject_Connection_Request */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x07;      /* parameter length 7 */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];
   hcibuf[9] = 0x0F;

   HCI_Command(hcibuf, 10);
}

void XBOXONEBTSSP::hci_remote_name_request()
{
   hci_clear_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);

   hcibuf[0] = 0x19;      /* HCI OCF = 19 */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x0A;      /* parameter length = 10 */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];
   hcibuf[9]  = 0x01; /*Page Scan Repetition Mode */
   hcibuf[10] = 0x00; /* Reserved */
   hcibuf[11] = 0x00; /* Clock offset - low byte */
   hcibuf[12] = 0x00; /* Clock offset - high byte */

   HCI_Command(hcibuf, 13);
}

void XBOXONEBTSSP::hci_write_local_name(const char *name)
{
   hcibuf[0] = 0x13;             /* HCI OCF = 13 */
   hcibuf[1] = 0x03 << 2;        /* HCI OGF = 3 */
   hcibuf[2] = strlen(name) + 1; /* parameter length = length of the string + end byte */
   uint8_t i;
   for (i = 0; i < strlen(name); i++)
      hcibuf[i + 3] = name[i];
   hcibuf[i + 3] = 0x00; /* End of string */

   HCI_Command(hcibuf, 4 + strlen(name));
}

void XBOXONEBTSSP::hci_set_event_mask()
{
   hcibuf[0]  = 0x01;      /* HCI OCF = 01 */
   hcibuf[1]  = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2]  = 0x08;
   hcibuf[3]  = 0xFF;
   hcibuf[4]  = 0xFF;
   hcibuf[5]  = 0xFF;
   hcibuf[6]  = 0xFF;
   hcibuf[7]  = 0xFF;
   hcibuf[8]  = 0x1F;
   hcibuf[9]  = 0xFF;
   hcibuf[10] = 0x00;

   HCI_Command(hcibuf, 11);
}

void XBOXONEBTSSP::hci_write_simple_pairing_mode()
{
   hcibuf[0] = 0x56;      /* HCI OCF = 56 */
   hcibuf[1] = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2] = 0x01;      /* parameter length = 1 */
   hcibuf[3] = 0x01;      /* enable = 1 */

   HCI_Command(hcibuf, 4);
}

void XBOXONEBTSSP::hci_authentication_requested(uint16_t handle)
{
   hcibuf[0] = 0x11;                            /* HCI OCF = 11 */
   hcibuf[1] = 0x01 << 2;                       /* HCI OGF = 1 */
   hcibuf[2] = 0x02;                            /* Parameter Length = 2 */
   hcibuf[3] = (uint8_t)(handle & 0xFF);        /* connection handle low byte */
   hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); /* connection handle high byte */

   HCI_Command(hcibuf, 5);
}

void XBOXONEBTSSP::hci_link_key_request_reply()
{
   hcibuf[0] = 0x0B;      /* HCI OCF = 0B */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x16;      /* parameter length 22 */
   /* 6 octet bdaddr */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];
   /* 16 octet link_key */
   for (uint8_t i = 0; i < 16; i++)
      hcibuf[i + 9] = link_key[i];

   HCI_Command(hcibuf, 25);
}

void XBOXONEBTSSP::hci_link_key_request_negative_reply()
{
   hcibuf[0] = 0x0C;      /* HCI OCF = 0C */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x06;      /* parameter length 6 */
   /* 6 octet bdaddr */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];

   HCI_Command(hcibuf, 9);
}

void XBOXONEBTSSP::hci_io_capability_request_reply()
{
   hcibuf[0] = 0x2B;      /* HCI OCF = 2B */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x09;
   /* 6 octet bdaddr */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];
   hcibuf[9]  = 0x03; /* NoInputNoOutput */
   hcibuf[10] = 0x00;
   hcibuf[11] = 0x00;

   HCI_Command(hcibuf, 12);
}

void XBOXONEBTSSP::hci_user_confirmation_request_reply()
{
   hcibuf[0] = 0x2C;      /* HCI OCF = 2C */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x06;      /* Parameter Length = 6 */
   /* 6 octet bdaddr */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = disc_bdaddr[i];

   HCI_Command(hcibuf, 9);
}

void XBOXONEBTSSP::hci_set_connection_encryption(uint16_t handle)
{
   hcibuf[0] = 0x13;                            /* HCI OCF = 13 */
   hcibuf[1] = 0x01 << 2;                       /* HCI OGF = 1 */
   hcibuf[2] = 0x03;                            /* Parameter Length = 9 */
   hcibuf[3] = (uint8_t)(handle & 0xFF);        /* Connection_Handle low byte */
   hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); /* Connection_Handle high byte */
   hcibuf[5] = 0x01;                            /* 0x00=OFF 0x01=ON */

   HCI_Command(hcibuf, 6);
}

void XBOXONEBTSSP::hci_inquiry()
{
   hci_clear_flag(HCI_FLAG_DEVICE_FOUND);

   hcibuf[0] = 0x01;
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x05;      /* Parameter Total Length = 5 */
   hcibuf[3] = 0x33;      /* General/unlimited inquiry access code (GIAC) 0x9E8B33 */
   hcibuf[4] = 0x8B;
   hcibuf[5] = 0x9E;
   hcibuf[6] = 0x30; /* Inquiry time 61.44 sec (maximum) */
   hcibuf[7] = 0x0A; /* 10 number of responses */

   HCI_Command(hcibuf, 8);
}

void XBOXONEBTSSP::hci_inquiry_cancel()
{
   hcibuf[0] = 0x02;
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x00;      /* Parameter Total Length = 0 */

   HCI_Command(hcibuf, 3);
}

void XBOXONEBTSSP::hci_connect()
{
   hci_connect(disc_bdaddr); /* Use last discovered device */
}

void XBOXONEBTSSP::hci_connect(uint8_t *bdaddr)
{
   hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE | HCI_FLAG_CONNECT_EVENT);

   hcibuf[0] = 0x05;      /* HCI_Create_Connection */
   hcibuf[1] = 0x01 << 2; /* HCI OGF = 1 */
   hcibuf[2] = 0x0D;      /* parameter Total Length = 13 */
   /* 6 octet bdaddr */
   for (uint8_t i = 0; i < 6; i++)
      hcibuf[i + 3] = bdaddr[i];
   hcibuf[9]  = 0x18; /* DM1 or DH1 may be used */
   hcibuf[10] = 0xCC; /* DM3, DH3, DM5, DH5 may be used */
   hcibuf[11] = 0x01; /* Page repetition mode R1 */
   hcibuf[12] = 0x00; /* Reserved */
   hcibuf[13] = 0x00; /* Clock offset */
   hcibuf[14] = 0x00; /* Invalid clock offset */
   hcibuf[15] = 0x00; /* Do not allow role switch */

   HCI_Command(hcibuf, 16);
}

void XBOXONEBTSSP::hci_disconnect(uint16_t handle)
{
   hci_clear_flag(HCI_FLAG_DISCONNECT_COMPLETE);

   hcibuf[0] = 0x06;                            /* HCI OCF = 6 */
   hcibuf[1] = 0x01 << 2;                       /* HCI OGF = 1 */
   hcibuf[2] = 0x03;                            /* parameter length = 3 */
   hcibuf[3] = (uint8_t)(handle & 0xFF);        /* connection handle low byte */
   hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); /* connection handle high byte */
   hcibuf[5] = 0x13;                            /* reason */

   HCI_Command(hcibuf, 6);
}

void XBOXONEBTSSP::hci_write_class_of_device()
{
   // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
   hcibuf[0] = 0x24;      /* HCI OCF = 24 */
   hcibuf[1] = 0x03 << 2; /* HCI OGF = 3 */
   hcibuf[2] = 0x03;      /* parameter length = 3 */
   hcibuf[3] = 0x0C;      /* Smartphone */
   hcibuf[4] = 0x02;      /* Phone */
   hcibuf[5] = 0x00;

   HCI_Command(hcibuf, 6);
}

/* HCI ACL Data Packet 

    buf[0]          buf[1]          buf[2]          buf[3]
    0       4       8       12      16              24            31 MSB
   .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
   |      HCI Handle       |PB |BC |       Data Total Length       |   HCI ACL Data Packet
   .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.

    buf[4]          buf[5]          buf[6]          buf[7]
    0               8               16                            31 MSB
   .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
   |            Length             |          Channel ID           |   Basic L2CAP header
   .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.

    buf[8]          buf[9]          buf[10]         buf[11]
    0               8               16                            31 MSB
   .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
   |     Code      |  Identifier   |            Length             |   Control frame (C-frame)
   .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.   (signaling packet format)
 */

void XBOXONEBTSSP::L2CAP_Command(uint16_t handle,
                                 uint8_t *data,
                                 uint8_t nbytes,
                                 uint8_t channelLow,
                                 uint8_t channelHigh)
{
   uint8_t buf[8 + nbytes];
   buf[0] = (uint8_t)(handle & 0xff); /* HCI handle with PB,BC flag */
   buf[1] = (uint8_t)(((handle >> 8) & 0x0f) | 0x20);
   buf[2] = (uint8_t)((4 + nbytes) & 0xff); /* HCI ACL total data length */
   buf[3] = (uint8_t)((4 + nbytes) >> 8);
   buf[4] = (uint8_t)(nbytes & 0xff); /* L2CAP header: Length */
   buf[5] = (uint8_t)(nbytes >> 8);
   buf[6] = channelLow;
   buf[7] = channelHigh;

   /* L2CAP C-frame */
   for (uint16_t i = 0; i < nbytes; i++)
      buf[8 + i] = data[i];

   uint8_t rcode = pUsb->outTransfer(bAddress,
                                     epInfo[BTDSSP_DATAOUT_PIPE].epAddr,
                                     (8 + nbytes),
                                     buf);
   if (rcode) {
      /* This small delay prevents it from overflowing if it fails */
      delay(100);
      // Serial1.print(F("Error sending L2CAP message: "));
      // Serial1.println(rcode, HEX);
      // Serial1.print(F("Channel ID: "));
      // Serial1.print(channelHigh, HEX);
      // Serial1.println(channelLow, HEX);
   }
}

void XBOXONEBTSSP::l2cap_connection_request(uint16_t handle,
                                            uint8_t rxid,
                                            uint8_t *scid,
                                            uint16_t psm)
{
   l2capoutbuf[0] = L2CAP_CMD_CONNECTION_REQUEST; /* Code */
   l2capoutbuf[1] = rxid;                         /* Identifier */
   l2capoutbuf[2] = 0x04;                         /* Length */
   l2capoutbuf[3] = 0x00;
   l2capoutbuf[4] = (uint8_t)(psm & 0xff); /* PSM */
   l2capoutbuf[5] = (uint8_t)(psm >> 8);
   l2capoutbuf[6] = scid[0]; /* Source CID */
   l2capoutbuf[7] = scid[1];

   L2CAP_Command(handle, l2capoutbuf, 8);
}

void XBOXONEBTSSP::l2cap_connection_response(uint16_t handle,
                                             uint8_t rxid,
                                             uint8_t *dcid,
                                             uint8_t *scid,
                                             uint8_t result)
{
   l2capoutbuf[0]  = L2CAP_CMD_CONNECTION_RESPONSE; /* Code */
   l2capoutbuf[1]  = rxid;                          /* Identifier */
   l2capoutbuf[2]  = 0x08;                          // Length
   l2capoutbuf[3]  = 0x00;
   l2capoutbuf[4]  = dcid[0]; /* Destination CID */
   l2capoutbuf[5]  = dcid[1];
   l2capoutbuf[6]  = scid[0]; /* Source CID */
   l2capoutbuf[7]  = scid[1];
   l2capoutbuf[8]  = result; /* Result: Pending or Success */
   l2capoutbuf[9]  = 0x00;
   l2capoutbuf[10] = 0x00; /* No further information */
   l2capoutbuf[11] = 0x00;

   L2CAP_Command(handle, l2capoutbuf, 12);
}

void XBOXONEBTSSP::l2cap_config_request(uint16_t handle,
                                        uint8_t rxid,
                                        uint8_t *dcid)
{
   l2capoutbuf[0]  = L2CAP_CMD_CONFIG_REQUEST; /* Code */
   l2capoutbuf[1]  = rxid;                     /* Identifier */
   l2capoutbuf[2]  = 0x08;                     /* Length */
   l2capoutbuf[3]  = 0x00;
   l2capoutbuf[4]  = dcid[0]; /* Destination CID */
   l2capoutbuf[5]  = dcid[1];
   l2capoutbuf[6]  = 0x00; /* Flags */
   l2capoutbuf[7]  = 0x00;
   l2capoutbuf[8]  = 0x01; /* Config Opt: type = MTU Hint */
   l2capoutbuf[9]  = 0x02; /* Config Opt: length */
   l2capoutbuf[10] = 0xFF; /* MTU */
   l2capoutbuf[11] = 0xFF;

   L2CAP_Command(handle, l2capoutbuf, 12);
}

void XBOXONEBTSSP::l2cap_config_response(uint16_t handle,
                                         uint8_t rxid,
                                         uint8_t *scid)
{
   l2capoutbuf[0]  = L2CAP_CMD_CONFIG_RESPONSE; /* Code */
   l2capoutbuf[1]  = rxid;                      /* Identifier */
   l2capoutbuf[2]  = 0x0A;                      /* Length */
   l2capoutbuf[3]  = 0x00;
   l2capoutbuf[4]  = scid[0]; /* Source CID */
   l2capoutbuf[5]  = scid[1];
   l2capoutbuf[6]  = 0x00; /* Flag */
   l2capoutbuf[7]  = 0x00;
   l2capoutbuf[8]  = 0x00; /* Result */
   l2capoutbuf[9]  = 0x00;
   l2capoutbuf[10] = 0x01; /* Config */
   l2capoutbuf[11] = 0x02;
   l2capoutbuf[12] = 0xA0;
   l2capoutbuf[13] = 0x02;

   L2CAP_Command(handle, l2capoutbuf, 14);
}

void XBOXONEBTSSP::l2cap_disconnection_request(uint16_t handle,
                                               uint8_t rxid,
                                               uint8_t *dcid,
                                               uint8_t *scid)
{
   l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_REQUEST; /* Code */
   l2capoutbuf[1] = rxid;                         /* Identifier */
   l2capoutbuf[2] = 0x04;                         /* Length */
   l2capoutbuf[3] = 0x00;
   l2capoutbuf[4] = dcid[0];
   l2capoutbuf[5] = dcid[1];
   l2capoutbuf[6] = scid[0];
   l2capoutbuf[7] = scid[1];

   L2CAP_Command(handle, l2capoutbuf, 8);
}

void XBOXONEBTSSP::l2cap_disconnection_response(uint16_t handle,
                                                uint8_t rxid,
                                                uint8_t *dcid,
                                                uint8_t *scid)
{
   l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE; /* Code */
   l2capoutbuf[1] = rxid;                          /* Identifier */
   l2capoutbuf[2] = 0x04;                          /* Length */
   l2capoutbuf[3] = 0x00;
   l2capoutbuf[4] = dcid[0];
   l2capoutbuf[5] = dcid[1];
   l2capoutbuf[6] = scid[0];
   l2capoutbuf[7] = scid[1];

   L2CAP_Command(handle, l2capoutbuf, 8);
}

void XBOXONEBTSSP::l2cap_information_response(uint16_t handle,
                                              uint8_t rxid,
                                              uint8_t infoTypeLow,
                                              uint8_t infoTypeHigh)
{
   l2capoutbuf[0]  = L2CAP_CMD_INFORMATION_RESPONSE; /* Code */
   l2capoutbuf[1]  = rxid;                           /* Identifier */
   l2capoutbuf[2]  = 0x08;                           /* Length */
   l2capoutbuf[3]  = 0x00;
   l2capoutbuf[4]  = infoTypeLow;
   l2capoutbuf[5]  = infoTypeHigh;
   l2capoutbuf[6]  = 0x00; /* Result = success */
   l2capoutbuf[7]  = 0x00; /* Result = success */
   l2capoutbuf[8]  = 0x00;
   l2capoutbuf[9]  = 0x00;
   l2capoutbuf[10] = 0x00;
   l2capoutbuf[11] = 0x00;

   L2CAP_Command(handle, l2capoutbuf, 12);
}
