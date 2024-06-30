#include "PR/os_internal.h"
#include "PR/bcp.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

extern HOST_GLOBAL_STRUCT host_global_struct[2];

void __osBbDelay(u32);

_usb_ctlr_state_t _usb_ctlr_state[2] = {
    { UCS_MODE_NONE, UCS_MASK_HOST | UCS_MASK_DEVICE },
    { UCS_MODE_NONE, UCS_MASK_HOST | UCS_MASK_DEVICE },
};

#ifdef BB_SA_1041
// In earlier versions of the library, _usb_ctlr_state was just s32[2] with no ucs_mask field
#define UCS_MODE(which) (_usb_ctlr_state[(which)])
#define UCS_DEVICE_OK(which) (1)
#define UCS_HOST_OK(which) (1)
#else
#define UCS_MODE(which) (_usb_ctlr_state[which].ucs_mode)
#define UCS_DEVICE_OK(which) (_usb_ctlr_state[(which)].ucs_mask & UCS_MASK_DEVICE)
#define UCS_HOST_OK(which) (_usb_ctlr_state[(which)].ucs_mask & UCS_MASK_HOST)
#endif

static int save_im_level = 0;
_usb_host_handle __osArcHostHandle[2];
static _usb_device_handle __osArcDeviceHandle[2];
static OSIntMask save_im;
u32 __Usb_Reset_Count[2];

static void __usbOtgStateChange(s32 which);

void __usb_splhigh(void) {
    OSIntMask im = __osDisableInt();

    save_im_level++;
    if (save_im_level == 1) {
        save_im = im;
    }
}

void __usb_splx(void) {
    save_im_level--;
    if (save_im_level == 0) {
        __osRestoreInt(save_im);
    }
}

s32 __usbDevInterrupt(s32 which) {
    // Get raised OTG interrupts
    u32 val = IO_READ(USB_OTGISTAT_REG_ALT(which));
    u32 mask = IO_READ(USB_OTGICR_REG(which));
    val &= mask;

    if (val != 0) {
        // If there are OTG interrupts to service

        if (val & USB_OTGISTAT_AVBUSCHG) {
            __usbOtgStateChange(which);
        }
        if (val & USB_OTGISTAT_B_SESS_CHG) {
            __usbOtgStateChange(which);
        }
        if (val & USB_OTGISTAT_SESSVLDCHG) {
            __usbOtgStateChange(which);
        }
        if (val & USB_OTGISTAT_LINE_STATE_CHG) {
            __usbOtgStateChange(which);
        }
        if (val & USB_OTGISTAT_ONEMSEC) {
            static int msec_count = 0;

            msec_count++;
            if (msec_count >= 5) {
                IO_WRITE(USB_OTGICR_REG(which), USB_OTGICR_IDEN | USB_OTGICR_LINESTATEEN | USB_OTGICR_SESSVLDEN | USB_OTGICR_BSESSEN | USB_OTGICR_RESERVED1);
            }
        }
        if (val & USB_OTGISTAT_IDCHG) {
            __usbOtgStateChange(which);
        }

        // Ack OTG interrupts
#ifndef BB_SA_1041
        IO_WRITE(USB_OTGISTAT_REG(which), val);
#endif
    }
#ifdef BB_SA_1041
    IO_WRITE(USB_OTGISTAT_REG(which), val);
#endif

    // Get other raised USB interrupts (but do nothing? handling is in the ISR called below)
    val = IO_READ(USB_ISTAT_REG(which));
    mask = IO_READ(USB_INTEN_REG(which));
    val &= mask;

    // Handle other USB interrupts
    if (_usb_ctlr_state[which].ucs_mode == UCS_MODE_HOST) {
#ifdef BB_SA_1041
        _usb_hci_vusb11_isr(__osArcHostHandle[which])
        _usb_host_state_machine(__osArcHostHandle[which]);
#endif
    } else if (_usb_ctlr_state[which].ucs_mode == UCS_MODE_DEVICE) {
        _usb_dci_vusb11_isr(__osArcDeviceHandle[which]);
#ifdef BB_SA_1041
        _usb_device_state_machine(__osArcDeviceHandle[which]);
#endif
    }

    // Re-enable USB0/USB1 interrupts that were disabled in interrupt handling (in exceptasm)
    IO_WRITE(MI_3C_REG, (which == 0) ? (1 << 21) : (1 << 23));
    return 0;
}

void __usbDevRead(_usb_ext_handle* uhp) {
#ifdef BB_SA_1041
    HOST_GLOBAL_STRUCT_PTR hgp = &D_805884D8[uhp->uh_which];
    _usb_host_handle handle = D_805205F0[uhp->uh_which];

    if (hgp->curhandle == NULL) {
        hgp->curhandle = uhp;
        hgp->driver->funcs->recv(handle, uhp->uh_rd_buffer, uhp->uh_rd_len, uhp->uh_rd_offset);
    }
#endif
}

void __usbDevWrite(_usb_ext_handle* uhp) {
#ifdef BB_SA_1041
    HOST_GLOBAL_STRUCT_PTR hgp = &D_805884D8[uhp->uh_which];
    _usb_host_handle handle = D_805205F0[uhp->uh_which];

    if (hgp->curhandle == NULL) {
        hgp->curhandle = uhp;
        hgp->driver->funcs->send(handle, uhp->uh_wr_buffer, uhp->uh_wr_len, uhp->uh_wr_offset);
    }
#endif
}

