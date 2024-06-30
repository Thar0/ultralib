#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

void _usb_device_shutdown(_usb_device_handle handle) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    // Shutdown hardware layer
    _usb_dci_vusb11_shutdown(handle);

    // Free service buffer and transfer descriptor buffers
    osFree(__usb_svc_callback_reg, usb_dev_ptr->SERVICE_HEAD_PTR);
    osFree(__usb_endpt_desc_reg, usb_dev_ptr->XDSEND);
    osFree(__usb_endpt_desc_reg, usb_dev_ptr->XDRECV);
}
