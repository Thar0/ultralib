#ifndef USB_H_
#define USB_H_

#include <PR/os_usb.h>

typedef char* char_ptr;
typedef unsigned char uchar;
typedef unsigned char* uchar_ptr;
typedef signed char int_8;
typedef signed char* int_8_ptr;
typedef unsigned char uint_8;
typedef unsigned char* uint_8_ptr;
typedef short int int_16;
typedef short int* int_16_ptr;
typedef short unsigned int uint_16;
typedef short unsigned int* uint_16_ptr;
typedef long int int_32;
typedef long int* int_32_ptr;
typedef long unsigned int uint_32;
typedef long unsigned int* uint_32_ptr;
typedef long unsigned int boolean;
typedef void* pointer;
typedef float ieee_single;
typedef double ieee_double;

typedef pointer _usb_host_handle;
typedef pointer _usb_device_handle;

typedef struct _usb_ctlr_state_s {
    /* 0x0000 */ int ucs_mode;
    /* 0x0004 */ int ucs_mask;
} _usb_ctlr_state_t; // size = 0x8

#define UCS_MODE_NONE   0
#define UCS_MODE_HOST   1
#define UCS_MODE_DEVICE 2

#define UCS_MASK_NONE   (0)
#define UCS_MASK_HOST   (1 << 0)
#define UCS_MASK_DEVICE (1 << 1)

typedef struct __OSBbUsbMesg_s {
    /* 0x0000 */ u8 um_type;
    union {
        struct {
            /* 0x0008 */ OSBbUsbInfo* umq_info;
            /* 0x000C */ s32 umq_ninfo;
        } umq;
        struct {
            /* 0x0008 */ OSBbUsbInfo* umh_info;
            /* 0x000C */ OSBbUsbHandle umh_handle;
        } umh;
        struct {
            /* 0x0008 */ OSBbUsbHandle umrw_handle;
            /* 0x000C */ u8* umrw_buffer;
            /* 0x0010 */ s32 umrw_len;
            /* 0x0018 */ u64 umrw_offset;
        } umrw;
    } u;
    /* 0x0020 */ s32 um_ret;
    /* 0x0024 */ OSMesgQueue* um_rq;
} __OSBbUsbMesg; // size = 0x28

typedef struct _usb_ext_handle_s {
    /* 0x0000 */ s32 uh_which;
    /* 0x0004 */ s32 uh_blksize;
    /* 0x0008 */ OSMesgQueue* uh_mq;
    /* 0x000C */ _usb_host_handle uh_host_handle;
    /* 0x0010 */ __OSBbUsbMesg* uh_wr_msg;
    /* 0x0014 */ u8* uh_wr_buffer;
    /* 0x0018 */ u64 uh_wr_offset;
    /* 0x0020 */ s32 uh_wr_len;
    /* 0x0024 */ __OSBbUsbMesg* uh_rd_msg;
    /* 0x0028 */ u8* uh_rd_buffer;
    /* 0x0030 */ u64 uh_rd_offset;
    /* 0x0038 */ s32 uh_rd_len;
} _usb_ext_handle; // size = 0x40

typedef struct setup_struct {
    /* 0x0000 */ uint_8 REQUESTTYPE; // bmRequestType
    /* 0x0001 */ uint_8 REQUEST;     // bRequest
    /* 0x0002 */ uint_16 VALUE;      // wValue
    /* 0x0004 */ uint_16 INDEX;      // wIndex
    /* 0x0006 */ uint_16 LENGTH;     // wLength
} SETUP_STRUCT, *SETUP_STRUCT_PTR; // size = 0x8

typedef struct {
    /* 0x0000 */ uchar BREQUESTTYPE;
    /* 0x0001 */ uchar BREQUEST;
    /* 0x0002 */ uint_16 WVALUE;
    /* 0x0004 */ uint_16 WINDEX;
    /* 0x0006 */ uint_16 WLENGTH;
} USB_SETUP, *USB_SETUP_PTR; // size = 0x8

typedef struct {
    /* 0x0000 */ void (*reset_ep0)(_usb_device_handle, boolean, uint_8, uint_8_ptr, uint_32);
    /* 0x0004 */ void (*get_desc)(_usb_device_handle, boolean, SETUP_STRUCT_PTR);
    /* 0x0008 */ void (*vendor)(_usb_device_handle, boolean, SETUP_STRUCT_PTR);
    /* 0x000C */ void (*initeps)(_usb_device_handle);
    /* 0x0010 */ void (*query)(OSBbUsbInfo*);
    /* 0x0014 */ void (*stall_ep)(_usb_device_handle, uint_8, boolean);
} usbdevfuncs; // size = 0x18

typedef struct dev_global_struct_s {
    /* 0x0000 */ uint_8 dev_state;
    /* 0x0001 */ uint_8 FIRST_SOF;
    /* 0x0002 */ uint_8 num_ifcs;
    /* 0x0004 */ usbdevfuncs* funcs;
} DEV_GLOBAL_STRUCT, *DEV_GLOBAL_STRUCT_PTR; // size = 0x8

