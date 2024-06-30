#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

void _usb_dci_vusb11_unstall_endpoint(_usb_device_handle handle, uint_8 ep) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    // Unstall descriptors
    usb_dev_ptr->XDSEND[ep].CTL &= ~USB_ENDPT_STALLED;
    usb_dev_ptr->XDRECV[ep].CTL &= ~USB_ENDPT_STALLED;

    // Cancel any ongoing transfers
    _usb_dci_vusb11_cancel_transfer(handle, USB_SEND, ep);
    _usb_dci_vusb11_cancel_transfer(handle, USB_RECV, ep);

    // Unstall hardware
    usb_dev_ptr->ENDPT_REGS[ep] &= ~USB_ENDPT_STALLED;
}

// Cancel a USB transfer
void _usb_dci_vusb11_cancel_transfer(_usb_device_handle handle, uint_8 direction, uint_8 ep) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;
    BDT_STRUCT_PTR BDT_PTR;
    BDT_STRUCT copy_bdt;

    if (direction == USB_SEND) {
        pxd = &usb_dev_ptr->XDSEND[ep];
    } else {
        pxd = &usb_dev_ptr->XDRECV[ep];
    }

    if (pxd->PACKETPENDING == 1) {
        // If 1 pending, cancel 1 BD
        BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[ep][direction][pxd->NEXTODDEVEN ^ 1];
        _usb_bdt_copy_swab_device(BDT_PTR, &copy_bdt);
        if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
            // It's owned by the SIE, but trash it anyway?
            copy_bdt.REGISTER.BDTCTL = 0;
            copy_bdt.BC = 0;
            copy_bdt.ADDRESS = NULL;
            pxd->PACKETPENDING--;
            _usb_bdt_copy_swab_device(&copy_bdt, BDT_PTR);
            pxd->NEXTODDEVEN ^= 1;
        }
    }

    if (pxd->PACKETPENDING == 2) {
        // If 2 pending, cancel 2 BDs
        BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[ep][direction][pxd->NEXTODDEVEN];
        _usb_bdt_copy_swab_device(BDT_PTR, &copy_bdt);
        if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
            // It's owned by the SIE, but trash it anyway?
            copy_bdt.REGISTER.BDTCTL = 0;
            copy_bdt.BC = 0;
            copy_bdt.ADDRESS = NULL;
            pxd->PACKETPENDING--;
            _usb_bdt_copy_swab_device(&copy_bdt, BDT_PTR);
        }

        BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[ep][direction][pxd->NEXTODDEVEN ^ 1];
        _usb_bdt_copy_swab_device(BDT_PTR, &copy_bdt);
        if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
            // It's owned by the SIE, but trash it anyway?
            copy_bdt.REGISTER.BDTCTL = 0;
            copy_bdt.BC = 0;
            copy_bdt.ADDRESS = NULL;
            pxd->PACKETPENDING--;
            _usb_bdt_copy_swab_device(&copy_bdt, BDT_PTR);
        }
    }

    pxd->BDTCTL = (uint_8)(~USB_BD_OWN);
    if (pxd->CTL & USB_ENDPT_STALLED) {
        usb_dev_ptr->XDRECV[ep].STATUS = USB_STATUS_STALLED;
    } else {
        pxd->STATUS = USB_STATUS_IDLE;
    }
}
