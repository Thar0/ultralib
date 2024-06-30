#include "PR/os_internal.h"
#include "PR/os_host.h"
#include "PR/rdb.h"
#include "PR/rcp.h"

#include "macros.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

// https://usb.org/sites/default/files/vendor_ids051920_0.pdf lists TransDimension-NH LLC as vendor 1443/0x05A3
#define ECHO_DEVICE_IDVENDOR    0x05A3
#define ECHO_DEVICE_IDPRODUCT   0x0001

extern uchar USB_IF_ALT[4];
extern uint_8 if_status;

typedef struct buffer_struct {
    /* 0x0000 */ uint_8 buf[USB_MAX_PACKET_SIZE_FULLSPEED];
    /* 0x0040 */ uint_32 len;
} BUFFER_STRUCT, *BUFFER_STRUCT_PTR; // Size = 0x44

typedef struct echo_global_struct {
    /* 0x0000 */ volatile boolean ECHO_ENABLED;
    /* 0x0004 */ int_16 PIPE_INT_IN;
    /* 0x0006 */ int_16 PIPE_INT_OUT;
    /* 0x0008 */ int_16 PIPE_BULK_IN;
    /* 0x000A */ int_16 PIPE_BULK_OUT;
    /* 0x000C */ uchar INTDATAOUT[300];
    /* 0x0138 */ uchar INTDATAIN[300];
    /* 0x0264 */ uchar BULKDATAOUT[300];
    /* 0x0390 */ uchar BULKDATAIN[300];
    /* 0x04BC */ uchar RX_TEST_BUFFER[20];
} ECHO_GLOBAL_STRUCT, *ECHO_GLOBAL_STRUCT_PTR; // Size = 0x4D0

static uint_8 DevDesc[18] = {
    /* bLength              */  sizeof(DevDesc),
    /* bDescriptorType      */  USB_DESC_DEVICE,
    /* bcdUSB               */  USB_DESC_16(0x0110), // 1.1
    /* bDeviceClass         */  0,
    /* bDeviceSubClass      */  0,
    /* bDeviceProtocol      */  0,
    /* bMaxPacketSize       */  USB_MAX_PACKET_SIZE_FULLSPEED,
    /* idVendor             */  USB_DESC_16(ECHO_DEVICE_IDVENDOR),
    /* idProduct            */  USB_DESC_16(ECHO_DEVICE_IDPRODUCT),
    /* bcdDevice            */  USB_DESC_16(2),
    /* iManufacturer        */  1,
    /* iProduct             */  2,
    /* iSerialNumber        */  0,
    /* bNumConfigurations   */  1,
};

static uint_8 ConfigDesc[35] = {
// Configuration Descriptor
    /* bLength              */  9,
    /* bDescriptorType      */  USB_DESC_CONFIG,
    /* wTotalLength         */  USB_DESC_16(sizeof(ConfigDesc)),
    /* bNumInterfaces       */  1,
    /* bConfigurationValue  */  1,
    /* iConfiguration       */  4,
    /* bmAttributes         */  0x80 | USB_ENDPOINT_ATTR_SELF_POWERED,
    /* bMaxPower            */  0/2, // 0mA (self-powered)
// Interface Descriptor
    /* bLength              */  9,
    /* bDescriptorType      */  USB_DESC_INTERFACE,
    /* bInterfaceNumber     */  0,
    /* bAlternateSetting    */  0,
    /* bNumEndpoints        */  2,
    /* bInterfaceClass      */  8,
    /* bInterfaceSubClass   */  6,
    /* bInterfaceProtocol   */  0x50,
    /* iInterface           */  0,
// Endpoint Descriptor
    /* bLength          */  7,
    /* bDescriptorType  */  USB_DESC_ENDPOINT,
    /* bEndpointAddress */  USB_ENDPOINT_IN | 1,
    /* bmAttributes     */  USB_ENDPOINT_XFER_TYPE_BULK,
    /* wMaxPacketSize   */  USB_DESC_16(USB_MAX_PACKET_SIZE_FULLSPEED),
    /* bInterval        */  0,
// Endpoint Descriptor
    /* bLength          */  7,
    /* bDescriptorType  */  USB_DESC_ENDPOINT,
    /* bEndpointAddress */  USB_ENDPOINT_OUT | 1,
    /* bmAttributes     */  USB_ENDPOINT_XFER_TYPE_BULK,
    /* wMaxPacketSize   */  USB_DESC_16(USB_MAX_PACKET_SIZE_FULLSPEED),
    /* bInterval        */  0,
// OTG Descriptor
    /* bLength          */  3,
    /* bDescriptorType  */  USB_DESC_OTG,
    /* bmAttributes     */  USB_DESC_OTG_SRP | USB_DESC_OTG_HNP,
};