#if 0
typedef union bdt_reg {
    // Structure format choice is based on OWN bit
    struct {
        // CPU writes these to give to the SIE
        uint_16 OWN     : 1;    // Ownership 0 -> 1
        uint_16 DATA01  : 1;    // Data Toggle Sync Value
        uint_16 KEEP    : 1;    // BD Keep Enable
        uint_16 NINC    : 1;    // Address Increment Disable
        uint_16 DTS     : 1;    // Data Toggle Sync Enable
        uint_16 STALL   : 1;    // Buffer Stall Enable
        uint_16         : 2;
    } CPU;
    struct {
        // SIE writes these to give to the CPU
        uint_16 OWN     : 1;    // Ownership 1 -> 0
        uint_16         : 1;
        uint_16 PID     : 4;    // Received Packet Identifier
        uint_16         : 2;
    } SIE;
} BDT_REG, *BDT_REG_PTR; // size = 0x2
#endif
typedef union bdt_reg {
    struct {
    /* 0x0000 [15:8] */ uint_16        : 8;
    /* 0x0000 [   7] */ uint_16 OWNS   : 1; // Indicates who owns the BD (1=SIE, 0=CPU)
    /* 0x0000 [   6] */ uint_16 DATA01 : 1; // Whether to send DATA0 or DATA1 PID
    /* 0x0000 [ 5:2] */ uint_16 PID    : 4; // Received PID
    /* 0x0000 [ 1:0] */ uint_16        : 2;
    } BITMAP;
    /* 0x0000 */ uint_16 BDTCTL;
} BDT_REG, *BDT_REG_PTR; // size = 0x2
#define BDTCTL_PID(bdtctl)  (((bdtctl) & (0xF << 2)) >> 2)
#define BDTCTL_OWNS(bdtctl) ((bdtctl) & 0x80)

typedef struct bdt_struct {
    /* 0x0000 */ uint_16 BC;                // Byte count, updated by the SIE based on how much data was sent/received
    /* 0x0002 */ BDT_REG REGISTER;          // Controls
    /* 0x0004 */ uint_8_ptr ADDRESS;        // DRAM address of buffer to read/write
} BDT_STRUCT, *BDT_STRUCT_PTR; // size = 0x8

#define BDT_SWAP_ODD_EVEN(BDT_PTR) ((BDT_STRUCT_PTR)((uint_32)(BDT_PTR) ^ 8))

typedef struct service_struct {
    /* 0x0000 */ uint_32 TYPE;
    /* 0x0004 */ void (*SERVICE)(_usb_device_handle, boolean, uint_8, uint_8_ptr, uint_32);
    /* 0x0008 */ struct service_struct* NEXT;
} SERVICE_STRUCT, *SERVICE_STRUCT_PTR; // size = 0xC

// 4 per endpoint: send/recv even/odd
typedef BDT_STRUCT VUSB_ENDPOINT_BDT_STRUCT[2][2];
typedef VUSB_ENDPOINT_BDT_STRUCT* VUSB_ENDPOINT_BDT_STRUCT_PTR;

typedef union {
    /* 0x0000 */ uint_32 W;
    /* 0x0000 */ struct {
        /* 0x0000 */ uint_16 L;
        /* 0x0002 */ uint_16 H;
    } B;
    /* 0x0000 */ uchar_ptr PB;
    /* 0x0000 */ struct BD* BDT_PTR;
} MP_STRUCT, *MP_STRUCT_PTR; // size = 0x4

typedef union {
    /* 0x0000 */ uint_16 W;
    /* 0x0000 */ struct {
        /* 0x0000 */ uint_8 L;
        /* 0x0001 */ uint_8 H;
    } B;
    /* 0x0000 */ uchar PB[2];
} MP_STRUCT16, *MP_STRUCT16_PTR; // size = 0x2

typedef struct BD {
    /* 0x0000 */ uint_16 BC;
    /* 0x0002 */ uint_16 PID;
    /* 0x0004 */ MP_STRUCT ADDR;
} HOST_BDT_STRUCT, *HOST_BDT_STRUCT_PTR;  // size = 0x8

typedef uint_32 USB_REGISTER, *USB_REGISTER_PTR;

typedef struct usb_struct {
    /* 0x0000 */ volatile USB_REGISTER INTSTATUS;
    /* 0x0004 */ volatile USB_REGISTER INTENABLE;
    /* 0x0008 */ volatile USB_REGISTER ERRORSTATUS;
    /* 0x000C */ volatile USB_REGISTER ERRORENABLE;
    /* 0x0010 */ volatile USB_REGISTER STATUS;
    /* 0x0014 */ volatile USB_REGISTER CONTROL;
    /* 0x0018 */ volatile USB_REGISTER ADDRESS;
    /* 0x001C */ volatile USB_REGISTER BDTPAGE1;
    /* 0x0020 */ volatile USB_REGISTER FRAMENUMLO;
    /* 0x0024 */ volatile USB_REGISTER FRAMENUMHI;
    /* 0x0028 */ volatile USB_REGISTER TOKEN;
    /* 0x002C */ volatile USB_REGISTER SOFTHRESHOLDLO;
    /* 0x0030 */ volatile USB_REGISTER BDTPAGE2;
    /* 0x0034 */ volatile USB_REGISTER BDTPAGE3;
    /* 0x0038 */ volatile USB_REGISTER SOFTHRESHOLDHI;
    /* 0x003C */ volatile USB_REGISTER RESERVED;
} USB_STRUCT, *USB_STRUCT_PTR; // size = 0x40

