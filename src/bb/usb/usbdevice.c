#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "macros.h"
#include "usb.h"
#ident "$Revision: 1.1 $"

DEV_GLOBAL_STRUCT dev_global_struct;
uint_16 usb_status;
uint_8 endpoint;
uint_8 if_status;
uint_8 data_to_send;
uint_16 sof_count;
uint_8 setup_packet[8];
SETUP_STRUCT local_setup_packet;

uchar USB_IF_ALT[4] = { 0, 0, 0, 0 };

static void ch9GetStatus(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 index;

    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    switch (setup_ptr->REQUESTTYPE) {
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_DEVICE|USB_REQ_TYPE_STANDARD):
            // Retrieve and send the desired status
            _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_status);
            _usb_device_send_data(handle, 0, (uchar_ptr)&usb_status, sizeof(usb_status));
            break;
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_INTERFACE|USB_REQ_TYPE_STANDARD):
            index = BSWAP16(setup_ptr->INDEX) & 0xFF;
            index = MIN(ARRLEN(USB_IF_ALT) - 1, index);
            if_status = USB_IF_ALT[index];
            _usb_device_send_data(handle, 0, &if_status, sizeof(if_status));
            break;
        case (USB_REQ_DIR_IN|USB_REQ_RECPT_ENDPOINT|USB_REQ_TYPE_STANDARD):
            endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
            _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
            _usb_device_send_data(handle, 0, (uchar_ptr)&usb_status, sizeof(usb_status));
            break;
        default:
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            return;
    }

    // Prepare to receive status phase
    _usb_device_recv_data(handle, 0, NULL, 0);
}

static void ch9ClearFeature(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    static uint_8 endpoint;
    uint_16 usb_status;
    uint_16 value;

    _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_status);

    if (usb_status > USB_SELF_POWERED) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }
    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    value = BSWAP16(setup_ptr->VALUE);

    switch (setup_ptr->REQUESTTYPE) {
        case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_DEVICE|USB_REQ_DIR_OUT):
            if (value == 1) { // unset remote wakeup
                _usb_device_get_status(handle, USB_STATUS_DEVICE, &usb_status);
                usb_status &= ~USB_REMOTE_WAKEUP;
                _usb_device_set_status(handle, USB_STATUS_DEVICE, usb_status);
            }
            break;
        case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_ENDPOINT|USB_REQ_DIR_OUT):
            if (value != 0) { // invalid
                _usb_device_stall_endpoint(handle, 0, USB_RECV);
                return;
            }
            // unstall endpoint
            endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
            if (dev_global_struct.funcs->stall_ep != NULL) {
                dev_global_struct.funcs->stall_ep(handle, endpoint, FALSE);
            }
            _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
            _usb_device_set_status(handle, USB_STATUS_ENDPOINT | endpoint, FALSE);
            break;
        default:
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            break;
    }

    // Status phase
    _usb_device_send_data(handle, 0, NULL, 0);
}

static void ch9SetFeature(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 usb_status;
    uint_16 value = BSWAP16(setup_ptr->VALUE);
    uint_8 endpoint;

    if (setup) {
        switch (setup_ptr->REQUESTTYPE) {
            case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_DEVICE|USB_REQ_DIR_OUT):
                if (value == 1) { // set remote wakeup
                    _usb_device_get_status(handle, USB_STATUS_DEVICE, &usb_status);
                    usb_status |= USB_REMOTE_WAKEUP;
                    _usb_device_set_status(handle, USB_STATUS_DEVICE, usb_status);
                }
                break;
            case (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_ENDPOINT|USB_REQ_DIR_OUT):
                if (value != 0) { // invalid
                    _usb_device_stall_endpoint(handle, 0, USB_RECV);
                    return;
                }
                // stall endpoint
                endpoint = BSWAP16(setup_ptr->INDEX) & USB_STATUS_ENDPOINT_NUMBER_MASK;
                _usb_device_get_status(handle, USB_STATUS_ENDPOINT | endpoint, &usb_status);
                _usb_device_set_status(handle, USB_STATUS_ENDPOINT | endpoint, TRUE);
                if (dev_global_struct.funcs->stall_ep != NULL) {
                    dev_global_struct.funcs->stall_ep(handle, endpoint, TRUE);
                }
                break;
            default:
                _usb_device_stall_endpoint(handle, 0, USB_RECV);
                return;
        }

        // Status phase
        _usb_device_send_data(handle, 0, NULL, 0);
        return;
    }

    // Status phase complete, register suspend
    if (value & 3) {
        _usb_device_register_service(handle, USB_SERVICE_SUSPEND, dev_bus_suspend);
    }
}