// The first string descriptor contains a list of language identifiers that follow the
// Windows Language Code Identifier (LCID) scheme.
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/
static uint_8 USB_STR_0[4] = {
    sizeof(USB_STR_0), USB_DESC_STRING,
    USB_DESC_16(0x0409), // en-us
};

static uint_8 USB_STR_1[56] = {
    sizeof(USB_STR_1), USB_DESC_STRING,
    USB_CHAR('B'),
    USB_CHAR('r'),
    USB_CHAR('o'),
    USB_CHAR('a'),
    USB_CHAR('d'),
    USB_CHAR('O'),
    USB_CHAR('n'),
    USB_CHAR(' '),
    USB_CHAR('C'),
    USB_CHAR('o'),
    USB_CHAR('m'),
    USB_CHAR('m'),
    USB_CHAR('u'),
    USB_CHAR('n'),
    USB_CHAR('i'),
    USB_CHAR('c'),
    USB_CHAR('a'),
    USB_CHAR('t'),
    USB_CHAR('i'),
    USB_CHAR('o'),
    USB_CHAR('n'),
    USB_CHAR('s'),
    USB_CHAR(' '),
    USB_CHAR('C'),
    USB_CHAR('o'),
    USB_CHAR('r'),
    USB_CHAR('p'),
};

static uint_8 USB_STR_2[36] = {
    sizeof(USB_STR_2), USB_DESC_STRING,
    USB_CHAR('E'),
    USB_CHAR('c'),
    USB_CHAR('h'),
    USB_CHAR('o'),
    USB_CHAR(' '),
    USB_CHAR('S'),
    USB_CHAR('l'),
    USB_CHAR('a'),
    USB_CHAR('v'),
    USB_CHAR('e'),
    USB_CHAR(' '),
    USB_CHAR('D'),
    USB_CHAR('e'),
    USB_CHAR('v'),
    USB_CHAR('i'),
    USB_CHAR('c'),
    USB_CHAR('e'),
};

static uint_8 USB_STR_3[10] = {
    sizeof(USB_STR_3), USB_DESC_STRING,
    USB_CHAR('B'),
    USB_CHAR('E'),
    USB_CHAR('T'),
    USB_CHAR('A'),
};

static uint_8 USB_STR_4[8] = {
    sizeof(USB_STR_4), USB_DESC_STRING,
    USB_CHAR('#'),
    USB_CHAR('0'),
    USB_CHAR('2'),
};

static uint_8 USB_STR_5[8] = {
    sizeof(USB_STR_5), USB_DESC_STRING,
    USB_CHAR('_'),
    USB_CHAR('A'),
    USB_CHAR('1'),
};

static uint_8 USB_STR_6[32] = {
    sizeof(USB_STR_6), USB_DESC_STRING,
    USB_CHAR('R'),
    USB_CHAR('u'),
    USB_CHAR('f'),
    USB_CHAR('u'),
    USB_CHAR('s'),
    USB_CHAR(' '),
    USB_CHAR('T'),
    USB_CHAR(' '),
    USB_CHAR('F'),
    USB_CHAR('i'),
    USB_CHAR('r'),
    USB_CHAR('e'),
    USB_CHAR('f'),
    USB_CHAR('l'),
    USB_CHAR('y'),
};

