#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "usb.h"
#ident "$Revision: 1.1 $"

s32 osBbUsbDevQuery(s32 which, OSBbUsbInfo* ip, s32 nip) {
    if (which != 0 && which != 1) {
        return -1;
    }

    if (nip == 0) {
        return 0;
    }

    if (_usb_ctlr_state[which].ucs_mode == UCS_MODE_NONE) {
        return 0;
    } else if (_usb_ctlr_state[which].ucs_mode == UCS_MODE_DEVICE) {
        bzero(ip, nip * sizeof(*ip));
        dev_global_struct.funcs->query(ip);
        return 1;
    } else {
        return 0;
    }
}
