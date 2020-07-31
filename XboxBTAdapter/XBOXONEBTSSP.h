#ifndef _XBOXONEBTSSP_H_
#define _XBOXONEBTSSP_H_

#include <EEPROM.h>
#include <Usb.h>

/* Bluetooth dongle data taken from descriptors */
#define BULK_MAXPKTSIZE 64 // Max size for ACL data

// Used in control endpoint header for HCI Commands
#define bmREQ_HCI_OUT USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_DEVICE

/* Bluetooth HCI states for hci_task() */
#define HCI_INIT_STATE            0
#define HCI_RESET_STATE           1
#define HCI_CLASS_OF_DEVICE_STATE 2
#define HCI_WRITE_NAME_STATE      3
#define HCI_CHECK_DEVICE_SERVICE  4

#define HCI_INQUIRY_STATE          5
#define HCI_CONNECT_DEVICE_STATE   6
#define HCI_CONNECTED_DEVICE_STATE 7

#define HCI_SCANNING_STATE          8
#define HCI_CONNECT_IN_STATE        9
#define HCI_REMOTE_NAME_STATE       10
#define HCI_CHECK_DISC_BDADDR_STATE 11
#define HCI_CONNECTED_STATE         12
#define HCI_DONE_STATE              13
#define HCI_DISCONNECT_STATE        14
#define HCI_DISCONNECTED_STATE      15

#define HCI_SET_EVENT_MASK_STATE                  16
#define HCI_WRITE_SIMPLE_PAIRING_STATE            17
#define HCI_AUTHENTICATION_REQUESTED_STATE        18
#define HCI_LINK_KEY_REQUEST_REPLY_STATE          19
#define HCI_LINK_KEY_REQUEST_NEGATIVE_REPLY_STATE 20
#define HCI_IO_CAPABILITY_REQUEST_REPLY_STATE     21
#define HCI_USER_CONFIRMATION_REQUEST_REPLY_STATE 22
#define HCI_SET_CONNECTION_ENCRYPTION_STATE       23

/* HCI event flags*/
#define HCI_FLAG_CMD_COMPLETE         (1UL << 0)
#define HCI_FLAG_CONNECT_COMPLETE     (1UL << 1)
#define HCI_FLAG_DISCONNECT_COMPLETE  (1UL << 2)
#define HCI_FLAG_REMOTE_NAME_COMPLETE (1UL << 3)
#define HCI_FLAG_INCOMING_REQUEST     (1UL << 4)
#define HCI_FLAG_DEVICE_FOUND         (1UL << 5)
#define HCI_FLAG_CONNECT_EVENT        (1UL << 6)

/* Macros for HCI event flag tests */
#define hci_check_flag(flag) (hci_event_flag & (flag))
#define hci_set_flag(flag)   (hci_event_flag |= (flag))
#define hci_clear_flag(flag) (hci_event_flag &= ~(flag))

/* HCI Events managed */
#define EV_INQUIRY_COMPLETE                         0x01
#define EV_INQUIRY_RESULT                           0x02
#define EV_CONNECTION_COMPLETE                      0x03
#define EV_CONNECTION_REQUEST                       0x04
#define EV_DISCONNECTION_COMPLETE                   0x05
#define EV_AUTHENTICATION_COMPLETE                  0x06
#define EV_REMOTE_NAME_REQUEST_COMPLETE             0x07
#define EV_ENCRYPTION_CHANGE                        0x08
#define EV_CHANGE_CONNECTION_LINK_KEY_COMPLETE      0x09
#define EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE 0x0C
#define EV_QOS_SETUP_COMPLETE                       0x0D
#define EV_COMMAND_COMPLETE                         0x0E
#define EV_COMMAND_STATUS                           0x0F
#define EV_ROLE_CHANGED                             0x12
#define EV_NUMBER_OF_COMPLETED_PACKETS              0x13
#define EV_RETURN_LINK_KEYS                         0x15
#define EV_PIN_CODE_REQUEST                         0x16
#define EV_LINK_KEY_REQUEST                         0x17
#define EV_LINK_KEY_NOTIFICATION                    0x18
#define EV_LOOPBACK_COMMAND                         0x19
#define EV_DATA_BUFFER_OVERFLOW                     0x1A
#define EV_MAX_SLOTS_CHANGE                         0x1B
#define EV_PAGE_SCAN_REPETITION_MODE_CHANGE         0x20
#define EV_IO_CAPABILITY_REQUEST                    0x31
#define EV_IO_CAPABILITY_RESPONSE                   0x32
#define EV_USER_CONFIRMATION_REQUEST                0x33
#define EV_SIMPLE_PAIRING_COMPLETE                  0x36

/* Bluetooth states for the different Bluetooth drivers */
#define L2CAP_WAIT 0
#define L2CAP_DONE 1