static uint_8 USB_STR_n[34] = {
    sizeof(USB_STR_n), USB_DESC_STRING,
    USB_CHAR('B'),
    USB_CHAR('A'),
    USB_CHAR('D'),
    USB_CHAR(' '),
    USB_CHAR('S'),
    USB_CHAR('T'),
    USB_CHAR('R'),
    USB_CHAR('I'),
    USB_CHAR('N'),
    USB_CHAR('G'),
    USB_CHAR(' '),
    USB_CHAR('I'),
    USB_CHAR('n'),
    USB_CHAR('d'),
    USB_CHAR('e'),
    USB_CHAR('x'),
};

static const uint_8_ptr USB_STRING_DESC[8] = {
    USB_STR_0,
    USB_STR_1,
    USB_STR_2,
    USB_STR_3,
    USB_STR_4,
    USB_STR_5,
    USB_STR_6,
    USB_STR_n,
};

BUFFER_STRUCT ep1_buf BBALIGNED(16);
BUFFER_STRUCT ep2_buf BBALIGNED(16);
ECHO_GLOBAL_STRUCT echo_app_struct;
uint_16 usb_status;
uint_8 endpoint;
uint_8 if_status;
uint_8 data_to_send;
uint_8 setup_packet[8];
SETUP_STRUCT local_setup_packet;

void echo_ch9GetStatus(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 index;

    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    switch (setup_ptr->REQUESTTYPE) {
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_DEVICE|USB_REQ_TYPE_STANDARD):
            // Retrieve and send the desired status
            _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_status);
            _usb_device_send_data(handle, 0, (uchar_ptr)&usb_status, sizeof(usb_status));
            break;
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_INTERFACE|USB_REQ_TYPE_STANDARD):
            index = BSWAP16(setup_ptr->INDEX) & 0xFF;
            index = MIN(ARRLEN(USB_IF_ALT) - 1, index);
            if_status = USB_IF_ALT[index];
            _usb_device_send_data(handle, 0, &if_status, sizeof(if_status));
            break;
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_ENDPOINT|USB_REQ_TYPE_STANDARD):
            endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
            _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
            _usb_device_send_data(handle, 0, (uchar_ptr)&usb_status, sizeof(usb_status));
            break;
        default:
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            return;
    }
    _usb_device_recv_data(handle, 0, NULL, 0);
}

void echo_ch9ClearFeature(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    static uint_8 endpoint;
    uint_16 usb_status;
    uint_16 value;

    _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_status);

    if (usb_status > USB_SELF_POWERED) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }
    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    value = BSWAP16(setup_ptr->VALUE);

    switch (setup_ptr->REQUESTTYPE) {
        case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_DEVICE|USB_REQ_DIR_OUT):
            if (value == 1) { // unset remote wakeup
                _usb_device_get_status(handle, USB_STATUS_DEVICE, &usb_status);
                usb_status &= ~USB_REMOTE_WAKEUP;
                _usb_device_set_status(handle, USB_STATUS_DEVICE, usb_status);
            }
            break;
        case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_ENDPOINT|USB_REQ_DIR_OUT):
            if (value != 0) { // invalid
                _usb_device_stall_endpoint(handle, 0, USB_RECV);
                return;
            }
            // unstall endpoint
            endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
            _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
            _usb_device_set_status(handle, USB_STATUS_ENDPOINT | endpoint, FALSE);
            break;
        default:
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            break;
    }

    // Status phase
    _usb_device_send_data(handle, 0, NULL, 0);
}

void echo_ch9SetFeature(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 usb_status;
    uint_16 value = BSWAP16(setup_ptr->VALUE);
    uint_8 endpoint;

    if (setup) {
        switch (setup_ptr->REQUESTTYPE) {
            case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_DEVICE|USB_REQ_DIR_OUT):
                if (value == 1) { // set remote wakeup
                    _usb_device_get_status(handle, USB_STATUS_DEVICE, &usb_status);
                    usb_status |= USB_REMOTE_WAKEUP;
                    _usb_device_set_status(handle, USB_STATUS_DEVICE, usb_status);
                }
                break;
            case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_ENDPOINT|USB_REQ_DIR_OUT):
                if (value != 0) { // invalid
                    _usb_device_stall_endpoint(handle, 0, USB_RECV);
                    return;
                }
                // stall endpoint
                endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
                _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
                _usb_device_set_status(handle, USB_STATUS_ENDPOINT | endpoint, TRUE);
                break;
            default:
                _usb_device_stall_endpoint(handle, 0, USB_RECV);
                return;
        }

        // Status phase
        _usb_device_send_data(handle, 0, NULL, 0);
        return;
    }

    // Status phase complete, register suspend
    if (value & 3) {
        _usb_device_register_service(handle, USB_SERVICE_SUSPEND, dev_bus_suspend);
        echo_app_struct.ECHO_ENABLED = FALSE;
    }
}

