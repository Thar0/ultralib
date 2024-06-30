#include "PR/os_internal.h"
#include "PR/bcp.h"

#include "macros.h"
#include "usb.h"
#ident "$Revision: 1.1 $"

extern USB_SETUP tx_get_status;
extern USB_SETUP tx_clear_feature;
extern USB_SETUP tx_set_feature;
extern USB_SETUP tx_set_address;
extern USB_SETUP tx_get_desc;
extern USB_SETUP tx_get_config_desc;
extern USB_SETUP tx_set_desc;
extern USB_SETUP tx_get_config;
extern USB_SETUP tx_set_config;
extern USB_SETUP tx_get_interface;
extern USB_SETUP tx_set_interface;
extern USB_SETUP tx_synch_frame;
extern USB_SETUP tx_enable_echo;

HOST_GLOBAL_STRUCT host_global_struct[2];

static usb_host_driver* host_driver[4];
static u16 _usb_ucode_str[64];

void _usb_host_close_all_pipes(_usb_host_handle handle);
void __usbInvalidateHandles(uint_8);

static void host_ch9SetAddress(_usb_host_handle handle, int_16 pipe_id, USB_SETUP_PTR setup_ptr, uchar_ptr buff_ptr) {
    _usb_host_send_setup(handle, pipe_id, (uchar_ptr)setup_ptr, NULL, buff_ptr, BSWAP16(setup_ptr->WLENGTH), 0);
}

static void host_ch9GetDescription(_usb_host_handle handle, int_16 pipe_id, USB_SETUP_PTR setup_ptr, uchar_ptr buff_ptr) {
    _usb_host_send_setup(handle, pipe_id, (uchar_ptr)setup_ptr, NULL, buff_ptr, BSWAP16(setup_ptr->WLENGTH), 0);
}

static void host_ch9SetConfig(_usb_host_handle handle, int_16 pipe_id, USB_SETUP_PTR setup_ptr, uchar_ptr buff_ptr) {
    _usb_host_send_setup(handle, pipe_id, (uchar_ptr)setup_ptr, NULL, buff_ptr, BSWAP16(setup_ptr->WLENGTH), 0);
}

static void host_ch9SetFeature(_usb_host_handle handle, int_16 pipe_id, USB_SETUP_PTR setup_ptr, uchar_ptr buff_ptr) {
    _usb_host_send_setup(handle, pipe_id, (uchar_ptr)setup_ptr, NULL, buff_ptr, BSWAP16(setup_ptr->WLENGTH), 0);
}

void host_ch9Vendor(_usb_host_handle handle, int_16 pipe_id, USB_SETUP_PTR setup_ptr, uchar_ptr buff_ptr) {
    _usb_host_send_setup(handle, pipe_id, (uchar_ptr)setup_ptr, NULL, buff_ptr, BSWAP16(setup_ptr->WLENGTH), 0);
}

s32 ucode_to_str(u16* ucp, char* sp, s32 size) {
    s32 n;
    s32 count;

    n = *ucp >> 8;
    if (size < n) {
        n = size;
    }
    ucp++;

    for (count = 0; count < n; count++) {
        *sp++ = (*ucp++) >> 8;
    }

    if (n < size) {
        *sp = '\0';
    }
    return n;
}

static USB_SETUP _usb_get_string_desc = {
    /* BREQUESTTYPE */  USB_REQ_DIR_IN|USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_DEVICE,
    /* BREQUEST     */  USB_REQ_GET_DESCRIPTOR,
    /* WVALUE       */  BSWAP16(USB_GET_DESC_STRING),
    /* WINDEX       */  BSWAP16(0),
    /* WLENGTH      */  BSWAP16(USB_MAX_PACKET_SIZE_FULLSPEED),
};

enum usb_enumeration_state {
    /* 0 */ USB_ENUMERATE_GET_DEVICE_DESC_SHORT,
    /* 1 */ USB_ENUMERATE_GET_DEVICE_DESC,
    /* 2 */ USB_ENUMERATE_GET_CONFIG_DESC_SHORT,
    /* 3 */ USB_ENUMERATE_GET_CONFIG_DESC,
    /* 4 */ USB_ENUMERATE_GET_MFG_STRING_LENGTH,
    /* 5 */ USB_ENUMERATE_GET_MFG_STRING,
    /* 6 */ USB_ENUMERATE_GET_PROD_STRING_LENGTH,
    /* 7 */ USB_ENUMERATE_GET_PROD_STRING,
    /* 8 */ USB_ENUMERATE_SAVE_PROD_STRING,
    /* 9 */ USB_ENUMERATE_DONE
};

