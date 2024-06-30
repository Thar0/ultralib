#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

uint_8 _usb_device_unregister_service(_usb_device_handle handle, uint_8 event_endpoint) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    SERVICE_STRUCT_PTR service_ptr;
    SERVICE_STRUCT_PTR* search_ptr;

    __usb_splhigh();

    search_ptr = &usb_dev_ptr->SERVICE_HEAD_PTR;
    while (*search_ptr != NULL) {
        if ((*search_ptr)->TYPE == event_endpoint) {
            break;
        }
        search_ptr = &(*search_ptr)->NEXT;
    }

    if ((*search_ptr) == NULL) {
        __usb_splx();
        return USB_DEV_ERR_SERVICE_NOT_FOUND;
    }

    service_ptr = *search_ptr;
    *search_ptr = service_ptr->NEXT;

    __usb_splx();
    osFree(__usb_svc_callback_reg, service_ptr);
    return USB_DEV_ERR_OK;
}
