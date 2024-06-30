#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// Cancel a USB transfer
uint_8 _usb_device_cancel_transfer(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction) {
    _usb_dci_vusb11_cancel_transfer(handle, direction, endpoint_num);
    return USB_DEV_ERR_OK;
}