/* Used for HID Control channel */
#define L2CAP_CONTROL_CONNECT_REQUEST 2
#define L2CAP_CONTROL_CONFIG_REQUEST  3
#define L2CAP_CONTROL_SUCCESS         4
#define L2CAP_CONTROL_DISCONNECT      5

/* Used for HID Interrupt channel */
#define L2CAP_INTERRUPT_SETUP           6
#define L2CAP_INTERRUPT_CONNECT_REQUEST 7
#define L2CAP_INTERRUPT_CONFIG_REQUEST  8
#define L2CAP_INTERRUPT_DISCONNECT      9

/* Used for SDP channel */
#define L2CAP_SDP_WAIT    10
#define L2CAP_SDP_SUCCESS 11

/* Used for RFCOMM channel */
#define L2CAP_RFCOMM_WAIT    12
#define L2CAP_RFCOMM_SUCCESS 13

/* Used for both SDP and RFCOMM channel */
#define L2CAP_DISCONNECT_RESPONSE 14

/* L2CAP event flags for HID Control channel */
#define L2CAP_FLAG_CONNECTION_CONTROL_REQUEST  (1UL << 0)
#define L2CAP_FLAG_CONFIG_CONTROL_SUCCESS      (1UL << 1)
#define L2CAP_FLAG_CONTROL_CONNECTED           (1UL << 2)
#define L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE (1UL << 3)

/* L2CAP event flags for HID Interrupt channel */
#define L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST  (1UL << 4)
#define L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS      (1UL << 5)
#define L2CAP_FLAG_INTERRUPT_CONNECTED           (1UL << 6)
#define L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE (1UL << 7)

/* L2CAP event flags for SDP channel */
#define L2CAP_FLAG_CONNECTION_SDP_REQUEST (1UL << 8)
#define L2CAP_FLAG_CONFIG_SDP_SUCCESS     (1UL << 9)
#define L2CAP_FLAG_DISCONNECT_SDP_REQUEST (1UL << 10)

/* L2CAP event flags for RFCOMM channel */
#define L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST (1UL << 11)
#define L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS     (1UL << 12)
#define L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST (1UL << 13)

#define L2CAP_FLAG_DISCONNECT_RESPONSE (1UL << 14)

/* Macros for L2CAP event flag tests */
#define l2cap_check_flag(flag) (l2cap_event_flag & (flag))
#define l2cap_set_flag(flag)   (l2cap_event_flag |= (flag))
#define l2cap_clear_flag(flag) (l2cap_event_flag &= ~(flag))

/* L2CAP signaling commands */
#define L2CAP_CMD_COMMAND_REJECT       0x01
#define L2CAP_CMD_CONNECTION_REQUEST   0x02
#define L2CAP_CMD_CONNECTION_RESPONSE  0x03
#define L2CAP_CMD_CONFIG_REQUEST       0x04
#define L2CAP_CMD_CONFIG_RESPONSE      0x05
#define L2CAP_CMD_DISCONNECT_REQUEST   0x06
#define L2CAP_CMD_DISCONNECT_RESPONSE  0x07
#define L2CAP_CMD_INFORMATION_REQUEST  0x0A
#define L2CAP_CMD_INFORMATION_RESPONSE 0x0B

// Used For Connection Response - Remember to Include High Byte
#define PENDING    0x01
#define SUCCESSFUL 0x00

/* Bluetooth L2CAP PSM - see http://www.bluetooth.org/Technical/AssignedNumbers/logical_link.htm */
#define SDP_PSM      0x01 /* Service Discovery Protocol PSM Value */
#define RFCOMM_PSM   0x03 /* RFCOMM PSM Value */
#define HID_CTRL_PSM 0x11 /* HID_Control PSM Value */
#define HID_INTR_PSM 0x13 /* HID_Interrupt PSM Value */

// Used to determine if it is a Bluetooth dongle
#define WI_SUBCLASS_RF 0x01 /* RF Controller */
#define WI_PROTOCOL_BT 0x01 /* Bluetooth Programming Interface */

#define BTDSSP_MAX_ENDPOINTS 4
#define BTDSSP_NUM_SERVICES  4 /* Max number of Bluetooth services - if you need more than 4 simply increase this number */

#define PAIR 1

class BluetoothService;

class XBOXONEBTSSP : public USBDeviceConfig, public UsbConfigXtracter
{
public:
   XBOXONEBTSSP(USB *p);