typedef struct xd_struct {
    /* 0x0000 */ volatile uint_8 STATUS;
    /* 0x0001 */ volatile uint_8 DIRECTION; // USB_RECV or USB_SEND
    /* 0x0002 */ volatile uint_8 BDTCTL;
    /* 0x0004 */ volatile uint_16 MAXPACKET;
    /* 0x0008 */ volatile uint_8_ptr STARTADDRESS;
    /* 0x000C */ volatile uint_8_ptr NEXTADDRESS;
    /* 0x0010 */ volatile uint_32 TODO;
    /* 0x0014 */ volatile uint_32 UNACKNOWLEDGEDBYTES;
    /* 0x0018 */ volatile uint_32 SOFAR;
    /* 0x001C */ volatile boolean DONT_ZERO_TERMINATE;
    /* 0x0020 */ volatile uint_8 TYPE; // endpoint type
    /* 0x0021 */ volatile uint_8 CTL;
    /* 0x0022 */ volatile uint_8 NEXTDATA01;
    /* 0x0023 */ volatile uint_8 NEXTODDEVEN;
    /* 0x0024 */ volatile uint_8 LASTODDEVEN;
    /* 0x0025 */ volatile uint_8 PACKETPENDING;
    /* 0x0026 */ volatile uint_8 MUSTSENDNULL;
    /* 0x0027 */ volatile uint_8 SETUP_TOKEN_RECEIVED;
    /* 0x0028 */ volatile uint_8 SETUP_BUFFER_QUEUED;
    /* 0x0029 */ volatile uint_8 RESERVED0[3];
    /* 0x002C */ volatile uint_8 CACHEALIGN0[16];
    /* 0x003C */ volatile uint_8 SETUP_BUFFER[8];
    /* 0x0044 */ volatile uint_8 RESERVED1[18];
    /* 0x0056 */ volatile uint_8 CACHEALIGN1[8];
} XD_STRUCT, *XD_STRUCT_PTR; // size = 0x60

// XD_STRUCT::STATUS
#define USB_STATUS_IDLE                 (0)
#define USB_STATUS_1                    (1)
#define USB_STATUS_TRANSFER_PENDING     (2)
#define USB_STATUS_TRANSFER_IN_PROGRESS (3)
#define USB_STATUS_4                    (4)
#define USB_STATUS_DISABLED             (5)
#define USB_STATUS_STALLED              (6)

// XD_STRUCT::DIRECTION
#define USB_RECV    0
#define USB_SEND    1

// XD_STRUCT::MAXPACKET
#define USB_MAX_PACKET_SIZE_FULLSPEED   (64)

// XD_STRUCT::TYPE
#define USB_ISOCHRONOUS_PIPE    (0x01)
#define USB_INTERRUPT_PIPE      (0x02)
#define USB_CONTROL_PIPE        (0x03)
#define USB_BULK_PIPE           (0x04)

typedef struct usb_dev_state_struct {
    /* 0x0000 */ volatile USB_STRUCT* USB;
    /* 0x0004 */ volatile USB_REGISTER_PTR ENDPT_REGS;
    /* 0x0008 */ BDT_STRUCT_PTR BDT_PTR;
    /* 0x000C */ VUSB_ENDPOINT_BDT_STRUCT_PTR USB_BDT_PAGE;
    /* 0x0010 */ pointer CALLBACK_STRUCT_PTR;
    /* 0x0014 */ XD_STRUCT_PTR XDSEND;
    /* 0x0018 */ XD_STRUCT_PTR XDRECV;
    /* 0x001C */ SERVICE_STRUCT_PTR SERVICE_HEAD_PTR;
    /* 0x0020 */ uint_16 USB_STATE;
    /* 0x0022 */ uint_16 USB_DEVICE_STATE;
    /* 0x0024 */ uint_16 USB_SOF_COUNT;
    /* 0x0026 */ uint_8 CTLR_NUM;
    /* 0x0027 */ uint_8 USB_CURR_CONFIG;
    /* 0x0028 */ uint_8 EP;
    /* 0x0029 */ uint_8 MAX_ENDPOINTS;
    /* 0x002A */ uint_8 DEV_NUM;
} USB_DEV_STATE_STRUCT, *USB_DEV_STATE_STRUCT_PTR; // size = 0x2C

typedef struct otg_state_machine_struct {
    /* 0x0000 */ uint_8 STATE;
    /* 0x0004 */ boolean A_BUS_DROP;
    /* 0x0008 */ boolean A_BUS_REQ;
    /* 0x000C */ boolean B_BUS_REQ;
    /* 0x0010 */ boolean HOST_UP;
    /* 0x0014 */ boolean DEVICE_UP;
    /* 0x0018 */ boolean A_SRP_DET;
    /* 0x001C */ int_32 A_SRP_TMR;
    /* 0x0020 */ boolean A_DATA_PULSE_DET;
    /* 0x0024 */ int_32 TB_SE0_SRP_TMR;
    /* 0x0028 */ boolean B_SE0_SRP;
    /* 0x002C */ uint_32 OTG_INT_STATUS;
    /* 0x0030 */ boolean A_BUS_RESUME;
    /* 0x0034 */ boolean A_BUS_SUSPEND;
    /* 0x0038 */ boolean A_BUS_SUSPEND_REQ;
    /* 0x003C */ boolean A_CONN;
    /* 0x0040 */ boolean B_BUS_RESUME;
    /* 0x0044 */ boolean B_BUS_SUSPEND;
    /* 0x0048 */ boolean B_CONN;
    /* 0x004C */ boolean A_SET_B_HNP_EN;
    /* 0x0050 */ boolean B_SESS_REQ;
    /* 0x0054 */ boolean B_SRP_DONE;
    /* 0x0058 */ boolean B_HNP_ENABLE;
    /* 0x005C */ boolean JSTATE;
    /* 0x0060 */ uint_32 JSTATE_COUNT;
    /* 0x0064 */ int_32 TA_WAIT_VRISE_TMR;
    /* 0x0068 */ int_32 TA_WAIT_BCON_TMR;
    /* 0x006C */ int_32 TA_AIDL_BDIS_TMR;
    /* 0x0070 */ int_32 TB_DATA_PLS_TMR;
    /* 0x0074 */ int_32 TB_SRP_INIT_TMR;
    /* 0x0078 */ int_32 TB_SRP_FAIL_TMR;
    /* 0x007C */ int_32 TB_ASE0_BRST_TMR;
} OTG_STATE_MACHINE_STRUCT, *OTG_STATE_MACHINE_STRUCT_PTR; // size = 0x80

