#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

USB_DEV_STATE_STRUCT _usb_device_state[USB_MAX_DEVICES];

uint_8 _usb_device_init(uint_8 controller_number, uint_8 device_number, _usb_device_handle* handlep, uint_8 number_of_endpoints) {
    uint_8 returncode;
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr;

    if (number_of_endpoints == 0 || number_of_endpoints > USB_NUM_ENDPOINTS) {
        return USB_DEV_ERR_BAD_ENDPOINT_NUM;
    }
    if (device_number >= USB_MAX_DEVICES) {
        return USB_DEV_ERR_BAD_DEVICE_NUM;
    }

    usb_dev_ptr = &_usb_device_state[controller_number];
    bzero(usb_dev_ptr, sizeof(USB_DEV_STATE_STRUCT));

    usb_dev_ptr->MAX_ENDPOINTS = number_of_endpoints;
    usb_dev_ptr->CTLR_NUM = controller_number;
    usb_dev_ptr->DEV_NUM = device_number;

    // Allocate the transfer descriptor buffers
    if ((usb_dev_ptr->XDSEND = osMalloc(__usb_endpt_desc_reg)) == NULL ||
        (usb_dev_ptr->XDRECV = osMalloc(__usb_endpt_desc_reg)) == NULL) {
        return USB_DEV_ERR_ALLOC_XD;
    }

    usb_dev_ptr->USB_STATE = USB_SELF_POWERED | USB_REMOTE_WAKEUP;
    *handlep = usb_dev_ptr;

    // Initialize the hardware layer
    returncode = _usb_dci_vusb11_init(usb_dev_ptr);

    if (returncode != USB_DEV_ERR_OK) {
        // Failed, free buffers
        osFree(__usb_endpt_desc_reg, usb_dev_ptr->XDSEND);
        osFree(__usb_endpt_desc_reg, usb_dev_ptr->XDRECV);
        return returncode;
    }

    return USB_DEV_ERR_OK;
}

uint_8 _usb_device_register_service(_usb_device_handle handle, uint_8 event_endpoint, void (*service)(_usb_device_handle, boolean, uint_8, uint_8_ptr, uint_32)) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    SERVICE_STRUCT_PTR service_ptr;
    SERVICE_STRUCT_PTR* search_ptr;

    __usb_splhigh();

    search_ptr = &usb_dev_ptr->SERVICE_HEAD_PTR;
    while (*search_ptr != NULL) {
        if ((*search_ptr)->TYPE == event_endpoint) {
            __usb_splx();
            return USB_DEV_ERR_SERVICE_EXISTS;
        }
        search_ptr = &(*search_ptr)->NEXT;
    }

    service_ptr = osMalloc(__usb_svc_callback_reg);
    if (service_ptr == NULL) {
        __usb_splx();
        return USB_DEV_ERR_ALLOC_SERVICE;
    }

    service_ptr->TYPE = event_endpoint;
    service_ptr->SERVICE = service;
    service_ptr->NEXT = NULL;
    *search_ptr = service_ptr;

    __usb_splx();
    return USB_DEV_ERR_OK;
}

uint_8 _usb_device_init_endpoint(_usb_device_handle handle, uint_8 endpoint_num, uint_16 max_packet_size, uint_8 direction, uint_8 endpoint_type, uint_8 flag) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;

    __usb_splhigh();

    if (direction != USB_RECV) {
        pxd = &usb_dev_ptr->XDSEND[endpoint_num];
    } else {
        pxd = &usb_dev_ptr->XDRECV[endpoint_num];
    }

    pxd->MAXPACKET = max_packet_size;
    pxd->TYPE = endpoint_type;
    pxd->STATUS = USB_STATUS_IDLE;
    pxd->DONT_ZERO_TERMINATE = flag;
    pxd->NEXTDATA01 = USB_BD_DATA0;

    _usb_dci_vusb11_init_endpoint(usb_dev_ptr, endpoint_num, pxd);

    __usb_splx();
    return USB_DEV_ERR_OK;
}

uint_8 _usb_device_get_transfer_status(_usb_device_handle handle, uint_8 endpoint_number, uint_8 direction) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    uint_32 status;

    __usb_splhigh();

    if (direction == USB_SEND) {
        status = usb_dev_ptr->XDSEND[endpoint_number].STATUS;
    } else {
        status = usb_dev_ptr->XDRECV[endpoint_number].STATUS;
    }
    __usb_splx();
    return status;
}

void _usb_device_read_setup_data(_usb_device_handle handle, uint_8 endpoint_num, uchar_ptr buff_ptr) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd = &usb_dev_ptr->XDRECV[endpoint_num];

    if (buff_ptr != NULL) {
        // Copy the control setup from the DMA buffer to the destination
        uchar_ptr from = (uchar_ptr)pxd->STARTADDRESS + ((pxd->SOFAR != 0) ? (pxd->SOFAR - sizeof(SETUP_STRUCT)) : 0);
        bcopy(from, buff_ptr, sizeof(SETUP_STRUCT));
    }
}

uint_8 _usb_device_recv_data(_usb_device_handle handle, uint_8 endpoint_number, uchar_ptr buff_ptr, uint_32 size) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;
    uint_8 status;

    __usb_splhigh();

    pxd = &usb_dev_ptr->XDRECV[endpoint_number];
    if (pxd->STATUS == USB_STATUS_DISABLED) {
        __usb_splx();
        return USB_DEV_ERR_ENDPT_DISABLED;
    }

    if (pxd->TYPE == USB_CONTROL_PIPE && pxd->SETUP_BUFFER_QUEUED) {
        // If we're on a control pipe and a setup buffer is queued on this endpoint,
        // cancel any existing receive transfer
        pxd->SETUP_BUFFER_QUEUED = FALSE;
        usb_dev_ptr->XDSEND[endpoint_number].SETUP_BUFFER_QUEUED = FALSE;
        if (size == 0) {
            __usb_splx();
            return USB_DEV_ERR_OK;
        }
        _usb_device_cancel_transfer(usb_dev_ptr, endpoint_number, USB_RECV);
    }

    status = pxd->STATUS;
    if (status == USB_STATUS_TRANSFER_PENDING || status == USB_STATUS_TRANSFER_IN_PROGRESS) {
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
    pxd->DIRECTION = USB_RECV;
    osInvalDCache(buff_ptr, size);
    pxd->STATUS = USB_STATUS_TRANSFER_PENDING;
    _usb_dci_vusb11_submit_transfer(usb_dev_ptr, USB_RECV, endpoint_number);

    if (pxd->STATUS == USB_STATUS_STALLED) {
        __usb_splx();
        return USB_DEV_ERR_ENDPT_STALLED;
    }
    __usb_splx();
    return USB_DEV_ERR_OK;
}

uint_8 _usb_device_call_service(_usb_device_handle handle, uint_8 type, boolean setup, boolean direction, uint_8_ptr buffer_ptr, uint_32 length) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    SERVICE_STRUCT_PTR* search_ptr;

    __usb_splhigh();

    search_ptr = &usb_dev_ptr->SERVICE_HEAD_PTR;
    while (*search_ptr != NULL) {
        if ((*search_ptr)->TYPE == type) {
            (*search_ptr)->SERVICE(handle, setup, direction, buffer_ptr, length);
            __usb_splx();
            return USB_DEV_ERR_OK;
        }
        search_ptr = &(*search_ptr)->NEXT;
    }

    __usb_splx();
    return USB_DEV_ERR_SERVICE_NOT_FOUND;
}