   uint8_t ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed);
   uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
   uint8_t Release();
   uint8_t Poll();
   void HCI_Command(uint8_t *data, uint16_t nbytes);
   void hci_reset();
   void hci_write_local_name(const char *name);
   void hci_write_simple_pairing_mode();
   void hci_set_event_mask();
   void hci_write_scan_enable();
   void hci_write_scan_disable();
   void hci_remote_name_request();
   void hci_accept_connection();
   void hci_reject_connection();
   void hci_disconnect(uint16_t handle);
   void hci_link_key_request_negative_reply();
   void hci_link_key_request_reply();
   void hci_user_confirmation_request_reply();
   void hci_set_connection_encryption(uint16_t handle);
   void hci_authentication_requested(uint16_t handle);
   void hci_inquiry();
   void hci_inquiry_cancel();
   void hci_connect();
   void hci_io_capability_request_reply();
   void hci_connect(uint8_t *bdaddr);
   void hci_write_class_of_device();
   void L2CAP_Command(uint16_t handle, uint8_t *data, uint8_t nbytes, uint8_t channelLow = 0x01, uint8_t channelHigh = 0x00);
   void l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t *scid, uint16_t psm);
   void l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t *dcid, uint8_t *scid, uint8_t result);
   void l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t *dcid);
   void l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t *scid);
   void l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t *dcid, uint8_t *scid);
   void l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t *dcid, uint8_t *scid);
   void l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh);

   void pairWithHID()
   {
      waitingForConnection = false;
      pairWithHIDDevice    = true;
      hci_state            = HCI_CHECK_DEVICE_SERVICE;
   };
   uint8_t readPollInterval()
   {
      return pollInterval;
   };
   virtual uint8_t GetAddress()
   {
      return bAddress;
   };
   virtual bool isReady()
   {
      return bPollEnable;
   };
   virtual bool DEVCLASSOK(uint8_t klass)
   {
      return (klass == USB_CLASS_WIRELESS_CTRL);
   };
   void EndpointXtract(uint8_t conf,
                       uint8_t iface,
                       uint8_t alt,
                       uint8_t proto,
                       const USB_ENDPOINT_DESCRIPTOR *ep);
   void disconnect();
   int8_t registerBluetoothService(BluetoothService *pService)
   {
      for (uint8_t i = 0; i < BTDSSP_NUM_SERVICES; i++) {
         if (!btService[i]) {
            btService[i] = pService;
            return i; /* Return ID */
         }
      }
      return -1; /* Error registering BluetoothService */
   };

   bool waitingForConnection;
   bool l2capConnectionClaimed;
   bool sdpConnectionClaimed;
   bool rfcommConnectionClaimed;
   const char *btdsspName;
   uint16_t hci_handle;
   uint8_t disc_bdaddr[6];
   uint8_t link_key[16];
   uint8_t stored_bdaddr[6];
   char remote_name[30];
   bool connectToHIDDevice;
   bool incomingHIDDevice;
   bool pairWithHIDDevice;
   bool pairedDevice;

protected:
   void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR *ep_ptr);
   USB *pUsb;
   uint8_t bAddress;
   EpInfo epInfo[BTDSSP_MAX_ENDPOINTS];
   uint8_t bConfNum;
   uint8_t bNumEP;
   uint32_t qNextPollTime;
   static const uint8_t BTDSSP_CONTROL_PIPE;
   static const uint8_t BTDSSP_EVENT_PIPE;
   static const uint8_t BTDSSP_DATAIN_PIPE;
   static const uint8_t BTDSSP_DATAOUT_PIPE;

private:
   void Initialize();
   void HCI_event_task();
   void HCI_task();
   void ACL_event_task();
   BluetoothService *btService[BTDSSP_NUM_SERVICES];
   uint16_t PID;
   uint16_t VID;
   uint8_t pollInterval;
   bool bPollEnable;
   bool checkRemoteName;
   bool differentDevice;
   uint8_t classOfDevice[3];
   uint8_t hci_state;
   uint16_t hci_counter;
   uint16_t hci_num_reset_loops;
   uint16_t hci_event_flag;
   uint8_t inquiry_counter;
   uint8_t hcibuf[BULK_MAXPKTSIZE];
   uint8_t l2capinbuf[BULK_MAXPKTSIZE];
   uint8_t l2capoutbuf[14];
};

class BluetoothService
{
public:
   BluetoothService(XBOXONEBTSSP *p) : pBtdssp(p)
   {
      if (pBtdssp)
         pBtdssp->registerBluetoothService(this);
   };
   void attachOnInit(void (*funcOnInit)(void))
   {
      pFuncOnInit = funcOnInit;
   };
   virtual void ACLData(uint8_t *ACLData) = 0;
   virtual void Run()                     = 0;
   virtual void Reset()                   = 0;
   virtual void disconnect()              = 0;

protected:
   bool checkHciHandle(uint8_t *buf, uint16_t handle)
   {
      return (buf[0] == (handle & 0xFF)) && (buf[1] == ((handle >> 8) | 0x20));
   }
   virtual void onInit() = 0;
   void (*pFuncOnInit)(void);
   XBOXONEBTSSP *pBtdssp;
   uint16_t hci_handle;
   uint32_t l2cap_event_flag;
   uint8_t identifier;
};

#endif