typedef struct otg_struct {
    /* 0x0000 */ USB_REGISTER OTG_INT_STAT;
    /* 0x0004 */ USB_REGISTER OTG_INT_ENABLE;
    /* 0x0008 */ USB_REGISTER OTG_STAT;
    /* 0x000C */ USB_REGISTER OTG_CTL;
} OTG_STRUCT, *OTG_STRUCT_PTR; // size = 0x10

typedef struct usb_otg_state_structure {
    /* 0x0000 */ OTG_STRUCT_PTR OTG_REG_PTR;
    /* 0x0004 */ USB_STRUCT_PTR USB_REG_PTR;
    /* 0x0008 */ OTG_STATE_MACHINE_STRUCT STATE_STRUCT;
    /* 0x0088 */ void (*SERVICE)();
    /* 0x008C */ uint_8 DEV_NUM;
    /* 0x0090 */ pointer CALLBACK_STRUCT_PTR;
} USB_OTG_STATE_STRUCT, *USB_OTG_STATE_STRUCT_PTR; // size = 0x94

typedef struct {
    /* 0x0000 */ uint_8 BREQUESTTYPE;
    /* 0x0001 */ uint_8 BREQUEST;
    /* 0x0002 */ uint_16 WVALUE;
    /* 0x0004 */ uint_16 WINDEX;
    /* 0x0006 */ uint_16 WLENGTH;
} INTERNAL_USB_SETUP, *INTERNAL_USB_SETUP_PTR; // size = 0x8

typedef struct host_service_struct {
    /* 0x0000 */ uint_32 TYPE;
    /* 0x0004 */ void (*SERVICE)(_usb_host_handle handle, uint_32 length);     
    /* 0x0008 */ struct host_service_struct* NEXT;
} USB_SERVICE_STRUCT, *USB_SERVICE_STRUCT_PTR; // size = 0xC

typedef struct {
    /* 0x0000 */ uint_8 ADDRESS;
    /* 0x0001 */ uint_8 EP;
    /* 0x0002 */ uint_8 DIRECTION;
    /* 0x0003 */ uint_8 INTERVAL;
    /* 0x0004 */ uint_8 PIPETYPE;
    /* 0x0005 */ uint_8 CURRENT_INTERVAL;
    /* 0x0006 */ uint_8 NAK_COUNT;
    /* 0x0007 */ uint_8 CURRENT_NAK_COUNT;
    /* 0x0008 */ uint_8 USB_PIPE_TRANSACTION_STATE;
    /* 0x0009 */ uint_8 STATUS;
    /* 0x000A */ uint_8 NEXTDATA01;
    /* 0x000B */ uint_8 MUSTSENDNULL;
    /* 0x000C */ uint_32 SOFAR;
    /* 0x0010 */ uint_32 TODO1;
    /* 0x0014 */ uint_32 TODO2;
    /* 0x0018 */ uint_16 MAX_PKT_SIZE;
    /* 0x001A */ int_16 PIPE_ID;
    /* 0x001C */ int_16 NEXT_PIPE;
    /* 0x0020 */ boolean OPEN;
    /* 0x0024 */ boolean SEND_PHASE;
    /* 0x0028 */ boolean PACKETPENDING;
    /* 0x002C */ uchar_ptr TX1_PTR;
    /* 0x0030 */ uchar_ptr TX2_PTR;
    /* 0x0034 */ uchar_ptr RX_PTR;
    /* 0x0038 */ boolean FIRST_PHASE;
    /* 0x003C */ boolean DONT_ZERO_TERMINATE;
    /* 0x0040 */ uint_8 RESERVED[2];
} PIPE_DESCRIPTOR_STRUCT, *PIPE_DESCRIPTOR_STRUCT_PTR; // size = 0x44

