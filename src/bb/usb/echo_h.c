#include "PR/os_internal.h"
#include "PR/bcp.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

extern HOST_GLOBAL_STRUCT host_global_struct[2];

typedef struct echo_global_struct {
    /* 0x0000 */ boolean ECHO_ENABLED;
    /* 0x0004 */ int_16 PIPE_INT_IN;
    /* 0x0006 */ int_16 PIPE_INT_OUT;
    /* 0x0008 */ int_16 PIPE_BULK_IN;
    /* 0x000A */ int_16 PIPE_BULK_OUT;
    /* 0x000C */ uchar INTDATAOUT[300];
    /* 0x0138 */ uchar INTDATAIN[300];
    /* 0x0264 */ uchar BULKDATAOUT[300];
    /* 0x0390 */ uchar BULKDATAIN[300];
    /* 0x04BC */ uchar RX_TEST_BUFFER[20];
} ECHO_GLOBAL_STRUCT, *ECHO_GLOBAL_STRUCT_PTR; // size = 0x4D0

volatile ECHO_GLOBAL_STRUCT echo_host_struct;

static void echo_host_detach(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    _usb_host_cancel_transfer(usb_host_ptr, hgp->DEFAULT_PIPE);
    _usb_host_cancel_transfer(usb_host_ptr, echo_host_struct.PIPE_INT_IN);
    _usb_host_cancel_transfer(usb_host_ptr, echo_host_struct.PIPE_INT_OUT);
    _usb_host_cancel_transfer(usb_host_ptr, echo_host_struct.PIPE_BULK_IN);
    _usb_host_cancel_transfer(usb_host_ptr, echo_host_struct.PIPE_BULK_OUT);
}

static void echo_host_send_transaction_done(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = TRUE;
}

static void echo_host_recv_transaction_done(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = TRUE;
}

static void echo_host_chapter9(_usb_host_handle handle) {
}

static void echo_host_open(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    echo_host_struct.PIPE_INT_OUT = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 2, USB_SEND, 0, USB_INTERRUPT_PIPE,
                                                        USB_MAX_PACKET_SIZE_FULLSPEED, 0, TRUE);
    echo_host_struct.PIPE_INT_IN = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 2, USB_RECV, 0, USB_INTERRUPT_PIPE,
                                                       USB_MAX_PACKET_SIZE_FULLSPEED, 0, TRUE);
    echo_host_struct.PIPE_BULK_OUT = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 1, USB_SEND, 0, USB_BULK_PIPE,
                                                         USB_MAX_PACKET_SIZE_FULLSPEED, 0, TRUE);
    echo_host_struct.PIPE_BULK_IN = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 1, USB_RECV, 0, USB_BULK_PIPE,
                                                        USB_MAX_PACKET_SIZE_FULLSPEED, 0, TRUE);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(echo_host_struct.PIPE_INT_OUT), echo_host_send_transaction_done);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(echo_host_struct.PIPE_INT_IN), echo_host_recv_transaction_done);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(echo_host_struct.PIPE_BULK_OUT), echo_host_send_transaction_done);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(echo_host_struct.PIPE_BULK_IN), echo_host_recv_transaction_done);
}

usb_dev_desc echo_dev_desc = {
    /* BLENGTH              */  sizeof(usb_dev_desc),
    /* BDESCTYPE            */  USB_DESC_DEVICE,
    /* BCDUSB               */  0x0110,
    /* BDEVICECLASS         */  0,
    /* BDEVICESUBCLASS      */  0,
    /* BDEVICEPROTOCOL      */  0,
    /* BMAXPACKETSIZE       */  USB_MAX_PACKET_SIZE_FULLSPEED,
    /* IDVENDOR             */  0x05A3,
    /* IDPRODUCT            */  0x0001,
    /* BCDDEVICE            */  0x0002,
    /* BMANUFACTURER        */  1,
    /* BPRODUCT             */  2,
    /* BSERIALNUMBER        */  0,
    /* BNUMCONFIGURATIONS   */  1,
};

int echo_host_match(usb_dev_desc* uddp) {
    if (BSWAP16(uddp->IDVENDOR) != echo_dev_desc.IDVENDOR) {
        return FALSE;
    }
    if (BSWAP16(uddp->IDPRODUCT) != echo_dev_desc.IDPRODUCT) {
        return FALSE;
    }
    if (BSWAP16(uddp->BCDDEVICE) != echo_dev_desc.BCDDEVICE) {
        return FALSE;
    }
    return TRUE;
}

static void echo_host_poll(_usb_host_handle handle) {
}

static void echo_host_send(_usb_host_handle handle, u8* buffer, s32 len, u64 offset) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    _usb_host_send_data(usb_host_ptr, echo_host_struct.PIPE_BULK_OUT, buffer, len);
}

static void echo_host_recv(_usb_host_handle handle, u8* buffer, s32 len, u64 offset) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    _usb_host_recv_data(usb_host_ptr, echo_host_struct.PIPE_BULK_IN, buffer, len);
}

usb_host_driver_funcs echo_host_funcs = {
    echo_host_match,
    echo_host_open,
    echo_host_poll,
    echo_host_send,
    echo_host_recv,
    echo_host_detach,
    echo_host_chapter9,
};

usb_host_driver echo_host_driver = {
    &echo_dev_desc,
    &echo_host_funcs,
    "Echo loopback",
};
