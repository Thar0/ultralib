#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

uint_8 _usb_host_send_data(_usb_host_handle handle, int_16 pipeid, uchar_ptr buff_ptr, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;

    if (pipeid >= usb_host_ptr->MAX_PIPES) {
        return USB_HOST_ERR_BAD_PIPE_NUM;
    }

    pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipeid];

    __usb_splhigh();
    if (pipe_descr_ptr->STATUS != 0) {
        __usb_splx();
        return USB_HOST_ERR_3;
    }
    __usb_splx();

    pipe_descr_ptr->SOFAR = 0;
    pipe_descr_ptr->TX1_PTR = buff_ptr;
    if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE && pipe_descr_ptr->MAX_PKT_SIZE < pipe_descr_ptr->TODO1) {
        pipe_descr_ptr->TODO1 = pipe_descr_ptr->MAX_PKT_SIZE;
    } else {
        pipe_descr_ptr->TODO1 = length;
    }

    __usb_splhigh();

    pipe_descr_ptr->STATUS = 7;
    _usb_hci_vusb11_send_data(usb_host_ptr, pipe_descr_ptr);
    pipe_descr_ptr->PACKETPENDING = TRUE;

    __usb_splx();
    return USB_HOST_ERR_7;
}