typedef struct usb_host_state_structure {
    /* 0x0000 */ USB_STRUCT_PTR DEV_PTR;
    /* 0x0004 */ HOST_BDT_STRUCT_PTR USB_BDT_PAGE;
    /* 0x0008 */ USB_REGISTER_PTR ENDPT_HOST_RG;
    /* 0x000C */ USB_SERVICE_STRUCT_PTR SERVICE_HEAD_PTR;
    /* 0x0010 */ PIPE_DESCRIPTOR_STRUCT_PTR PIPE_DESCRIPTOR_BASE_PTR;
    /* 0x0014 */ pointer CALLBACK_STRUCT_PTR;
    /* 0x0018 */ HOST_BDT_STRUCT_PTR CURRENT_BDT;
    /* 0x001C */ uint_8 CTLR_NUM;
    /* 0x001D */ uint_8 DEV_NUM;
    /* 0x001E */ uint_8 USB_OUT;
    /* 0x001F */ uint_8 USB_IN;
    /* 0x0020 */ uint_8 USB_BUFFERED;
    /* 0x0021 */ uint_8 USB_BUFFERED_TOKEN;
    /* 0x0022 */ uint_8 USB_BUFFERED_ADDRESS;
    /* 0x0023 */ uint_8 USB_BUFFERED_TYPE;
    /* 0x0024 */ uint_8 USB_NEXT_BUFFERED_ADDRESS;
    /* 0x0025 */ uint_8 USB_NEXT_BUFFERED_TOKEN;
    /* 0x0026 */ uint_8 USB_NEXT_BUFFERED_TYPE;
    /* 0x0027 */ uint_8 USB_PKT_PENDING;
    /* 0x0028 */ uint_8 NO_HUB;
    /* 0x0029 */ uint_8 SPEED;
    /* 0x002A */ uint_8 MAX_PIPES;
    /* 0x002C */ HOST_BDT_STRUCT USB_BUFFERED_BDT;
    /* 0x0034 */ HOST_BDT_STRUCT USB_NEXT_BUFFERED_BDT;
    /* 0x003C */ HOST_BDT_STRUCT USB_TEMP_SND_BDT;
    /* 0x0044 */ HOST_BDT_STRUCT USB_TEMP_RCV_BDT;
    /* 0x004C */ boolean SND_TOK_IN_PROGRESS;
    /* 0x0050 */ boolean NEXT_PKT_QUEUED;
    /* 0x0054 */ boolean USB_TOKEN_BUFFERED;
    /* 0x0058 */ int_16 TOK_THIS_FRAME;
    /* 0x005A */ int_16 CTRL_PIPE1;
    /* 0x005C */ int_16 CURRENT_PIPE_ID;
    /* 0x005E */ int_16 NEXT_PIPE_ID;
    /* 0x0060 */ int_16 CURRENT_ISO_HEAD;
    /* 0x0062 */ int_16 CURRENT_ISO_TAIL;
    /* 0x0064 */ int_16 CURRENT_INTR_HEAD;
    /* 0x0066 */ int_16 CURRENT_INTR_TAIL;
    /* 0x0068 */ int_16 CURRENT_CTRL_HEAD;
    /* 0x006A */ int_16 CURRENT_CTRL_TAIL;
    /* 0x006C */ int_16 CURRENT_BULK_HEAD;
    /* 0x006E */ int_16 CURRENT_BULK_TAIL;
} USB_HOST_STATE_STRUCT, *USB_HOST_STATE_STRUCT_PTR; // size = 0x70

typedef struct dev_desc {
    /* 0x0000 */ uchar BLENGTH;
    /* 0x0001 */ uchar BDESCTYPE;
    /* 0x0002 */ uint_16 BCDUSB;
    /* 0x0004 */ uchar BDEVICECLASS;
    /* 0x0005 */ uchar BDEVICESUBCLASS;
    /* 0x0006 */ uchar BDEVICEPROTOCOL;
    /* 0x0007 */ uchar BMAXPACKETSIZE;
    /* 0x0008 */ uint_16 IDVENDOR;
    /* 0x000A */ uint_16 IDPRODUCT;
    /* 0x000C */ uint_16 BCDDEVICE;
    /* 0x000E */ uchar BMANUFACTURER;
    /* 0x000F */ uchar BPRODUCT;
    /* 0x0010 */ uchar BSERIALNUMBER;
    /* 0x0011 */ uchar BNUMCONFIGURATIONS;
} USB_DEV_DESC, *USB_DEV_DESC_PTR; // size = 0x12
typedef struct dev_desc usb_dev_desc;

typedef struct usb_host_driver_funcs_s {
    /* 0x0000 */ int (*match)(usb_dev_desc* uddp);
    /* 0x0004 */ void (*open)(_usb_host_handle handle);
    /* 0x0008 */ void (*poll)(_usb_host_handle handle);
    /* 0x000C */ void (*send)(_usb_host_handle handle, u8* buffer, s32 len, u64 offset);
    /* 0x0010 */ void (*recv)(_usb_host_handle handle, u8* buffer, s32 len, u64 offset);
    /* 0x0014 */ void (*detach)(_usb_host_handle handle, uint_32 length);
    /* 0x0018 */ void (*chap9)(_usb_host_handle handle); 
} usb_host_driver_funcs; // size = 0x1C

typedef struct usb_host_driver_s {
    /* 0x0000 */ usb_dev_desc* desc;
    /* 0x0004 */ usb_host_driver_funcs* funcs;
    /* 0x0008 */ char* name;
} usb_host_driver; // size = 0xC