static void ch9SetAddress(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    static uint_8 new_address;

    if (setup) {
        // Read the address sent by the host
        new_address = BSWAP16(setup_ptr->VALUE);
        // Send a zero-length response as status phase
        _usb_device_send_data(handle, 0, NULL, 0);
    } else {
        // When the zero-length packet (status phase) completes, actually assign the address and init endpoints
        _usb_device_set_status(handle, USB_STATUS_ADDRESS, new_address);
        _usb_device_set_status(handle, USB_STATUS_DEVICE_STATE, USB_SELF_POWERED);
        dev_global_struct.funcs->initeps(handle);
    }
}

static void ch9GetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    dev_global_struct.funcs->get_desc(handle, setup, setup_ptr);
}

static void ch9SetDescription(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    _usb_device_stall_endpoint(handle, 0, USB_RECV);
}

static void ch9GetConfig(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 current_config;

    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    _usb_device_get_status(handle, USB_STATUS_CURRENT_CONFIG, &current_config);
    data_to_send = current_config;
    _usb_device_send_data(handle, 0, &data_to_send, sizeof(data_to_send));

    // Status phase
    _usb_device_recv_data(handle, 0, NULL, 0);
}

static void ch9SetConfig(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 usb_state;
    uint_16 value;

    _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_state);

    if (usb_state < USB_REMOTE_WAKEUP) {
        if (!setup) {
            // Status phase complete, nothing to do
            return;
        }

        value = BSWAP16(setup_ptr->VALUE) & 0xFF;

        if (value == 0) {
            _usb_device_set_status(handle, USB_STATUS_CURRENT_CONFIG, 0);
            _usb_device_set_status(handle, USB_STATUS_DEVICE_STATE, USB_SELF_POWERED);
        } else {
            _usb_device_get_status(handle, USB_STATUS_CURRENT_CONFIG, &usb_state);
            if (usb_state != value) {
                _usb_device_set_status(handle, USB_STATUS_CURRENT_CONFIG, value);
            }
            _usb_device_set_status(handle, USB_STATUS_DEVICE_STATE, USB_STATE_0);
        }

        // Status phase
        _usb_device_send_data(handle, 0, NULL, 0);
    } else {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
    }
}

static void ch9GetInterface(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 usb_state;
    uint_16 index;
    uint_16 length;

    _usb_device_get_status(handle, USB_STATUS_DEVICE_STATE, &usb_state);

    if (usb_state != 0) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }
    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }

    index = BSWAP16(setup_ptr->INDEX) & 0xFF;
    index = MIN(ARRLEN(USB_IF_ALT) - 1, index);
    length = BSWAP16(setup_ptr->LENGTH);
    _usb_device_send_data(handle, 0, &USB_IF_ALT[index], length != 0);

    // Status phase
    _usb_device_recv_data(handle, 0, NULL, 0);
}

static void ch9SetInterface(_usb_device_handle handle, u32 setup, SETUP_STRUCT* setup_ptr) {
    uint_16 value;
    uint_16 index;

    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }
    if (setup_ptr->REQUESTTYPE != (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_INTERFACE|USB_REQ_DIR_OUT)) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }

    index = BSWAP16(setup_ptr->INDEX) & 0xFF;
    index = MIN(ARRLEN(USB_IF_ALT) - 1, index);
    value = BSWAP16(setup_ptr->VALUE) & 0xFF;
    if (USB_IF_ALT[index] != value) {
        USB_IF_ALT[index] = value;
    }

    // Status phase
    _usb_device_send_data(handle, 0, NULL, 0);
}