// GET_DESCRIPTOR setup response
static void echo_ch9GetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 value;

    if (!setup) {
        // Status phase, nothing to do
        return;
    }

    // Obtain wValue
    value = BSWAP16(setup_ptr->VALUE) & 0xFF00;

    // Upper 8 bits of wValue is the descriptor requested
    switch (value) {
        case USB_GET_DESC_DEVICE:
            // Send device descriptor, not exceeding the descriptor size
            _usb_device_send_data(handle, 0, DevDesc, MIN((unsigned)BSWAP16(setup_ptr->LENGTH), sizeof(DevDesc)));
            break;
        case USB_GET_DESC_CONFIG:
            // Send config descriptor, not exceeding the descriptor size
            _usb_device_send_data(handle, 0, ConfigDesc, MIN((unsigned)BSWAP16(setup_ptr->LENGTH), sizeof(ConfigDesc)));
            break;
        case USB_GET_DESC_STRING:
            // Get a descriptor string
            // Lower 8 bits of wValue is the string index
            value = BSWAP16(setup_ptr->VALUE) & 0xFF;

            if (value >= ARRLEN(USB_STRING_DESC) - 1) {
                // Bad index, send "BAD STRING Index"
                _usb_device_send_data(handle, 0, USB_STRING_DESC[ARRLEN(USB_STRING_DESC) - 1], MIN(BSWAP16(setup_ptr->LENGTH), USB_STRING_DESC[ARRLEN(USB_STRING_DESC) - 1][0]));
            } else {
                // Good index, send requested descriptor string
                _usb_device_send_data(handle, 0, USB_STRING_DESC[value], MIN(BSWAP16(setup_ptr->LENGTH), USB_STRING_DESC[value][0]));
            }
            break;
        default:
            // Unknown or unhandled descriptor request, stall
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            return;
    }

    // Prepare to receive status phase
    _usb_device_recv_data(handle, 0, NULL, 0);

    // ?
    dev_global_struct.dev_state = 2;
}

void echo_ch9SetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    _usb_device_stall_endpoint(handle, 0, USB_RECV);
}

void echo_ch9GetConfig(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    if (setup) {
        uint_16 current_config;

        _usb_device_get_status(handle, USB_STATUS_CURRENT_CONFIG, &current_config);
        data_to_send = current_config;
        _usb_device_send_data(handle, 0, &data_to_send, sizeof(data_to_send));

        // Status phase
        _usb_device_recv_data(handle, 0, NULL, 0);
    }
}

void echo_ch9SetConfig(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 usb_state;

    _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_state);

    if (usb_state < USB_REMOTE_WAKEUP) {
        uint_16 value;

        if (!setup) {
            // Status phase complete, nothing to do
            return;
        }

        value = BSWAP16(setup_ptr->VALUE) & 0xFF;

        if (value == 0) {
            _usb_device_set_status(handle, USB_STATUS_CURRENT_CONFIG, 0);
            _usb_device_set_status(handle, USB_STATUS_DEVICE_STATE, USB_SELF_POWERED);
        } else {
            _usb_device_get_status(handle, USB_STATUS_CURRENT_CONFIG, &usb_state);
            if (usb_state != value) {
                _usb_device_set_status(handle, USB_STATUS_CURRENT_CONFIG, value);
            }
            _usb_device_set_status(handle, USB_STATUS_DEVICE_STATE, USB_STATE_0);
        }

        // Status phase
        _usb_device_send_data(handle, 0, NULL, 0);
    } else {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
    }
}

void echo_ch9Vendor(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
}

