#include "PR/os_internal.h"
#include "PR/rcp.h"
#include "PR/region.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

USB_HOST_STATE_STRUCT _usb_host_state[2];
PIPE_DESCRIPTOR_STRUCT _usb_host_state_pipes[2][16];

uint_8 _usb_host_init(uint_8 controller_number, uint_8 device_number, uint_8 max_pipe_num, _usb_host_handle* handlep) {
    USB_HOST_STATE_STRUCT_PTR usb_host_state_struct_ptr;

    if (controller_number < 2 && device_number < 4) {
        usb_host_state_struct_ptr = &_usb_host_state[controller_number];
        bzero(usb_host_state_struct_ptr, sizeof(USB_HOST_STATE_STRUCT));
        usb_host_state_struct_ptr->MAX_PIPES = max_pipe_num;
        usb_host_state_struct_ptr->CTLR_NUM = controller_number;
        usb_host_state_struct_ptr->DEV_NUM = device_number;
        usb_host_state_struct_ptr->PIPE_DESCRIPTOR_BASE_PTR = _usb_host_state_pipes[controller_number];
        bzero(_usb_host_state_pipes[controller_number], max_pipe_num * sizeof(PIPE_DESCRIPTOR_STRUCT));
        usb_host_state_struct_ptr->CURRENT_ISO_HEAD = -1;
        usb_host_state_struct_ptr->CURRENT_ISO_TAIL = -1;
        usb_host_state_struct_ptr->CURRENT_INTR_HEAD = -1;
        usb_host_state_struct_ptr->CURRENT_INTR_TAIL = -1;
        usb_host_state_struct_ptr->CURRENT_CTRL_HEAD = -1;
        usb_host_state_struct_ptr->CURRENT_CTRL_TAIL = -1;
        usb_host_state_struct_ptr->CURRENT_BULK_HEAD = -1;
        usb_host_state_struct_ptr->CURRENT_BULK_TAIL = -1;
        *handlep = (_usb_host_handle)usb_host_state_struct_ptr;
        _usb_hci_vusb11_init(usb_host_state_struct_ptr);
        return USB_DEV_ERR_OK;
    }
    return USB_DEV_ERR_BAD_DEVICE_NUM;
}

uint_8 _usb_host_register_service(_usb_host_handle handle, uint_8 type, void (*service)(_usb_host_handle handle, uint_32 length)) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    USB_SERVICE_STRUCT_PTR service_ptr;
    USB_SERVICE_STRUCT_PTR* search_ptr;

    __usb_splhigh();

    search_ptr = &usb_host_ptr->SERVICE_HEAD_PTR;
    while ((*search_ptr) != NULL) {
        if ((*search_ptr)->TYPE == type) {
            __usb_splx();
            return USB_DEV_ERR_SERVICE_EXISTS;
        }
        search_ptr = &(*search_ptr)->NEXT;
    }

    service_ptr = osMalloc(__usb_svc_callback_reg);
    if (service_ptr == NULL) {
        __usb_splx();
        return USB_HOST_ERR_ALLOC_SERVICE;
    }

    service_ptr->TYPE = type;
    service_ptr->SERVICE = service;
    service_ptr->NEXT = NULL;
    *search_ptr = service_ptr;
    __usb_splx();
    return USB_DEV_ERR_OK;
}

uint_8 _usb_host_call_service(_usb_host_handle handle, uint_8 type, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    USB_SERVICE_STRUCT_PTR service_ptr;

    __usb_splhigh();

    service_ptr = usb_host_ptr->SERVICE_HEAD_PTR;
    while (service_ptr != NULL) {
        if (service_ptr->TYPE == type) {
            service_ptr->SERVICE(handle, length);
            __usb_splx();
            return USB_DEV_ERR_OK;
        }
        service_ptr = service_ptr->NEXT;
    }

    __usb_splx();
    return USB_DEV_ERR_SERVICE_NOT_FOUND;
}

int_16 _usb_host_open_pipe(_usb_host_handle handle, uint_8 address, uint_8 ep_num, uint_8 direction, uint_8 interval, uint_8 pipe_type, uint_16 max_pkt_size, uint_8 nak_count, uint_8 flag) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    int_16 pipeid = -0x8000;
    int_16 i;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;

    __usb_splhigh();

    for (i = 0; i < usb_host_ptr->MAX_PIPES; i++) {
        pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[i];
        if (!pipe_descr_ptr->OPEN) {
            break;
        }
    }

    __usb_splx();

    if (i != usb_host_ptr->MAX_PIPES) {
        pipeid = i;
        pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipeid];
        pipe_descr_ptr->ADDRESS = address;
        pipe_descr_ptr->EP = ep_num;
        pipe_descr_ptr->DIRECTION = direction;
        pipe_descr_ptr->CURRENT_INTERVAL = interval;
        pipe_descr_ptr->INTERVAL = interval;
        pipe_descr_ptr->CURRENT_NAK_COUNT = nak_count;
        pipe_descr_ptr->NAK_COUNT = nak_count;
        pipe_descr_ptr->PIPETYPE = pipe_type;
        pipe_descr_ptr->MAX_PKT_SIZE = max_pkt_size;
        pipe_descr_ptr->OPEN = TRUE;
        pipe_descr_ptr->PIPE_ID = pipeid;
        pipe_descr_ptr->STATUS = 0;
        pipe_descr_ptr->DONT_ZERO_TERMINATE = flag;
        return pipeid;
    }
    return USB_HOST_ERR_111;
}

