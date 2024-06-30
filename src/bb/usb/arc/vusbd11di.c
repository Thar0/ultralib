#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// deinit the endpoint by resetting bits in the corresponding endpoint hardware register
void _usb_dci_vusb11_deinit_endpoint(_usb_device_handle handle, uint_8 endpoint_num, XD_STRUCT_PTR pxd) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    switch (pxd->TYPE) {
        case USB_ISOCHRONOUS_PIPE:
            pxd->CTL = 0;
            usb_dev_ptr->ENDPT_REGS[endpoint_num] &= ~(USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS);
            break;
        case USB_CONTROL_PIPE:
            pxd->CTL = 0;
            usb_dev_ptr->ENDPT_REGS[endpoint_num] &= ~(USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_RETRY_DIS);
            break;
        case USB_INTERRUPT_PIPE:
        case USB_BULK_PIPE:
            pxd->CTL = 0;
            usb_dev_ptr->ENDPT_REGS[endpoint_num] &= ~(USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS);
            break;
    }
}
