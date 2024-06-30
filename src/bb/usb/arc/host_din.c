#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

void _usb_host_close_pipe(_usb_host_handle handle, int_16 pipeid) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    bzero(&usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipeid], sizeof(PIPE_DESCRIPTOR_STRUCT));

    __usb_splx();
}

void _usb_host_close_all_pipes(_usb_host_handle handle) {
    int_16 i;
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    for (i = 0; i < usb_host_ptr->MAX_PIPES; i++) {
        if (!usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[i].OPEN) {
            break;
        }
        _usb_host_unregister_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[i].PIPE_ID));
        bzero(&usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[i], sizeof(PIPE_DESCRIPTOR_STRUCT));
    }

    usb_host_ptr->CURRENT_PIPE_ID = -1;
    __usb_splx();
}