typedef struct host_global_struct {
    /* 0x0000 */ volatile uint_8 USB_CURRENT_ADDRESS;
    /* 0x0001 */ volatile uint_8 DEVICE_ADDRESS;
    /* 0x0002 */ volatile int_16 DEFAULT_PIPE;
    /* 0x0004 */ volatile boolean DEVICE_ATTACHED;
    /* 0x0008 */ volatile boolean ENUMERATION_DONE;
    /* 0x000C */ volatile boolean REGISTER_SERVICES;
    /* 0x0010 */ volatile boolean TRANSACTION_COMPLETE;
    /* 0x0014 */ volatile uint_8 ENUMERATION_STATE;
    /* 0x0015 */ volatile uchar RX_TEST_BUFFER[20];
    /* 0x002A */ volatile USB_DEV_DESC RX_DESC_DEVICE;
    /* 0x003C */ volatile uint_8 RX_CONFIG_DESC[64];
    /* 0x007C */ volatile uchar MFG_STR[64];
    /* 0x00BC */ volatile uchar PROD_STR[64];
    /* 0x00FC */ usb_host_driver* volatile driver;
    /* 0x0100 */ _usb_ext_handle* volatile curhandle;
} HOST_GLOBAL_STRUCT, *HOST_GLOBAL_STRUCT_PTR; // size = 0x104

extern _usb_ctlr_state_t _usb_ctlr_state[2];
extern DEV_GLOBAL_STRUCT dev_global_struct;
extern _usb_host_handle __osArcHostHandle[2];
extern usbdevfuncs rdbs_dev_funcs;
extern u32 __Usb_Reset_Count[2];
extern void* __usb_endpt_desc_reg;
extern void* __usb_svc_callback_reg;

// Device driver public?

void dev_bus_suspend(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length);

// Public Device?

uint_8 _usb_device_init(uint_8 controller_number, uint_8 device_number, _usb_device_handle* handlep, uint_8 number_of_endpoints);
void _usb_device_shutdown(_usb_device_handle handle);

uint_8 _usb_device_init_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_16 max_packet_size, uint_8 direction, uint_8 endpoint_type, uint_8 flag);
uint_8 _usb_device_deinit_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction);

uint_8 _usb_device_cancel_transfer(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction);

uint_8 _usb_device_get_status(_usb_device_handle handle, uint_8 component, uint_16_ptr status);
uint_8 _usb_device_set_status(_usb_device_handle handle, uint_8 component, uint_16 setting);

void _usb_device_read_setup_data(_usb_device_handle handle, uint_8 endpoint_num, uchar_ptr buff_ptr);

uint_8 _usb_device_recv_data(_usb_device_handle handle, uint_8 endpoint_number, uchar_ptr buff_ptr, uint_32 size);
uint_8 _usb_device_send_data(_usb_device_handle handle, uint_8 endpoint_number, uchar_ptr buff_ptr, uint_32 size);

void _usb_device_stall_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction);
void _usb_device_unstall_endpoint(_usb_device_handle handle, uint_8 endpoint_number, uint_8 direction);

uint_8 _usb_device_get_transfer_status(_usb_device_handle handle, uint_8 endpoint_number, uint_8 direction);

uint_8 _usb_device_register_service(_usb_device_handle handle, uint_8 event_endpoint, void (*service)(_usb_device_handle, boolean, uint_8, uint_8_ptr, uint_32));
uint_8 _usb_device_call_service(_usb_device_handle handle, uint_8 type, boolean setup, boolean direction, uint_8_ptr buffer_ptr, uint_32 length);
uint_8 _usb_device_unregister_service(_usb_device_handle handle, uint_8 event_endpoint);

// Public Host ?