static void ch9SynchFrame(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    uint_16 index;
    uint_16 length;
    uint_32 temp;

    if (!setup) {
        // Status phase complete, nothing to do
        return;
    }
    if (setup_ptr->REQUESTTYPE != (USB_REQ_TYPE_STANDARD|USB_REQ_RECPT_ENDPOINT|USB_REQ_DIR_OUT)) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }

    index = BSWAP16(setup_ptr->INDEX) & 0xFF;
    if (index >= dev_global_struct.num_ifcs) {
        _usb_device_stall_endpoint(handle, 0, USB_RECV);
        return;
    }

    temp = BSWAP16(setup_ptr->LENGTH);
    length = MIN(temp, sizeof(sof_count));
    _usb_device_get_status(handle, USB_STATUS_SOF_COUNT, &sof_count);
    _usb_device_send_data(handle, 0, (uchar_ptr)&sof_count, length);

    // Status phase
    _usb_device_recv_data(handle, 0, NULL, 0);
}

static void ch9Vendor(_usb_device_handle handle, boolean setup, SETUP_STRUCT_PTR setup_ptr) {
    dev_global_struct.funcs->vendor(handle, setup, setup_ptr);
}

static void dev_service_ep0(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    if (setup) {
        // Obtain the setup packet. setup is false when a status phase completes
        _usb_device_read_setup_data(handle, 0, (uchar_ptr)&local_setup_packet);
    }

    switch (local_setup_packet.REQUESTTYPE & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_STANDARD:
            switch (local_setup_packet.REQUEST) {
                case USB_REQ_GET_STATUS:
                    ch9GetStatus(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_CLEAR_FEATURE:
                    ch9ClearFeature(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SET_FEATURE:
                    ch9SetFeature(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SET_ADDRESS:
                    ch9SetAddress(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_GET_DESCRIPTOR:
                    ch9GetDescription(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SET_DESCRIPTOR:
                    ch9SetDescription(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_GET_CONFIGURATION:
                    ch9GetConfig(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SET_CONFIGURATION:
                    ch9SetConfig(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_GET_INTERFACE:
                    ch9GetInterface(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SET_INTERFACE:
                    ch9SetInterface(handle, setup, &local_setup_packet);
                    break;
                case USB_REQ_SYNCH_FRAME:
                    ch9SynchFrame(handle, setup, &local_setup_packet);
                    break;
                default:
                    _usb_device_stall_endpoint(handle, 0, USB_RECV);
                    break;
            }
            break;
        case USB_REQ_TYPE_CLASS:
            break;
        case USB_REQ_TYPE_VENDOR:
            ch9Vendor(handle, setup, &local_setup_packet);
            break;
        default:
            _usb_device_stall_endpoint(handle, 0, USB_RECV);
            break;
    }
}

static void dev_reset_ep0(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    dev_global_struct.FIRST_SOF = TRUE;

    dev_global_struct.funcs->reset_ep0(handle, setup, direction, buffer, length);

    if (dev_global_struct.dev_state != 2) {
        dev_global_struct.dev_state = 1;
    }
}

void dev_bus_suspend(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
}

static void dev_bus_sof(_usb_device_handle handle, boolean setup, uint_8 direction, uint_8_ptr buffer, uint_32 length) {
    if (dev_global_struct.FIRST_SOF) {
        // Register suspend on first sof
        _usb_device_register_service(handle, USB_SERVICE_SUSPEND, dev_bus_suspend);
        dev_global_struct.FIRST_SOF = FALSE;
    }
}

void _usb_device_state_machine(_usb_device_handle handle) {
}

void __usb_arc_device_setup(int which, _usb_device_handle* handlep) {
    if (_usb_device_init(which, 0, handlep, USB_NUM_ENDPOINTS) != 0) {
        return;
    }
    _usb_device_register_service(*handlep, USB_SERVICE_EP(0), dev_service_ep0);
    _usb_device_register_service(*handlep, USB_SERVICE_RESET_EP0, dev_reset_ep0);
    _usb_device_register_service(*handlep, USB_SERVICE_SOF, dev_bus_sof);
    rdbs_device_init(*handlep);
    dev_global_struct.funcs = &rdbs_dev_funcs;
    dev_global_struct.dev_state = 0;
}