static void host_process_default_pipe(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    if (hgp->ENUMERATION_DONE) {
        return;
    }

    switch (hgp->ENUMERATION_STATE) {
        case USB_ENUMERATE_GET_DEVICE_DESC_SHORT:
            tx_get_desc.WLENGTH = BSWAP16(8);
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &tx_get_desc, (uchar_ptr)&hgp->RX_DESC_DEVICE);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_DEVICE_DESC;
            break;
        case USB_ENUMERATE_GET_DEVICE_DESC:
            tx_get_desc.WLENGTH = BSWAP16(18);
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &tx_get_desc, (uchar_ptr)&hgp->RX_DESC_DEVICE);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_CONFIG_DESC_SHORT;
            break;
        case USB_ENUMERATE_GET_CONFIG_DESC_SHORT:
            tx_get_config_desc.WLENGTH = BSWAP16(8);
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &tx_get_config_desc, (uchar_ptr)hgp->RX_CONFIG_DESC);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_CONFIG_DESC;
            break;
        case USB_ENUMERATE_GET_CONFIG_DESC:
            tx_get_config_desc.WLENGTH = BSWAP16(hgp->RX_CONFIG_DESC[2]); // wTotalLength
            // BUG: RX_CONFIG_DESC is only 64 bytes but it submits an uncapped transfer based on device-controlled
            // wTotalLength.
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &tx_get_config_desc, (uchar_ptr)hgp->RX_CONFIG_DESC);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_MFG_STRING_LENGTH;
            break;
        case USB_ENUMERATE_GET_MFG_STRING_LENGTH:
            _usb_get_string_desc.WVALUE = BSWAP16(USB_GET_DESC_STRING | 1);
            _usb_get_string_desc.WLENGTH = BSWAP16(2);
            bzero(_usb_ucode_str, sizeof(_usb_ucode_str));
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &_usb_get_string_desc, (uchar_ptr)_usb_ucode_str);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_MFG_STRING;
            break;
        case USB_ENUMERATE_GET_MFG_STRING:
            _usb_get_string_desc.WLENGTH = BSWAP16(*(char*)_usb_ucode_str);
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &_usb_get_string_desc, (uchar_ptr)_usb_ucode_str);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_PROD_STRING_LENGTH;
            break;
        case USB_ENUMERATE_GET_PROD_STRING_LENGTH:
            ucode_to_str(_usb_ucode_str, (char*)hgp->MFG_STR, ARRLEN(_usb_ucode_str));
            _usb_get_string_desc.WVALUE = BSWAP16(USB_GET_DESC_STRING | 2);
            _usb_get_string_desc.WLENGTH = BSWAP16(2);
            bzero(_usb_ucode_str, sizeof(_usb_ucode_str));
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &_usb_get_string_desc, (uchar_ptr)_usb_ucode_str);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_PROD_STRING;
            break;
        case USB_ENUMERATE_GET_PROD_STRING:
            _usb_get_string_desc.WLENGTH = BSWAP16(*(char*)_usb_ucode_str);
            host_ch9GetDescription(handle, hgp->DEFAULT_PIPE, &_usb_get_string_desc, (uchar_ptr)_usb_ucode_str);
            hgp->ENUMERATION_STATE = USB_ENUMERATE_SAVE_PROD_STRING;
            break;
        case USB_ENUMERATE_SAVE_PROD_STRING:
            ucode_to_str(_usb_ucode_str, (char*)hgp->PROD_STR, ARRLEN(_usb_ucode_str));
            hgp->ENUMERATION_STATE = USB_ENUMERATE_DONE;
            hgp->ENUMERATION_DONE = TRUE;
            break;
    }
}

