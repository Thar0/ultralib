#include "PR/os_internal.h"
#include "PR/os_host.h"
#include "PR/rdb.h"
#include "PR/rcp.h"

#include "macros.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

// https://usb.org/sites/default/files/vendor_ids051920_0.pdf lists iQue Ltd. as vendor 5415/0x1527
#define BBRDB_IDVENDOR  0x1527
#define BBRDB_IDPRODUCT 0xBBDB

#ifdef _DEBUG
u8* __osRdb_DbgRead_Buf;
u32 __osRdb_DbgRead_BufSize;
u32 __osRdb_DbgRead_Ct;
u32 __osRdb_DbgRead_Err;
#endif

extern s32 __osRdb_Usb_Active;
extern void (*__osRdb_Usb_StartSend)(void*, int);

#define BULK_BUF_SIZE 0x5000

static boolean rdbs_bulk_enabled;
static s32 rdbs_bulk_outlen;
static s32 rdbs_bulk_outpending;
static uchar in_bulk_buf0[BULK_BUF_SIZE] BBALIGNED(16);
static uchar in_bulk_buf1[BULK_BUF_SIZE] BBALIGNED(16);
static uchar out_bulk_buf[BULK_BUF_SIZE] BBALIGNED(16);
#ifdef _DEBUG
static _usb_device_handle rdb_device_handle;
#endif
static char DevDescShort[8];
#ifdef _DEBUG
static u8 rdb_buffer[0x1560];
#endif

static uint_8 DevDesc[18] = {
    /* bLength              */  sizeof(DevDesc),
    /* bDescriptorType      */  USB_DESC_DEVICE,
    /* bcdUSB               */  USB_DESC_16(0x0110), // 1.1
    /* bDeviceClass         */  0,
    /* bDeviceSubClass      */  0,
    /* bDeviceProtocol      */  0,
    /* bMaxPacketSize0      */  USB_MAX_PACKET_SIZE_FULLSPEED,
    /* idVendor             */  USB_DESC_16(BBRDB_IDVENDOR),
    /* idProduct            */  USB_DESC_16(BBRDB_IDPRODUCT),
    /* bcdDevice            */  USB_DESC_16(1),
    /* iManufacturer        */  1,
    /* iProduct             */  2,
    /* iSerialNumber        */  0,
    /* bNumConfigurations   */  1,
};

static uint_8 ConfigDesc[32] = {
// Configuration Descriptor
    /* bLength              */  9,
    /* bDescriptorType      */  USB_DESC_CONFIG,
    /* wTotalLength         */  USB_DESC_16(sizeof(ConfigDesc)),
    /* bNumInterfaces       */  1,
    /* bConfigurationValue  */  1,
    /* iConfiguration       */  0,
    /* bmAttributes         */  0x80,
    /* bMaxPower            */  500/2, // 500mA
// Interface Descriptor
    /* bLength              */  9,
    /* bDescriptorType      */  USB_DESC_INTERFACE,
    /* bInterfaceNumber     */  0,
    /* bAlternateSetting    */  0,
    /* bNumEndpoints        */  2,
    /* bInterfaceClass      */  0xFF,
    /* bInterfaceSubClass   */  0xFF,
    /* bInterfaceProtocol   */  0,
    /* iInterface           */  0,
// Endpoint Descriptor
    /* bLength          */  7,
    /* bDescriptorType  */  USB_DESC_ENDPOINT,
    /* bEndpointAddress */  USB_ENDPOINT_IN | 2,
    /* bmAttributes     */  USB_ENDPOINT_XFER_TYPE_BULK,
    /* wMaxPacketSize   */  USB_DESC_16(USB_MAX_PACKET_SIZE_FULLSPEED),
    /* bInterval        */  0,
// Endpoint Descriptor
    /* bLength          */  7,
    /* bDescriptorType  */  USB_DESC_ENDPOINT,
    /* bEndpointAddress */  USB_ENDPOINT_OUT | 2,
    /* bmAttributes     */  USB_ENDPOINT_XFER_TYPE_BULK,
    /* wMaxPacketSize   */  USB_DESC_16(USB_MAX_PACKET_SIZE_FULLSPEED),
    /* bInterval        */  0,
};

