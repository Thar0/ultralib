#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// Unstall a particular endpoint
void _usb_device_unstall_endpoint(_usb_device_handle handle, uint_8 endpoint_number, uint_8 direction) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    _usb_dci_vusb11_unstall_endpoint(handle, endpoint_number);
    usb_dev_ptr->XDSEND[endpoint_number].STATUS = USB_STATUS_IDLE;
    usb_dev_ptr->XDRECV[endpoint_number].STATUS = USB_STATUS_IDLE;

    __usb_splx();
}

uint_8 _usb_device_get_status(_usb_device_handle handle, uint_8 component, uint_16_ptr status) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    switch (component) {
        case USB_STATUS_DEVICE_STATE:
            *status = usb_dev_ptr->USB_STATE;
            break;
        case USB_STATUS_DEVICE:
            *status = usb_dev_ptr->USB_DEVICE_STATE;
            break;
        case USB_STATUS_INTERFACE:
            break;
        case USB_STATUS_ADDRESS:
            *status = usb_dev_ptr->USB->ADDRESS;
            break;
        case USB_STATUS_CURRENT_CONFIG:
            *status = usb_dev_ptr->USB_CURR_CONFIG;
            break;
        case USB_STATUS_SOF_COUNT:
            *status = usb_dev_ptr->USB_SOF_COUNT;
            break;
        default:
            if (component & USB_STATUS_ENDPOINT) {
                *status = _usb_dci_vusb11_get_endpoint_status(handle, component & USB_STATUS_ENDPOINT_NUMBER_MASK);
                break;
            }
            __usb_splx();
            return USB_DEV_ERR_INVALID_COMPONENT;
    }
    __usb_splx();
    return USB_DEV_ERR_OK;
}

uint_8 _usb_device_set_status(_usb_device_handle handle, uint_8 component, uint_16 setting) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    switch (component) {
        case USB_STATUS_DEVICE_STATE:
            usb_dev_ptr->USB_STATE = setting;
            break;
        case USB_STATUS_DEVICE:
            usb_dev_ptr->USB_DEVICE_STATE = setting;
            break;
        case USB_STATUS_INTERFACE:
            break;
        case USB_STATUS_ADDRESS:
            usb_dev_ptr->USB->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(setting);
            break;
        case USB_STATUS_CURRENT_CONFIG:
            usb_dev_ptr->USB_CURR_CONFIG = setting;
            break;
        case USB_STATUS_SOF_COUNT:
            usb_dev_ptr->USB_SOF_COUNT = setting;
            break;
        default:
            if (component & USB_STATUS_ENDPOINT) {
                _usb_dci_vusb11_set_endpoint_status(handle, component & USB_STATUS_ENDPOINT_NUMBER_MASK, setting);
                break;
            }
            __usb_splx();
            return USB_DEV_ERR_INVALID_COMPONENT;
    }
    __usb_splx();
    return USB_DEV_ERR_OK;
}

// Stall the supplied endpoint
void _usb_device_stall_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_8 direction) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    usb_dev_ptr->XDSEND[endpoint_num].STATUS = USB_STATUS_DEVICE;
    usb_dev_ptr->XDRECV[endpoint_num].STATUS = USB_STATUS_DEVICE;
    _usb_device_set_status(handle, USB_STATUS_ENDPOINT | endpoint_num, TRUE);
}
