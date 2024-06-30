#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// Get status on whether a particular endpoint is stalled or not
uint_8 _usb_dci_vusb11_get_endpoint_status(_usb_device_handle handle, uint_8 ep) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    return (usb_dev_ptr->ENDPT_REGS[ep] & USB_ENDPT_STALLED) != 0;
}

// Stall or unstall an endpoint
void _usb_dci_vusb11_set_endpoint_status(_usb_device_handle handle, uint_8 ep, uint_8 stall) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    if (stall) {
        usb_dev_ptr->XDSEND[ep].CTL |= USB_ENDPT_STALLED;
        usb_dev_ptr->XDRECV[ep].CTL |= USB_ENDPT_STALLED;
        usb_dev_ptr->ENDPT_REGS[ep] |= USB_ENDPT_STALLED;
    } else {
        usb_dev_ptr->XDSEND[ep].CTL &= ~USB_ENDPT_STALLED;
        usb_dev_ptr->XDRECV[ep].CTL &= ~USB_ENDPT_STALLED;
        usb_dev_ptr->ENDPT_REGS[ep] &= ~USB_ENDPT_STALLED;
    }
}
