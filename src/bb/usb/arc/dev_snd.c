#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

uint_8 _usb_device_send_data(_usb_device_handle handle, uint_8 endpoint_number, uchar_ptr buff_ptr, uint_32 size) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;

    __usb_splhigh();

    pxd = &usb_dev_ptr->XDSEND[endpoint_number];

    if (pxd->STATUS == USB_STATUS_DISABLED) {
        __usb_splx();
        return USB_DEV_ERR_ENDPT_DISABLED;
    }
    if (pxd->STATUS == USB_STATUS_TRANSFER_IN_PROGRESS) {
        __usb_splx();
        return USB_DEV_ERR_XFER_IN_PROGRESS;
    }

    if (pxd->TYPE == USB_ISOCHRONOUS_PIPE) {
        // Isochronous transfers must fit in 1 packet
        if (size > pxd->MAXPACKET) {
            size = pxd->MAXPACKET;
        }
    }

    // Submit the transfer
    pxd->STARTADDRESS = buff_ptr;
    pxd->NEXTADDRESS = buff_ptr;
    pxd->TODO = size;
    pxd->SOFAR = 0;
    pxd->UNACKNOWLEDGEDBYTES = size;
    pxd->DIRECTION = USB_SEND;
    osWritebackDCache(buff_ptr, size);
    pxd->STATUS = USB_STATUS_TRANSFER_PENDING;
    _usb_dci_vusb11_submit_transfer(handle, USB_SEND, endpoint_number);

    if (pxd->STATUS == USB_STATUS_STALLED) {
        __usb_splx();
        return USB_DEV_ERR_ENDPT_STALLED;
    }

    __usb_splx();
    return USB_DEV_ERR_OK;
}
