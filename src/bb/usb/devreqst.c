#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

USB_SETUP tx_get_status = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_GET_STATUS,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(2),
};

USB_SETUP tx_clear_feature = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_CLEAR_FEATURE,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0),
};

USB_SETUP tx_set_feature = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_SET_FEATURE,
    /* WVALUE        */ BSWAP16(3),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0),
};

USB_SETUP tx_set_address = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_SET_ADDRESS,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0),
};

USB_SETUP tx_get_desc = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_GET_DESCRIPTOR,
    /* WVALUE        */ BSWAP16(USB_GET_DESC_DEVICE),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(18),
};

USB_SETUP tx_get_config_desc = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_GET_DESCRIPTOR,
    /* WVALUE        */ BSWAP16(USB_GET_DESC_CONFIG),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0x40),
};

USB_SETUP tx_set_desc = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_SET_DESCRIPTOR,
    /* WVALUE        */ BSWAP16(USB_GET_DESC_DEVICE),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(18),
};

USB_SETUP tx_get_config = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_GET_CONFIGURATION,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(1),
};

USB_SETUP tx_set_config = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ USB_REQ_SET_CONFIGURATION,
    /* WVALUE        */ BSWAP16(1),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0),
};

USB_SETUP tx_get_interface = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_INTERFACE,
    /* BREQUEST      */ USB_REQ_GET_INTERFACE,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(1),
};

USB_SETUP tx_set_interface = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
    /* BREQUEST      */ USB_REQ_SET_INTERFACE,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(0),
};

USB_SETUP tx_synch_frame = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_ENDPOINT,
    /* BREQUEST      */ USB_REQ_SYNCH_FRAME,
    /* WVALUE        */ BSWAP16(0),
    /* WINDEX        */ BSWAP16(0),
    /* WLENGTH       */ BSWAP16(2),
};

USB_SETUP tx_enable_echo = {
    /* BREQUESTTYPE  */ USB_REQ_DIR_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_RECPT_DEVICE,
    /* BREQUEST      */ 2,
    /* WVALUE        */ BSWAP16(29),
    /* WINDEX        */ BSWAP16(1),
    /* WLENGTH       */ BSWAP16(0),
};