static void host_process_attach(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->DEVICE_ATTACHED = TRUE;
    hgp->DEFAULT_PIPE = _usb_host_open_pipe(usb_host_ptr, 0, 0, USB_RECV, 0, USB_CONTROL_PIPE, USB_MAX_PACKET_SIZE_FULLSPEED, 0, FALSE);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(hgp->DEFAULT_PIPE), host_process_default_pipe);
    hgp->DEVICE_ADDRESS++;
    tx_set_address.WVALUE = BSWAP16(hgp->DEVICE_ADDRESS);
    host_ch9SetAddress(usb_host_ptr, hgp->DEFAULT_PIPE, &tx_set_address, (uchar*)hgp->RX_TEST_BUFFER);
    hgp->USB_CURRENT_ADDRESS = hgp->DEVICE_ADDRESS;
    hgp->ENUMERATION_DONE = FALSE;
    hgp->ENUMERATION_STATE = USB_ENUMERATE_GET_DEVICE_DESC_SHORT;
    hgp->REGISTER_SERVICES = TRUE;
    hgp->DEFAULT_PIPE = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 0, USB_RECV, 0, USB_CONTROL_PIPE, hgp->RX_DESC_DEVICE.BMAXPACKETSIZE, 0, FALSE);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(hgp->DEFAULT_PIPE), host_process_default_pipe);
}

static void host_process_detach(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    if (hgp->driver != NULL) {
        hgp->driver->funcs->detach(handle, length);
    }
    _usb_host_close_all_pipes(usb_host_ptr);
    hgp->ENUMERATION_DONE = FALSE;
    hgp->DEVICE_ATTACHED = FALSE;
    __usbInvalidateHandles(usb_host_ptr->CTLR_NUM);
}

static void host_process_resume(_usb_host_handle handle, uint_32 length) {
}

static void host_process_pipe_transaction_done(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = TRUE;
}

void _usb_host_state_machine(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];
    _usb_ext_handle* uhp;
    __OSBbUsbMesg* mp;

    if (hgp->DEVICE_ATTACHED && hgp->ENUMERATION_DONE) {
        if (hgp->REGISTER_SERVICES) {
            _usb_host_unregister_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(hgp->DEFAULT_PIPE));
            _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(hgp->DEFAULT_PIPE), host_process_pipe_transaction_done);
            hgp->REGISTER_SERVICES = FALSE;
        }
        if (hgp->driver != NULL && hgp->TRANSACTION_COMPLETE == TRUE) {
            uhp = hgp->curhandle;
            if (uhp != NULL) {
                mp = uhp->uh_wr_msg;
                if (mp != NULL) {
                    mp->um_ret = 0;
                    mp->um_type |= 0x80;
                    osSendMesg(mp->um_rq, mp, OS_MESG_NOBLOCK);
                    uhp->uh_wr_msg = NULL;
                }
                mp = uhp->uh_rd_msg;
                if (mp != NULL) {
                    mp->um_ret = 0;
                    mp->um_type |= 0x80;
                    osSendMesg(mp->um_rq, mp, OS_MESG_NOBLOCK);
                    uhp->uh_rd_msg = NULL;
                }
                hgp->curhandle = NULL;
                hgp->TRANSACTION_COMPLETE = FALSE;
            }
        }
    }
}

void __usb_host_chapter9_negotiation(s32 which) {
    _usb_host_handle handle = __osArcHostHandle[which];
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[which];
    int i;

    hgp->TRANSACTION_COMPLETE = FALSE;

    for (i = 0; i < ARRLEN(host_driver); i++) {
        if (host_driver[i] != NULL && host_driver[i]->funcs->match((USB_DEV_DESC*)&hgp->RX_DESC_DEVICE)) {
            hgp->driver = host_driver[i];
            break;
        }
    }

    if (i != ARRLEN(host_driver) && hgp->driver != NULL && hgp->driver->funcs != NULL) {
        hgp->driver->funcs->chap9(handle);
    }
}

void __usb_arc_host_setup(int which, _usb_host_handle* handlep) {
    if (_usb_host_init(which, which, 16, handlep) == 0) {
        _usb_host_register_service(*handlep, USB_HOST_SERVICE_ATTACH, host_process_attach);
        _usb_host_register_service(*handlep, USB_HOST_SERVICE_DETACH, host_process_detach);
        _usb_host_register_service(*handlep, USB_HOST_SERVICE_RESUME, host_process_resume);
    }
}

void __usb_host_driver_register(usb_host_driver* hdp) {
    int i;

    for (i = 0; i < ARRLEN(host_driver); i++) {
        if (host_driver[i] == NULL) {
            host_driver[i] = hdp;
            return;
        }
    }
}
