#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"

void _usb_hci_vusb11_bus_control(_usb_host_handle handle, uint_8 bControl) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    switch (bControl) {
        case 1: // Bus Reset
            usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_RESET | USB_CTL_HOST_MODE_EN | USB_CTL_USB_EN;
            break;
        case 3: // Resume signaling
            usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_RESUME | USB_CTL_USB_EN;
            break;
        case 5: // Suspend?
            usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
            usb_host_ptr->DEV_PTR->INTENABLE |= USB_INTEN_RESUMEEN;
            break;
        case 2: // Resume from suspend?
        case 4: //
        case 6: //
            usb_host_ptr->DEV_PTR->INTENABLE &= ~USB_INTEN_RESUMEEN;
            usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_EN;
            break;
    }
}

void _usb_hci_vusb11_shutdown(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->ENDPT_HOST_RG[0] = 0;
    // Disable the USB
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_USB_DISABLE;
    // Disable and ack all interrupts
    usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_NONE;
    usb_host_ptr->DEV_PTR->INTSTATUS = USB_ISTAT_ALL;
    // Reset address
    usb_host_ptr->DEV_PTR->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(0);
}