static void echo_reset_ep0(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    if (echo_app_struct.ECHO_ENABLED) {
        _usb_device_cancel_transfer(handle, 1, USB_RECV);
        _usb_device_cancel_transfer(handle, 1, USB_SEND);
        _usb_device_cancel_transfer(handle, 2, USB_RECV);
        _usb_device_cancel_transfer(handle, 2, USB_SEND);
        echo_app_struct.ECHO_ENABLED = FALSE;
    }
    if (!echo_app_struct.ECHO_ENABLED) {
        _usb_device_init_endpoint(handle, 0, USB_MAX_PACKET_SIZE_FULLSPEED, USB_RECV, USB_CONTROL_PIPE, FALSE);
        _usb_device_init_endpoint(handle, 0, USB_MAX_PACKET_SIZE_FULLSPEED, USB_SEND, USB_CONTROL_PIPE, FALSE);
    }
}

static void echo_service_data_ep1(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length);

static void echo_init_data_eps(_usb_device_handle handle) {
    int ep_type = USB_BULK_PIPE;

    // Initialize endpoint 1 as a bidirectional bulk endpoint and register a service to handle completions on them
    _usb_device_init_endpoint(handle, 1, USB_MAX_PACKET_SIZE_FULLSPEED, USB_RECV, ep_type, FALSE);
    _usb_device_init_endpoint(handle, 1, USB_MAX_PACKET_SIZE_FULLSPEED, USB_SEND, ep_type, FALSE);
    _usb_device_register_service(handle, USB_SERVICE_EP(1), echo_service_data_ep1);
    if (_usb_device_get_transfer_status(handle, 1, USB_RECV) == USB_STATUS_IDLE) {
        // Start listening for data on endpoint 1
        _usb_device_recv_data(handle, 1, &ep1_buf.buf[ep1_buf.len], sizeof(ep1_buf.buf));
    }
    echo_app_struct.ECHO_ENABLED = TRUE;
}

static void echo_service_data_ep1(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    if (direction == USB_RECV) {
        ep1_buf.len += length;
        if (ep1_buf.len == USB_MAX_PACKET_SIZE_FULLSPEED) {
            _usb_device_cancel_transfer(handle, 1, USB_SEND);
            _usb_device_send_data(handle, 1, ep1_buf.buf, ep1_buf.len);
        }
    } else if (direction == USB_SEND) {
        ep1_buf.len = 0;
        bzero(ep1_buf.buf, USB_MAX_PACKET_SIZE_FULLSPEED);
        osInvalDCache(&ep1_buf.buf[ep1_buf.len], sizeof(ep1_buf.buf));
        _usb_device_cancel_transfer(handle, 1, USB_RECV);
        _usb_device_recv_data(handle, 1, &ep1_buf.buf[ep1_buf.len], sizeof(ep1_buf.buf));
    }
}

static void echo_service_data_ep2(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
}

void echo_query(OSBbUsbInfo* ip) {
    ip->ua_type = OS_USB_TYPE_DEVICE;
    ip->ua_state = (dev_global_struct.dev_state == 2) ? OS_USB_STATE_ATTACHED : OS_USB_STATE_NULL;
    ip->ua_class = OS_USB_DEV_GENERIC;
    ip->ua_subclass = 0;
    ip->ua_protocol = 0;
    ip->ua_vendor = ECHO_DEVICE_IDVENDOR;
    ip->ua_product = ECHO_DEVICE_IDPRODUCT;
    ip->ua_cfg = 0;
    ip->ua_ep = 1;
    ip->ua_mode = OS_USB_MODE_RW;
    ip->ua_blksize = USB_MAX_PACKET_SIZE_FULLSPEED;
    ip->ua_speed = OS_USB_FULL_SPEED;
    ip->ua_mfr_str = (u8*)"BroadOn Communications";
    ip->ua_prod_str = (u8*)"Echo Slave Device";
    ip->ua_driver_name = (u8*)"";
    ip->ua_support = TRUE;
}

void echo_device_init(_usb_device_handle handle) {
    ep1_buf.len = 0;
    ep2_buf.len = 0;
    dev_global_struct.num_ifcs = ConfigDesc[4];
}

usbdevfuncs echo_dev_funcs = {
    echo_reset_ep0,
    echo_ch9GetDescription,
    echo_ch9Vendor,
    echo_init_data_eps,
    echo_query,
    NULL,
};
