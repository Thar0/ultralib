#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

uint_8 _usb_host_cancel_transfer(_usb_host_handle handle, int_16 pipeid) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
    uint_8 status;

    if (pipeid < usb_host_ptr->MAX_PIPES) {
        pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipeid];
        status = pipe_descr_ptr->STATUS;

        _usb_hci_vusb11_cancel_transfer(usb_host_ptr, pipe_descr_ptr);

        pipe_descr_ptr->PACKETPENDING = FALSE;
        pipe_descr_ptr->STATUS = 0;

        _usb_host_update_current_head(usb_host_ptr, pipe_descr_ptr->PIPETYPE);
        return status;
    }
    return USB_HOST_ERR_BAD_PIPE_NUM;
}
