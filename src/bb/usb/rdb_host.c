#include "PR/os_internal.h"
#include "PR/bcp.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

extern HOST_GLOBAL_STRUCT host_global_struct[2];

struct {
    /* 0x0000 */ boolean rdbs_enabled;
    /* 0x0004 */ int_16 bulkpipein;
    /* 0x0006 */ int_16 bulkpipeout;
} rdbs_host_state;

static char rdbs_send_buf[USB_MAX_PACKET_SIZE_FULLSPEED];
static char rdbs_recv_buf[USB_MAX_PACKET_SIZE_FULLSPEED];

static void rdbs_host_detach(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    _usb_host_cancel_transfer(usb_host_ptr, hgp->DEFAULT_PIPE);
    if (rdbs_host_state.rdbs_enabled) {
        _usb_host_cancel_transfer(usb_host_ptr, rdbs_host_state.bulkpipein);
        _usb_host_cancel_transfer(usb_host_ptr, rdbs_host_state.bulkpipeout);
    }
}

static void rdbs_host_send_transaction_done(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = TRUE;
}

static void rdbs_host_recv_transaction_done(_usb_host_handle handle, uint_32 length) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = TRUE;
}

static void rdbs_host_chapter9(_usb_host_handle handle) {
}

static void rdbs_host_open(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    rdbs_host_state.bulkpipeout = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 2, USB_SEND, 0,
                                                      USB_BULK_PIPE, USB_MAX_PACKET_SIZE_FULLSPEED, 0, FALSE);
    rdbs_host_state.bulkpipein = _usb_host_open_pipe(usb_host_ptr, hgp->USB_CURRENT_ADDRESS, 2, USB_RECV, 0,
                                                     USB_BULK_PIPE, USB_MAX_PACKET_SIZE_FULLSPEED, 0, FALSE);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(rdbs_host_state.bulkpipeout), rdbs_host_send_transaction_done);
    _usb_host_register_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(rdbs_host_state.bulkpipein), rdbs_host_recv_transaction_done);
}

usb_dev_desc rdbs_dev_desc = {
    /* BLENGTH              */  sizeof(usb_dev_desc),
    /* BDESCTYPE            */  USB_DESC_DEVICE,
    /* BCDUSB               */  0x0110,
    /* BDEVICECLASS         */  0,
    /* BDEVICESUBCLASS      */  0,
    /* BDEVICEPROTOCOL      */  0,
    /* BMAXPACKETSIZE       */  USB_MAX_PACKET_SIZE_FULLSPEED - 2,
    /* IDVENDOR             */  0xBB3D, // Note this vendor ID is different from rdb_slave and doesn't match the USB-IF assigned vendor ID for iQue (or any other)
    /* IDPRODUCT            */  0xBBDB,
    /* BCDDEVICE            */  0x0001,
    /* BMANUFACTURER        */  1,
    /* BPRODUCT             */  2,
    /* BSERIALNUMBER        */  0,
    /* BNUMCONFIGURATIONS   */  1,
};

int rdbs_host_match(usb_dev_desc* uddp) {
    if (BSWAP16(uddp->IDVENDOR) != rdbs_dev_desc.IDVENDOR) {
        return FALSE;
    }
    if (BSWAP16(uddp->IDPRODUCT) != rdbs_dev_desc.IDPRODUCT) {
        return FALSE;
    }
    if (BSWAP16(uddp->BCDDEVICE) != rdbs_dev_desc.BCDDEVICE) {
        return FALSE;
    }
    return TRUE;
}

static void rdbs_host_poll(_usb_host_handle handle) {
}

static void rdbs_host_send(_usb_host_handle handle, u8* buffer, s32 len, u64 offset) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = FALSE;
    if (len > USB_MAX_PACKET_SIZE_FULLSPEED - 2) {
        len = USB_MAX_PACKET_SIZE_FULLSPEED - 2;
    }
    bzero(rdbs_send_buf, USB_MAX_PACKET_SIZE_FULLSPEED);
    *(u16*)&rdbs_send_buf[0] = len;
    bcopy(buffer, &rdbs_send_buf[2], len);

    _usb_host_send_data(usb_host_ptr, rdbs_host_state.bulkpipeout, (uchar_ptr)rdbs_send_buf, USB_MAX_PACKET_SIZE_FULLSPEED);
    while (!hgp->TRANSACTION_COMPLETE) {
        if (!hgp->DEVICE_ATTACHED) {
            return;
        }
        osYieldThread();
    }
}

static void rdbs_host_recv(_usb_host_handle handle, u8* buffer, s32 len, u64 offset) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_GLOBAL_STRUCT_PTR hgp = &host_global_struct[usb_host_ptr->CTLR_NUM];

    hgp->TRANSACTION_COMPLETE = FALSE;
    _usb_host_recv_data(usb_host_ptr, rdbs_host_state.bulkpipein, (uchar_ptr)rdbs_recv_buf, USB_MAX_PACKET_SIZE_FULLSPEED);

    while (!hgp->TRANSACTION_COMPLETE) {
        if (!hgp->DEVICE_ATTACHED) {
            return;
        }
        osYieldThread();
    }

    len = *(u16*)&rdbs_recv_buf[0];
    if (len > USB_MAX_PACKET_SIZE_FULLSPEED - 2) {
        len = USB_MAX_PACKET_SIZE_FULLSPEED - 2;
    }
    bcopy(&rdbs_recv_buf[2], buffer, len);
}

usb_host_driver_funcs rdbs_host_funcs = {
    rdbs_host_match,
    rdbs_host_open,
    rdbs_host_poll,
    rdbs_host_send,
    rdbs_host_recv,
    rdbs_host_detach,
    rdbs_host_chapter9,
};

usb_host_driver rdbs_host_driver = {
    &rdbs_dev_desc,
    &rdbs_host_funcs,
    "BB RDB host driver",
};