uint_8 _usb_host_send_setup(_usb_host_handle handle, int_16 pipeid, uchar_ptr tx1_ptr, uchar_ptr tx2_ptr, uchar_ptr rx_ptr, uint_32 rx_length, uint_32 length2) {
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
    pipe_descr_ptr->NEXTDATA01 = USB_BD_DATA0;
    pipe_descr_ptr->RX_PTR = rx_ptr;
    pipe_descr_ptr->TX1_PTR = tx1_ptr;
    pipe_descr_ptr->TX2_PTR = tx2_ptr;
    pipe_descr_ptr->TODO1 = rx_length;
    pipe_descr_ptr->TODO2 = length2;
    pipe_descr_ptr->SEND_PHASE = FALSE;
    pipe_descr_ptr->FIRST_PHASE = TRUE;
    osWritebackDCache(tx1_ptr, sizeof(SETUP_STRUCT));

    if (tx2_ptr != NULL) {
        osWritebackDCache(tx2_ptr, length2);
    }
    if (rx_ptr != NULL) {
        osInvalDCache(rx_ptr, rx_length);
    }
    if (!(((SETUP_STRUCT_PTR)pipe_descr_ptr->TX1_PTR)->REQUESTTYPE & USB_REQ_DIR_IN)) {
        pipe_descr_ptr->SEND_PHASE = TRUE;
    }

    __usb_splhigh();
    pipe_descr_ptr->STATUS = 7;
    _usb_hci_vusb11_send_setup(usb_host_ptr, pipe_descr_ptr);
    pipe_descr_ptr->PACKETPENDING = TRUE;
    __usb_splx();
    return USB_HOST_ERR_7;
}

uint_8 _usb_host_get_transfer_status(_usb_host_handle handle, int_16 pipeid) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    if (pipeid < usb_host_ptr->MAX_PIPES) {
        return usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipeid].STATUS;
    }
    return USB_HOST_ERR_BAD_PIPE_NUM;
}

void _usb_host_queue_pkts(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    int_16 pipe_head;
    int_16 pipe_tail;

    __usb_splhigh();

    pipe_descr_ptr->NEXT_PIPE = -1;

    if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
        pipe_head = usb_host_ptr->CURRENT_ISO_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_ISO_TAIL;
    } else if (pipe_descr_ptr->PIPETYPE == USB_INTERRUPT_PIPE) {
        pipe_head = usb_host_ptr->CURRENT_INTR_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_INTR_TAIL;
    } else if (pipe_descr_ptr->PIPETYPE == USB_CONTROL_PIPE) {
        pipe_head = usb_host_ptr->CURRENT_CTRL_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_CTRL_TAIL;
    } else if (pipe_descr_ptr->PIPETYPE == USB_BULK_PIPE) {
        pipe_head = usb_host_ptr->CURRENT_BULK_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_BULK_TAIL;
    }

    if (pipe_head == -1) {
        pipe_head = pipe_descr_ptr->PIPE_ID;
    } else {
        if (pipe_tail != -1) {
            usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipe_tail].NEXT_PIPE = pipe_descr_ptr->PIPE_ID;
        } else {
            usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipe_head].NEXT_PIPE = pipe_descr_ptr->PIPE_ID;
        }
        pipe_tail = pipe_descr_ptr->PIPE_ID;
    }

    if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
        usb_host_ptr->CURRENT_ISO_HEAD = pipe_head;
        usb_host_ptr->CURRENT_ISO_TAIL = pipe_tail;
    } else if (pipe_descr_ptr->PIPETYPE == USB_INTERRUPT_PIPE) {
        usb_host_ptr->CURRENT_INTR_HEAD = pipe_head;
        usb_host_ptr->CURRENT_INTR_TAIL = pipe_tail;
    } else if (pipe_descr_ptr->PIPETYPE == USB_CONTROL_PIPE) {
        usb_host_ptr->CURRENT_CTRL_HEAD = pipe_head;
        usb_host_ptr->CURRENT_CTRL_TAIL = pipe_tail;
    } else if (pipe_descr_ptr->PIPETYPE == USB_BULK_PIPE) {
        usb_host_ptr->CURRENT_BULK_HEAD = pipe_head;
        usb_host_ptr->CURRENT_BULK_TAIL = pipe_tail;
    }

    __usb_splx();
}

