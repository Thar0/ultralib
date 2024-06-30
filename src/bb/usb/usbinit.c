#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "macros.h"
#include "usb.h"
#ident "$Revision: 1.1 $"

OSMesgQueue __osBbUsbCtlrQ[2];
void* __usb_svc_callback_reg;
void* __usb_endpt_desc_reg;
static OSThread __osBbUsbMgr[2];
static char __osBbUsbMgrStacks[2 * 0x2000];
static OSMesg __osBbUsbMesgs[2 * 128];
static __OSBbUsbMesg __osBbUsbEventMesg[2];
static u8 __usb_svc_callback_buffer[OS_DCACHE_ROUNDUP_SIZE(2 * USB_SERVICE_ALLOC_MAX * sizeof(SERVICE_STRUCT))];
static u8 __usb_endpt_desc_buffer[OS_DCACHE_ROUNDUP_SIZE(2 * 2 * sizeof(XD_STRUCT) * USB_NUM_ENDPOINTS)];

#define USB_NUM_MSGS_PER_CTLR (ARRLEN(__osBbUsbMesgs) / 2)

void __usbHwInit(void);

void __usbDevWrite(_usb_ext_handle*);
void __usbDevRead(_usb_ext_handle*);
s32 __usbDevInterrupt(s32, __OSBbUsbMesg*);

static s32 __osBbUsbInterrupt(s32 which, __OSBbUsbMesg* mp) {
    return __usbDevInterrupt(which, mp);
}

static s32 __osBbUsbRead(s32 which, __OSBbUsbMesg* ump) {
    _usb_ext_handle* uhp = (_usb_ext_handle*)ump->u.umrw.umrw_handle;

    if (uhp->uh_host_handle == NULL) {
        return -0xC6;
    }
    if (uhp->uh_rd_msg != NULL) {
        return -0xC7;
    }

    uhp->uh_rd_msg = ump;
    uhp->uh_rd_buffer = ump->u.umrw.umrw_buffer;
    uhp->uh_rd_offset = ump->u.umrw.umrw_offset;
    uhp->uh_rd_len = ump->u.umrw.umrw_len;

    __usbDevRead(uhp);
    return 0;
}

static s32 __osBbUsbWrite(s32 which, __OSBbUsbMesg* ump) {
    _usb_ext_handle* uhp = (_usb_ext_handle*)ump->u.umrw.umrw_handle;

    if (uhp->uh_host_handle == NULL) {
        return -0xC6;
    }
    if (uhp->uh_wr_msg != NULL) {
        return -0xC7;
    }

    uhp->uh_wr_msg = ump;
    uhp->uh_wr_buffer = ump->u.umrw.umrw_buffer;
    uhp->uh_wr_offset = ump->u.umrw.umrw_offset;
    uhp->uh_wr_len = ump->u.umrw.umrw_len;

    __usbDevWrite(uhp);
    return 0;
}

static void __osBbUsbMgrProc(char* arg) {
    OSMesg msg;
    __OSBbUsbMesg* mp;
    s32 ret;
    s32 which = osGetThreadId(NULL) - 0xC45;

    while (TRUE) {
        osRecvMesg((OSMesgQueue*)arg, &msg, OS_MESG_BLOCK);
        mp = (__OSBbUsbMesg*)msg;

        switch (mp->um_type) {
            case 0: // usb0
            case 1: // usb1
                ret = __osBbUsbInterrupt(which, mp);
                break;
            case 4:
                ret = __osBbUsbRead(which, mp);
                break;
            case 5:
                ret = __osBbUsbWrite(which, mp);
                break;
            default:
                ret = EUSBRINVAL;
                break;
        }

        if (ret != 0) {
            mp->um_ret = ret;
            mp->um_type |= 0x80;
            osSendMesg(mp->um_rq, mp, OS_MESG_NOBLOCK);
        }
    }
}

static s32 __osBbUsbThreadInit(s32 which) {
    if (which != 0 && which != 1) {
        return -1;
    }

    osCreateMesgQueue(&__osBbUsbCtlrQ[which], &__osBbUsbMesgs[which * USB_NUM_MSGS_PER_CTLR], USB_NUM_MSGS_PER_CTLR);

    osCreateThread(&__osBbUsbMgr[which], 0xC45 + which, (void (*)(void *))__osBbUsbMgrProc,
            &__osBbUsbCtlrQ[which],
            &__osBbUsbMgrStacks[(which + 1) * (sizeof(__osBbUsbMgrStacks) / 2)],
            (which == 1) ? 230 : 232);
    osStartThread(&__osBbUsbMgr[which]);

    bzero(&__osBbUsbEventMesg[which], sizeof(__OSBbUsbMesg));
    __osBbUsbEventMesg[which].um_type = which != 0;

    osSetEventMesg((which != 0) ? OS_EVENT_USB1 : OS_EVENT_USB0, &__osBbUsbCtlrQ[which], &__osBbUsbEventMesg[which]);
    return 0;
}

s32 osBbUsbSetCtlrModes(s32 which, u32 mask) {
    if (which != 0 && which != 1) {
        return -1;
    }
    _usb_ctlr_state[which].ucs_mask = mask;
    _usb_ctlr_state[which].ucs_mode &= ~mask;
    return 0;
}

s32 osBbUsbInit(void) {
    s32 i;
    s32 numctlr = 0;

    __usb_svc_callback_reg = osCreateRegion(__usb_svc_callback_buffer, sizeof(__usb_svc_callback_buffer), sizeof(SERVICE_STRUCT), 0);
    __usb_endpt_desc_reg = osCreateRegion(__usb_endpt_desc_buffer, sizeof(__usb_endpt_desc_buffer), sizeof(XD_STRUCT) * USB_NUM_ENDPOINTS, 0);

    for (i = 0; i < 2; i++) {
        if (__osBbUsbThreadInit(i) == 0) {
            numctlr++;
        }
    }

#ifdef BB_SA_1041
    // TODO maybe the other way around
    __usb_host_driver_register(&echo_host_driver);
    __usb_host_driver_register(&rdb_host_driver);
#endif

    __usbHwInit();

    return numctlr;
}
