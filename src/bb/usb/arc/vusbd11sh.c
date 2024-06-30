#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

void _usb_dci_vusb11_shutdown(_usb_device_handle handle) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    uint_8 i;

    // Disable controller and interrupts
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_DISABLE;
    usb_dev_ptr->USB->INTENABLE = USB_INTEN_NONE;

    // Disable all endpoints
    for (i = 0; i < usb_dev_ptr->MAX_ENDPOINTS; i++) {
        usb_dev_ptr->ENDPT_REGS[i] = 0;
    }
}