static void __usbCtlrTest(s32 which) {
}

void __usb_arc_host_setup(int which, _usb_host_handle* handlep);

static void __usbHostMode(s32 which) {
#ifdef BB_SA_1041
    // Enable pull-down on D+ and D- to indicate host
    // TODO what is RESERVED3 for iQue Player?
    IO_WRITE(USB_OTGCTL_REG_ALT(which), USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW | USB_OTGCTL_RESERVED3 | USB_OTGCTL_OTGEN);

    __usb_arc_host_setup(which, &__osArcHostHandle[which]);

    // Enable USB0/USB1 interrupts to begin handling events
    IO_WRITE(MI_3C_REG, (which == 0) ? (1 << 21) : (1 << 23));
#endif
}

static void __usbDeviceMode(s32 which) {
    __usb_arc_device_setup(which, &__osArcDeviceHandle[which]);

    // Disable pull-ups on D+ and D-
    IO_WRITE(USB_OTGCTL_REG(which), USB_OTGCTL_OTGEN);

    __osBbDelay(1000);

    if (which != 0) {
        // Enable pull-up on D+, pull-down on D+ and D-
        // Enabling both pull-downs indicates host? Sounds wrong considering this is device mode setup..
        IO_WRITE(USB_OTGCTL_REG(1), USB_OTGCTL_DPHIGH | USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW | USB_OTGCTL_OTGEN);
    } else {
        // Enable pull-up on D+ to indicate full speed
        IO_WRITE(USB_OTGCTL_REG(0), USB_OTGCTL_DPHIGH | USB_OTGCTL_OTGEN);
    }

    __osBbDelay(500);

    IO_WRITE(USB_OTGICR_REG(which), USB_OTGICR_ALL);

    // Enable USB0/USB1 interrupts to begin handling events
    IO_WRITE(MI_3C_REG, (which == 0) ? (1 << 21) : (1 << 23));
}

static void __usbOtgStateChange(s32 which) {
    u32 val = IO_READ(USB_OTGSTAT_REG_ALT(which));

#ifndef BB_SA_1041
    if (_usb_ctlr_state[which].ucs_mask == UCS_MASK_NONE) {
        UCS_MODE(which) = UCS_MODE_NONE;
        return;
    }
#endif

    if (which != 0) {
        // USB1 device mode only?
        if (UCS_DEVICE_OK(which) && UCS_MODE(which) != UCS_MODE_DEVICE) {
            UCS_MODE(which) = UCS_MODE_DEVICE;
            __usbDeviceMode(which);
        }
    } else {
        if ((val & USB_OTGSTAT_ID) == 0) {
            // Type A
            if (UCS_HOST_OK(which) && UCS_MODE(which) != UCS_MODE_HOST) {
                UCS_MODE(which) = UCS_MODE_HOST;
                __usbHostMode(which);
            }
        } else {
            // Type B
            if (UCS_DEVICE_OK(which) && UCS_MODE(which) != UCS_MODE_DEVICE) {
                UCS_MODE(which) = UCS_MODE_DEVICE;
                __usbDeviceMode(which);
            }
        }
    }
}

u32 osBbUsbGetResetCount(s32 which) {
    return __Usb_Reset_Count[which];
}

static void __usbCtlrInit(s32 which) {
    u32 addr = USB_BDT_BUF(which, 0);

    // Disable USB0/USB1 interrupts
    IO_WRITE(MI_3C_REG, (which == 0) ? (1 << 20) : (1 << 22));

    IO_WRITE(USB_CTL_REG(which), USB_CTL_USB_DISABLE);
    IO_WRITE(USB_OTGICR_REG(which), USB_OTGICR_NONE);
    IO_WRITE(USB_ERREN_REG(which), USB_ERREN_NONE);
    IO_WRITE(USB_INTEN_REG(which), USB_INTEN_NONE);
    IO_WRITE(USB_OTGISTAT_REG(which), USB_OTGISTAT_ALL);
    IO_WRITE(USB_ERRSTAT_REG(which), USB_ERRSTAT_ALL);
    IO_WRITE(USB_ISTAT_REG(which), USB_ISTAT_ALL);
    IO_WRITE(USB_CTL_REG(which), USB_CTL_ODD_RST | USB_CTL_USB_DISABLE);
    IO_WRITE(USB_ADDR_REG(which), USB_ADDR_FSEN | USB_ADDR_ADDR(0));
    IO_WRITE(USB_BDTPAGE1_REG(which), USB_BDTPAGE1_ADDR(addr));
    IO_WRITE(USB_BDTPAGE2_REG(which), USB_BDTPAGE2_ADDR(addr));
    IO_WRITE(USB_BDTPAGE3_REG(which), USB_BDTPAGE3_ADDR(addr));

    __usbOtgStateChange(which);

    __Usb_Reset_Count[which] = 0;
}

s32 __usbHwInit(void) {
    s32 i;

    // Enable access
    IO_WRITE(USB_ACCESS_REG(0), 1);
    IO_WRITE(USB_ACCESS_REG(1), 1);

    // Initialize both controllers
    for (i = 0; i < 2; i++) {
        __usbCtlrTest(i);
        __usbCtlrInit(i);
    }
    return i;
}