uint_8 _usb_host_cancel_transfer(_usb_host_handle handle, int_16 pipeid);
void _usb_host_close_pipe(_usb_host_handle handle, int_16 pipeid);
void _usb_host_close_all_pipes(_usb_host_handle handle);
uint_8 _usb_host_init(uint_8 controller_number, uint_8 device_number, uint_8 max_pipe_num, _usb_host_handle* handlep);
uint_8 _usb_host_register_service(_usb_host_handle handle, uint_8 type, void (*service)(_usb_host_handle handle, uint_32 length));
uint_8 _usb_host_call_service(_usb_host_handle handle, uint_8 type, uint_32 length);
int_16 _usb_host_open_pipe(_usb_host_handle handle, uint_8 address, uint_8 ep_num, uint_8 direction, uint_8 interval, uint_8 pipe_type, uint_16 max_pkt_size, uint_8 nak_count, uint_8 flag);
uint_8 _usb_host_send_setup(_usb_host_handle handle, int_16 pipeid, uchar_ptr tx1_ptr, uchar_ptr tx2_ptr, uchar_ptr rx_ptr, uint_32 rx_length, uint_32 length2);
uint_8 _usb_host_get_transfer_status(_usb_host_handle handle, int_16 pipeid);
void _usb_host_queue_pkts(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_update_current_head(_usb_host_handle handle, uint_8 pipetype);
void _usb_host_update_current_interval_on_queue(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_update_interval_for_pipes(_usb_host_handle handle);
uint_8 _usb_host_recv_data(_usb_host_handle handle, int_16 pipeid, uchar_ptr buff_ptr, uint_32 length);
void _usb_host_shutdown(_usb_host_handle handle);
void _usb_host_bus_control(_usb_host_handle handle, uint_8 bcontrol);
uint_8 _usb_host_send_data(_usb_host_handle handle, int_16 pipeid, uchar_ptr buff_ptr, uint_32 length);
uint_8 _usb_host_unregister_service(_usb_host_handle handle, uint_8 type);



// ?

void rdbs_device_init(_usb_device_handle handle);
void __usb_arc_device_setup(int which, _usb_device_handle* handlep);
void _usb_device_state_machine(_usb_device_handle handle);

// Internal

uint_8 _usb_dci_vusb11_init(_usb_device_handle handle);
void _usb_dci_vusb11_shutdown(_usb_device_handle handle);
void _usb_dci_vusb11_bus_reset(_usb_device_handle handle);

void _usb_dci_vusb11_isr(_usb_device_handle handle);

void _usb_dci_vusb11_init_endpoint(_usb_device_handle handle, uint_8 ep, XD_STRUCT_PTR pxd);
void _usb_dci_vusb11_deinit_endpoint(_usb_device_handle handle, uint_8 endpoint_num, XD_STRUCT_PTR pxd);
void _usb_dci_vusb11_set_endpoint_status(_usb_device_handle handle, uint_8 ep, uint_8 stall);
uint_8 _usb_dci_vusb11_get_endpoint_status(_usb_device_handle handle, uint_8 ep);
void _usb_dci_vusb11_unstall_endpoint(_usb_device_handle handle, uint_8 ep);

void _usb_dci_vusb11_read_setup(_usb_device_handle handle, uint_8 ep_num, uchar_ptr buff_ptr);

void _usb_dci_vusb11_submit_transfer(_usb_device_handle handle, uint_8 direction, uint_8 ep);
void _usb_dci_vusb11_cancel_transfer(_usb_device_handle handle, uint_8 direction, uint_8 ep);

void _usb_dci_vusb11_submit_BDT(BDT_STRUCT_PTR pBDT, XD_STRUCT_PTR pxd);

void _usb_hci_vusb11_send_data(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_hci_vusb11_send_setup(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_hci_vusb11_recv_data(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_hci_vusb11_cancel_transfer(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_set_bdt_page(_usb_host_handle handle);
HOST_BDT_STRUCT_PTR _usb_host_vusb11_get_next_in_bdt(_usb_host_handle handle);
HOST_BDT_STRUCT_PTR _usb_host_vusb11_get_next_out_bdt(_usb_host_handle handle);
void _usb_host_vusb11_sched_pending_pkts(_usb_host_handle handle);
void _usb_host_vusb11_sched_iso_pkts(_usb_host_handle handle);
void _usb_host_vusb11_initiate_correct_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_hci_vusb11_isr(_usb_host_handle handle);
uint_8 _usb_hci_vusb11_init(_usb_host_handle handle);
void _usb_host_delay(_usb_host_handle handle, uint_32 delay);
void _usb_host_vusb11_reset_the_device(_usb_host_handle handle);
void _usb_host_vusb11_process_attach(_usb_host_handle handle);
void _usb_host_vusb11_process_detach(_usb_host_handle handle);
void _usb_host_vusb11_send_in_token(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_send_out_token(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_send_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType);
void _usb_host_vusb11_process_token_done(_usb_host_handle handle, uint_8 direction, uint_32 error_status);
void _usb_host_vusb11_init_send_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_init_recv_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_init_setup_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr);
void _usb_host_vusb11_resend_token(_usb_host_handle handle, uint_8 bDirection);
void _usb_host_buffer_bdt(_usb_host_handle handle, HOST_BDT_STRUCT_PTR BDT_ptr);
void _usb_host_buffer_next_bdt(_usb_host_handle handle, HOST_BDT_STRUCT_PTR BDT_ptr);
void _usb_host_buffer_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType);
void _usb_host_buffer_next_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType);
void _usb_hci_vusb11_bus_control(_usb_host_handle handle, uint_8 bControl);
void _usb_hci_vusb11_shutdown(_usb_host_handle handle);

void _usb_bdt_copy_swab(HOST_BDT_STRUCT_PTR from_bdt, HOST_BDT_STRUCT_PTR to_bdt);
#define _usb_bdt_copy_swab_host(from_bdt, to_bdt)   _usb_bdt_copy_swab((from_bdt), (to_bdt))
#define _usb_bdt_copy_swab_device(from_bdt, to_bdt) _usb_bdt_copy_swab((HOST_BDT_STRUCT_PTR)(from_bdt), (HOST_BDT_STRUCT_PTR)(to_bdt))

void __usb_splhigh(void);
void __usb_splx(void);

#define USB_MAX_DEVICES 4

// _usb_device_* error codes
#define USB_DEV_ERR_OK                0
#define USB_DEV_ERR_ALLOC_SERVICE     1
#define USB_DEV_ERR_INVALID_COMPONENT 2
#define USB_DEV_ERR_SERVICE_NOT_FOUND 3
#define USB_DEV_ERR_SERVICE_EXISTS    4
#define USB_DEV_ERR_BAD_ENDPOINT_NUM  5
#define USB_DEV_ERR_ENDPT_DISABLED    6
#define USB_DEV_ERR_XFER_IN_PROGRESS  7
#define USB_DEV_ERR_ENDPT_STALLED     8
#define USB_DEV_ERR_ALLOC_XD          11
#define USB_DEV_ERR_BAD_DEVICE_NUM    15

#define USB_HOST_ERR_ALLOC_SERVICE 31
#define USB_HOST_ERR_BAD_PIPE_NUM  79
#define USB_HOST_ERR_111           111
#define USB_HOST_ERR_3             3
#define USB_HOST_ERR_7             7


#define BSWAP16(x) \
    ((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))

#define BSWAP32(x) \
    (((x) & 0xFF) << 0x18) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 0x18)

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define USB_STATE_0         (0x00)
#define USB_SELF_POWERED    (0x01)
#define USB_REMOTE_WAKEUP   (0x02)

// _usb_device_get_status::component
#define USB_STATUS_DEVICE_STATE         (0x01)
#define USB_STATUS_INTERFACE            (0x02)
#define USB_STATUS_ADDRESS              (0x03)
#define USB_STATUS_CURRENT_CONFIG       (0x04)
#define USB_STATUS_SOF_COUNT            (0x05)
#define USB_STATUS_DEVICE               (0x06)
#define USB_STATUS_TEST_MODE            (0x07)
#define USB_STATUS_ENDPOINT             (0x10)
#define USB_STATUS_ENDPOINT_NUMBER_MASK (0x0F)

// Services

#define USB_SERVICE_EP(n)       (n)     // 0 to 15
#define USB_SERVICE_RESET_EP0   0x10
#define USB_SERVICE_RESUME      0x11
#define USB_SERVICE_SUSPEND     0x12
#define USB_SERVICE_ERROR       0x13
#define USB_SERVICE_STALL       0x14
#define USB_SERVICE_SOF         0x15
#define USB_SERVICE_ALLOC_MAX   0x15 // not 0x16 ?

#define USB_HOST_SERVICE_PIPE(n) (n)     // 0 to 15
#define USB_HOST_SERVICE_0xFC   0xFC
#define USB_HOST_SERVICE_RESUME 0xFD
#define USB_HOST_SERVICE_ATTACH 0xFE
#define USB_HOST_SERVICE_DETACH 0xFF

// Chapter 9

// Standard descriptor types
#define USB_DESC_DEVICE         1
#define USB_DESC_CONFIG         2
#define USB_DESC_STRING         3
#define USB_DESC_INTERFACE      4
#define USB_DESC_ENDPOINT       5
// OTG descriptor type
#define USB_DESC_OTG            9

// bRequestType [7]
#define USB_REQ_DIR_IN          0x80
#define USB_REQ_DIR_OUT         0x00
// bRequestType [6:5]
#define USB_REQ_TYPE_MASK       0x60
#define USB_REQ_TYPE_STANDARD   0x00
#define USB_REQ_TYPE_CLASS      0x20
#define USB_REQ_TYPE_VENDOR     0x40
#define USB_REQ_TYPE_RESERVED   0x60
// bRequestType [4:0]
#define USB_REQ_RECPT_DEVICE    0x00
#define USB_REQ_RECPT_INTERFACE 0x01
#define USB_REQ_RECPT_ENDPOINT  0x02
#define USB_REQ_RECPT_OTHER     0x03

// bRequest (STANDARD)
#define USB_REQ_GET_STATUS          0
#define USB_REQ_CLEAR_FEATURE       1
// reserved                         2
#define USB_REQ_SET_FEATURE         3
// reserved                         4
#define USB_REQ_SET_ADDRESS         5
#define USB_REQ_GET_DESCRIPTOR      6
#define USB_REQ_SET_DESCRIPTOR      7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE       10
#define USB_REQ_SET_INTERFACE       11
#define USB_REQ_SYNCH_FRAME         12

// Get Descriptor wValues
#define USB_GET_DESC_DEVICE         (USB_DESC_DEVICE    << 8)
#define USB_GET_DESC_CONFIG         (USB_DESC_CONFIG    << 8)
#define USB_GET_DESC_STRING         (USB_DESC_STRING    << 8)
#define USB_GET_DESC_INTERFACE      (USB_DESC_INTERFACE << 8)
#define USB_GET_DESC_ENDPOINT       (USB_DESC_ENDPOINT  << 8)

// Descriptors

#define USB_DESC_16(n) ((n) & 0xFF), (((n) >> 8) & 0xFF)

#define USB_CHAR(C) (C), '\x00'

// OTG Descriptor bmAttributes
#define USB_DESC_OTG_SRP (1 << 0)
#define USB_DESC_OTG_HNP (1 << 1)
#define USB_DESC_OTG_ADP (1 << 2)

// Endpoint Descriptor bmAttributes
#define USB_ENDPOINT_XFER_TYPE_CTRL     (0 << 0)
#define USB_ENDPOINT_XFER_TYPE_ISO      (1 << 0)
#define USB_ENDPOINT_XFER_TYPE_BULK     (2 << 0)
#define USB_ENDPOINT_XFER_TYPE_INTR     (3 << 0)
// Isochronous only, for others leave as 0
#define USB_ENDPOINT_ISO_NOSYNC         (0 << 2)
#define USB_ENDPOINT_ISO_ASYNC          (1 << 2)
#define USB_ENDPOINT_ISO_ADAPTIVE       (2 << 2)
#define USB_ENDPOINT_ISO_SYNC           (3 << 2)
#define USB_ENDPOINT_ISO_USAGE_DATA     (0 << 4)
#define USB_ENDPOINT_ISO_USAGE_FEEDBACK (1 << 4)
#define USB_ENDPOINT_ISO_USAGE_IMPLICIT (2 << 4)
#define USB_ENDPOINT_ISO_USAGE_RESERVED (3 << 4)

#define USB_ENDPOINT_IN     0x80
#define USB_ENDPOINT_OUT    0x00

#define USB_ENDPOINT_ATTR_SELF_POWERED 0x40

// Hardware

#include "usb_hw.h"

#endif