// The first string descriptor contains a list of language identifiers that follow the
// Windows Language Code Identifier (LCID) scheme.
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/
static uint_8 USB_STR_0[4]  = {
    sizeof(USB_STR_0), USB_DESC_STRING,
    USB_DESC_16(0x0409), // en-us
};

static uint_8 USB_STR_1[20] = {
    sizeof(USB_STR_1), USB_DESC_STRING,
    USB_CHAR('i'),
    USB_CHAR('Q'),
    USB_CHAR('u'),
    USB_CHAR('e'),
    USB_CHAR(' '),
    USB_CHAR('L'),
    USB_CHAR('t'),
    USB_CHAR('d'),
    USB_CHAR('.'),
};

static uint_8 USB_STR_2[24] = {
    sizeof(USB_STR_2), USB_DESC_STRING,
    USB_CHAR('i'),
    USB_CHAR('Q'),
    USB_CHAR('u'),
    USB_CHAR('e'),
    USB_CHAR(' '),
    USB_CHAR('P'),
    USB_CHAR('l'),
    USB_CHAR('a'),
    USB_CHAR('y'),
    USB_CHAR('e'),
    USB_CHAR('r'),
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

static const uint_8_ptr USB_STRING_DESC[4] = {
    USB_STR_0,
    USB_STR_1,
    USB_STR_2,
    USB_STR_n,
};

static int cur_inbuf = 0;

struct {
    /* 0x0000 */ uchar* buf;
    /* 0x0004 */ int len;
} inbuf[2] = {
    { in_bulk_buf0, 0 },
    { in_bulk_buf1, 0 },
};

#ifdef _DEBUG
extern rdbPacket* __osRdb_IP6_Data;
extern u32 __osRdb_IP6_Size;
extern u32 __osRdb_IP6_Ct;
extern u32 __osRdb_IP6_CurWrite;
extern u32 __osRdb_IP6_CurSend;
extern u32 __osRdb_IP6_Empty;
extern u32 __osRdb_Read_Data_Buf;
extern u32 __osRdb_Read_Data_Ct;


void send_mesg(u32 a0);

u32 __osRdb_Usb_GetAvailSendBytes(void) {
    return BULK_BUF_SIZE - rdbs_bulk_outlen;
}

static void osRdb_Usb_StartSend(void* datap, int len) {
    int dlen;
    rdbPacket* pktp;

    __usb_splhigh();

    dlen = MIN(len, BULK_BUF_SIZE - rdbs_bulk_outlen);

    if (dlen > 0) {
        if ((rdbs_bulk_outlen & 3) || ((u32)datap & 3)) {
            bcopy(datap, &out_bulk_buf[rdbs_bulk_outlen], dlen);
        } else {
            pktp = (rdbPacket*)&out_bulk_buf[rdbs_bulk_outlen];
            *pktp = *(rdbPacket*)datap;
        }

        rdbs_bulk_outlen += dlen;

        len -= dlen;
        if (len > 0) {
            goto write;
        }
    }

    while (__osRdb_IP6_Ct != 0) {
        if ((BULK_BUF_SIZE - rdbs_bulk_outlen) < 4) {
            break;
        }
        if (rdbs_bulk_outlen & 3) {
            bcopy(&__osRdb_IP6_Data[__osRdb_IP6_CurSend], &out_bulk_buf[rdbs_bulk_outlen], 4);
        } else {
            pktp = (rdbPacket*)&out_bulk_buf[rdbs_bulk_outlen];
            *pktp = __osRdb_IP6_Data[__osRdb_IP6_CurSend];
        }
        rdbs_bulk_outlen += 4;
        __osRdb_IP6_Ct--;
        __osRdb_IP6_CurSend++;
        if (__osRdb_IP6_CurSend >= __osRdb_IP6_Size) {
            __osRdb_IP6_CurSend = 0;
        }
    }

write:
    if (rdbs_bulk_outpending == 0) {
        rdbs_service_bulk_ep(rdb_device_handle, FALSE, USB_SEND, NULL, 0);
    }
    if (__osRdb_IP6_Ct == 0) {
        __osRdb_IP6_Empty = 1;
    }
    __usb_splx();
}
#endif

static void rdb_ip6_input(_usb_device_handle handle) {
#ifdef _DEBUG
    int cmd;
    int dlen;
    unsigned char* dp; // $s1
    unsigned char* buf;
    int other = ~cur_inbuf & 1;
    int total = 0; // $s2
    int len;
    int excess;
    int save_data_ct = __osRdb_Read_Data_Ct;
    int buflen;
    int i;

    buf = inbuf[other].buf;
    len = inbuf[other].len;

    while (len > 0) {
        cmd = buf[total++];
        len--;

        dlen = cmd & 3;
        cmd >>= 2;

        switch (cmd) {
            case RDB_TYPE_HtoG_DATA:
                dp = (void*)__osRdb_Read_Data_Buf;

                if (dlen > __osRdb_Read_Data_Ct) {
                    excess = dlen - __osRdb_Read_Data_Ct;
                    dlen = __osRdb_Read_Data_Ct;
                } else {
                    excess = 0;
                }

                for (i = 0; i < dlen; i++) {
                    *dp++ = buf[total++];
                }

                __osRdb_Read_Data_Ct -= dlen;
                __osRdb_Read_Data_Buf = dp;
                len -= dlen + excess;
                total += excess;
                break;

            case RDB_TYPE_HtoG_DATAB:
                dp = (void*)__osRdb_Read_Data_Buf;

                dlen = buf[total++];
                len--;

                if (dlen > __osRdb_Read_Data_Ct) {
                    excess = dlen - __osRdb_Read_Data_Ct;
                    dlen = __osRdb_Read_Data_Ct;
                } else {
                    excess = 0;
                }

                bcopy(&buf[total], dp, dlen);

                dp += dlen;
                __osRdb_Read_Data_Ct -= dlen;
                __osRdb_Read_Data_Buf = dp;
                len -= dlen + excess;
                total += dlen + excess;
                break;

            case RDB_TYPE_HtoG_DATA_DONE:
            case RDB_TYPE_HtoG_SYNC_DONE:
                send_mesg(OS_EVENT_RDB_DATA_DONE * 8);
                total += dlen;
                len -= dlen;
                break;

            case RDB_TYPE_HtoG_DEBUG:
                dp = (void*)__osRdb_DbgRead_Buf;

                if (dlen == 0 && len > 0) {
                    dlen = buf[total++];
                    len--;
                }

                if (dp != NULL) {
                    if (dlen > len) {
                        __osRdb_DbgRead_Err++;
                        dlen = len;
                    }
                    buflen = __osRdb_DbgRead_BufSize - __osRdb_DbgRead_Ct;
                    excess = dlen - buflen;
                    if (dlen > buflen) {
                        __osRdb_DbgRead_Err++;
                        dlen = buflen;
                    } else {
                        excess = 0;
                    }

                    bcopy(&buf[total], dp, dlen);

                    dp += dlen;
                    __osRdb_DbgRead_Ct += dlen;
                    __osRdb_DbgRead_Buf = dp;
                    len -= dlen + excess;
                    total += dlen + excess;
                } else {
                    total += dlen;
                    len -= dlen;
                }
                break;

            case RDB_TYPE_HtoG_DEBUG_DONE:
                send_mesg(OS_EVENT_RDB_DBG_DONE * 8);
                total += dlen;
                len -= dlen;
                break;

            default:
                len -= dlen;
                total += dlen;
                break;
        }
    }

    if (__osRdb_Read_Data_Ct == 0 && save_data_ct != 0) {
        send_mesg(OS_EVENT_RDB_READ_DONE * 8);
    }
#endif
}

static void rdbs_service_bulk_ep(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    __usb_splhigh();

    if (direction == USB_RECV) {
        // Finalize previous buffer
        inbuf[cur_inbuf].len = length;
        // Swap to next buffer and submit transfer to fill it
        cur_inbuf = ~cur_inbuf & 1; // ^= 1
        _usb_device_recv_data(handle, 2, inbuf[cur_inbuf].buf, BULK_BUF_SIZE);
        // Process previous buffer
        rdb_ip6_input(handle);
    } else if (direction == USB_SEND) {
        rdbs_bulk_outlen -= length;
        if (rdbs_bulk_outlen < 0) {
            rdbs_bulk_outlen = 0;
        }

        if (rdbs_bulk_outlen > 0) {
            int outlen;

            if (length != 0) {
                bcopy(&out_bulk_buf[length], out_bulk_buf, rdbs_bulk_outlen);
            }
            outlen = rdbs_bulk_outlen;
            if (outlen % USB_MAX_PACKET_SIZE_FULLSPEED == 0) {
                outlen -= 4;
            }
            // Submit transfer
            _usb_device_send_data(handle, 2, out_bulk_buf, outlen);
        }
        rdbs_bulk_outpending = rdbs_bulk_outlen;
#ifdef _DEBUG
        if (rdbs_bulk_outpending == 0 && length != 0 && __osRdb_IP6_Ct != 0) {
            osRdb_Usb_StartSend(NULL, 0);
        }
#endif
    }

    __usb_splx();
}

// GET_DESCRIPTOR setup response
static void rdbs_ch9GetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 value;
    int len;

    if (!setup) {
        // Status phase, nothing to do
        return;
    }

    // Obtain wValue and wLength
    value = BSWAP16(setup_ptr->VALUE) & 0xFF00;
    len = BSWAP16(setup_ptr->LENGTH);

    // Upper 8 bits of wValue is the descriptor requested
    switch (value) {
        case USB_GET_DESC_DEVICE:
            // Get device descriptor
            if (len <= (int)sizeof(DevDescShort)) {
                // If the request is <= 8 bytes, submit only the first 8 bytes for transfer
                bcopy(DevDesc, DevDescShort, sizeof(DevDescShort));
                DevDescShort[0] = sizeof(DevDescShort);
                _usb_device_send_data(handle, 0, (uchar_ptr)DevDescShort, sizeof(DevDescShort));
            } else {
                // Otherwise submit as much as was requested, not exceeding the descriptor size
                _usb_device_send_data(handle, 0, DevDesc, MIN((unsigned)len, sizeof(DevDesc)));
            }
            break;
        case USB_GET_DESC_CONFIG:
            // Get configuration descriptor
            // Submit as much as was requested, not exceeding the descriptor size
            _usb_device_send_data(handle, 0, ConfigDesc, MIN((unsigned)len, sizeof(ConfigDesc)));
            break;
        case USB_GET_DESC_STRING:
            // Get a descriptor string
            // Lower 8 bits of wValue is the string index
            value = BSWAP16(setup_ptr->VALUE) & 0xFF;

            if (value >= ARRLEN(USB_STRING_DESC) - 1) {
                // Bad index, send "BAD STRING Index"
                _usb_device_send_data(handle, 0, USB_STRING_DESC[ARRLEN(USB_STRING_DESC) - 1], MIN(len, USB_STRING_DESC[ARRLEN(USB_STRING_DESC) - 1][0]));
            } else {
                // Good index, send requested descriptor string
                _usb_device_send_data(handle, 0, USB_STRING_DESC[value], MIN(len, USB_STRING_DESC[value][0]));
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

static void rdbs_init_data_eps(_usb_device_handle handle) {
    uint_8 ep_type = USB_BULK_PIPE;

    // Initialize endpoint 2 as a bidirectional bulk endpoint and register a service to handle completions on them
    _usb_device_init_endpoint(handle, 2, USB_MAX_PACKET_SIZE_FULLSPEED, USB_RECV, ep_type, FALSE);
    _usb_device_init_endpoint(handle, 2, USB_MAX_PACKET_SIZE_FULLSPEED, USB_SEND, ep_type, FALSE);
    _usb_device_register_service(handle, USB_SERVICE_EP(2), rdbs_service_bulk_ep);
    if (_usb_device_get_transfer_status(handle, 2, USB_RECV) == USB_STATUS_IDLE) {
        // Start listening for data on endpoint 2
        cur_inbuf = 0;
        _usb_device_recv_data(handle, 2, inbuf->buf, BULK_BUF_SIZE);
    }
    rdbs_bulk_outlen = 0;
    rdbs_bulk_outpending = 0;
    rdbs_bulk_enabled = TRUE;
}

static void rdbs_reset_data_eps(_usb_device_handle handle) {
    _usb_device_cancel_transfer(handle, 2, USB_RECV);
    _usb_device_cancel_transfer(handle, 2, USB_SEND);
    rdbs_bulk_outlen = 0;
    rdbs_bulk_outpending = 0;
    rdbs_bulk_enabled = FALSE;
    osResetRdb();
#ifdef _DEBUG
    send_mesg(OS_EVENT_RDB_UNK32 * 8);
    send_mesg(OS_EVENT_RDB_UNK33 * 8);
#endif
}

static void rdbs_stall_data_eps(_usb_device_handle handle, uint_8 ep_num, boolean stall) {
    if (ep_num == 2 && !stall) {
        rdbs_reset_data_eps(handle);
        rdbs_init_data_eps(handle);
    }
}

void rdbs_ch9Vendor(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
}

static void rdbs_reset_ep0(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    if (rdbs_bulk_enabled) {
        rdbs_reset_data_eps(handle);
    }
    _usb_device_init_endpoint(handle, 0, USB_MAX_PACKET_SIZE_FULLSPEED, USB_RECV, USB_CONTROL_PIPE, FALSE);
    _usb_device_init_endpoint(handle, 0, USB_MAX_PACKET_SIZE_FULLSPEED, USB_SEND, USB_CONTROL_PIPE, FALSE);
}

void rdbs_query(OSBbUsbInfo* ip) {
    ip->ua_type = OS_USB_TYPE_DEVICE;
    ip->ua_state = (dev_global_struct.dev_state == 2) ? OS_USB_STATE_ATTACHED : OS_USB_STATE_NULL;
    ip->ua_class = OS_USB_DEV_GENERIC;
    ip->ua_subclass = 0;
    ip->ua_protocol = 0;
    ip->ua_vendor = BBRDB_IDVENDOR;
    ip->ua_product = BBRDB_IDPRODUCT;
    ip->ua_cfg = 0;
    ip->ua_ep = 1;
    ip->ua_mode = OS_USB_MODE_RW;
    ip->ua_blksize = USB_MAX_PACKET_SIZE_FULLSPEED;
    ip->ua_speed = OS_USB_FULL_SPEED;
    ip->ua_mfr_str = (u8*)"iQue Ltd";
    ip->ua_prod_str = (u8*)"iQue Player (disabled)";
    ip->ua_driver_name = (u8*)"";
    ip->ua_support = TRUE;
}

void rdbs_device_init(_usb_device_handle handle) {
    dev_global_struct.num_ifcs = ConfigDesc[4];
#ifdef _DEBUG
    if (__osRdb_IP6_Data == NULL) {
        osInitRdb(rdb_buffer, sizeof(rdb_buffer));
    }
    rdb_device_handle = handle;
    __osRdb_Usb_Active = 1;
    __osRdb_Usb_StartSend = osRdb_Usb_StartSend;
#endif
}

usbdevfuncs rdbs_dev_funcs = {
    rdbs_reset_ep0,
    rdbs_ch9GetDescription,
    rdbs_ch9Vendor,
    rdbs_init_data_eps,
    rdbs_query,
    rdbs_stall_data_eps,
};