void _usb_host_update_current_head(_usb_host_handle handle, uint_8 pipetype) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
    int_16 pipe_head;
    int_16 pipe_tail;

    __usb_splhigh();

    pipe_descr_ptr = NULL;

    if (pipetype == USB_ISOCHRONOUS_PIPE) {
        if (usb_host_ptr->CURRENT_ISO_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_ISO_HEAD];
        }
        pipe_head = usb_host_ptr->CURRENT_ISO_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_ISO_TAIL;
    } else if (pipetype == USB_INTERRUPT_PIPE) {
        if (usb_host_ptr->CURRENT_INTR_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_INTR_HEAD];
        }
        pipe_head = usb_host_ptr->CURRENT_INTR_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_INTR_TAIL;
    } else if (pipetype == USB_CONTROL_PIPE) {
        if (usb_host_ptr->CURRENT_CTRL_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_CTRL_HEAD];
        }
        pipe_head = usb_host_ptr->CURRENT_CTRL_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_CTRL_TAIL;
    } else if (pipetype == USB_BULK_PIPE) {
        if (usb_host_ptr->CURRENT_BULK_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_BULK_HEAD];
        }
        pipe_head = usb_host_ptr->CURRENT_BULK_HEAD;
        pipe_tail = usb_host_ptr->CURRENT_BULK_TAIL;
    }

    if (pipe_descr_ptr != NULL) {
        if (pipe_descr_ptr->NEXT_PIPE != -1) {
            if (pipe_descr_ptr->STATUS != 0 && pipe_tail != -1) {
                usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipe_tail].NEXT_PIPE = pipe_head;
                pipe_tail = pipe_head;
            }
            pipe_head = pipe_descr_ptr->NEXT_PIPE;
            if (pipe_tail != -1) {
                usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipe_tail].NEXT_PIPE = -1;
            }
        } else if (pipe_descr_ptr->STATUS == 0) {
            pipe_head = -1;
            pipe_tail = -1;
        }
    }

    if (pipetype == USB_ISOCHRONOUS_PIPE) {
        usb_host_ptr->CURRENT_ISO_HEAD = pipe_head;
        usb_host_ptr->CURRENT_ISO_TAIL = pipe_tail;
    } else if (pipetype == USB_INTERRUPT_PIPE) {
        usb_host_ptr->CURRENT_INTR_HEAD = pipe_head;
        usb_host_ptr->CURRENT_INTR_TAIL = pipe_tail;
    } else if (pipetype == USB_CONTROL_PIPE) {
        usb_host_ptr->CURRENT_CTRL_HEAD = pipe_head;
        usb_host_ptr->CURRENT_CTRL_TAIL = pipe_tail;
    } else if (pipetype == USB_BULK_PIPE) {
        usb_host_ptr->CURRENT_BULK_HEAD = pipe_head;
        usb_host_ptr->CURRENT_BULK_TAIL = pipe_tail;
    }

    __usb_splx();
}

void _usb_host_update_current_interval_on_queue(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    while (TRUE) {
        if (pipe_descr_ptr->CURRENT_INTERVAL != 0) {
            pipe_descr_ptr->CURRENT_INTERVAL--;
            if (pipe_descr_ptr->CURRENT_INTERVAL == 0 && pipe_descr_ptr->PIPETYPE != USB_INTERRUPT_PIPE) {
                pipe_descr_ptr->CURRENT_NAK_COUNT = pipe_descr_ptr->NAK_COUNT;
            }
        }
        if (pipe_descr_ptr->NEXT_PIPE == -1) {
            break;
        }
        pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[pipe_descr_ptr->NEXT_PIPE];
    }
}

void _usb_host_update_interval_for_pipes(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    __usb_splhigh();

    if (usb_host_ptr->CURRENT_INTR_HEAD != -1) {
        _usb_host_update_current_interval_on_queue(usb_host_ptr, &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_INTR_HEAD]);
    }
    if (usb_host_ptr->CURRENT_CTRL_HEAD != -1) {
        _usb_host_update_current_interval_on_queue(usb_host_ptr, &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_CTRL_HEAD]);
    }
    if (usb_host_ptr->CURRENT_BULK_HEAD != -1) {
        _usb_host_update_current_interval_on_queue(usb_host_ptr, &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_BULK_HEAD]);
    }

    __usb_splx();
}
