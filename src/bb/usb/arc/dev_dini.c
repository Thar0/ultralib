#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// Deinit the endpoint for a particular direction
uint_8 _usb_device_deinit_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;

    __usb_splhigh();

    if (direction != USB_RECV) {
        pxd = &usb_dev_ptr->XDSEND[endpoint_num];
    } else {
        pxd = &usb_dev_ptr->XDRECV[endpoint_num];
    }

    pxd->STATUS = USB_STATUS_IDLE;
    _usb_dci_vusb11_deinit_endpoint(handle, endpoint_num, pxd);

    __usb_splx();

    return USB_DEV_ERR_OK;
}
