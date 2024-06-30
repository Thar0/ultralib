#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

void _usb_host_shutdown(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    _usb_hci_vusb11_shutdown(usb_host_ptr);
}

void _usb_host_bus_control(_usb_host_handle handle, uint_8 bcontrol) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    _usb_hci_vusb11_bus_control(usb_host_ptr, bcontrol);
}
