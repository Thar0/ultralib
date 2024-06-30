#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "macros.h"
#include "../usb.h"
#ident "$Revision: 1.1 $"

#define BDT_FOR_DIR(page, dir) ((HOST_BDT_STRUCT_PTR)((uint_32)(page) | (dir)))

void _usb_hci_vusb11_send_data(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    _usb_host_queue_pkts(handle, pipe_descr_ptr);
}

void _usb_hci_vusb11_send_setup(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    _usb_host_queue_pkts(handle, pipe_descr_ptr);
}

void _usb_hci_vusb11_recv_data(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    _usb_host_queue_pkts(handle, pipe_descr_ptr);
}

void _usb_hci_vusb11_cancel_transfer(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
}

void _usb_host_vusb11_set_bdt_page(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->DEV_PTR->BDTPAGE1 = USB_BDTPAGE1_ADDR(KDM_TO_PHYS(usb_host_ptr->USB_BDT_PAGE));
    usb_host_ptr->DEV_PTR->BDTPAGE2 = USB_BDTPAGE2_ADDR(KDM_TO_PHYS(usb_host_ptr->USB_BDT_PAGE));
    usb_host_ptr->DEV_PTR->BDTPAGE3 = USB_BDTPAGE3_ADDR(KDM_TO_PHYS(usb_host_ptr->USB_BDT_PAGE));
}

HOST_BDT_STRUCT_PTR _usb_host_vusb11_get_next_in_bdt(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_IN ^= 8;

    return BDT_FOR_DIR(usb_host_ptr->USB_BDT_PAGE, usb_host_ptr->USB_IN);
}

HOST_BDT_STRUCT_PTR _usb_host_vusb11_get_next_out_bdt(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_OUT ^= 8;

    return BDT_FOR_DIR(usb_host_ptr->USB_BDT_PAGE, usb_host_ptr->USB_OUT);
}

void _usb_host_vusb11_sched_pending_pkts(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
    PIPE_DESCRIPTOR_STRUCT_PTR temp = NULL;
    uint_8 i;
    boolean sched_bulk_pipe = FALSE;
    boolean sched_ctrl_pipe = FALSE;
    boolean sched_int_pipe = FALSE;

    for (i = 0; i < 2; i++) {
        if (usb_host_ptr->CURRENT_INTR_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_INTR_HEAD];
            if (pipe_descr_ptr->CURRENT_INTERVAL == 0) {
                pipe_descr_ptr->CURRENT_INTERVAL = pipe_descr_ptr->INTERVAL;
                sched_int_pipe = TRUE;
            } else {
                sched_int_pipe = FALSE;
            }
        } else if (usb_host_ptr->CURRENT_CTRL_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_CTRL_HEAD];
            if (pipe_descr_ptr->CURRENT_INTERVAL == 0) {
                sched_ctrl_pipe = TRUE;
            } else {
                sched_ctrl_pipe = FALSE;
            }
        } else if (usb_host_ptr->CURRENT_BULK_HEAD != -1) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_BULK_HEAD];
            if (pipe_descr_ptr->CURRENT_INTERVAL == 0) {
                sched_bulk_pipe = TRUE;
            } else {
                sched_bulk_pipe = FALSE;
            }
        }

        if (usb_host_ptr->CURRENT_INTR_HEAD != -1 && sched_int_pipe) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_INTR_HEAD];
        } else if (usb_host_ptr->CURRENT_CTRL_HEAD != -1 && sched_ctrl_pipe) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_CTRL_HEAD];
        } else if (usb_host_ptr->CURRENT_BULK_HEAD != -1 && sched_bulk_pipe) {
            pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_BULK_HEAD];
        } else {
            break;
        }

        if (i == 0 || temp != pipe_descr_ptr) {
            temp = pipe_descr_ptr;
            _usb_host_vusb11_initiate_correct_bdt(usb_host_ptr, pipe_descr_ptr);
            _usb_host_update_current_head(usb_host_ptr, pipe_descr_ptr->PIPETYPE);
        }
    }
}

void _usb_host_vusb11_sched_iso_pkts(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
    PIPE_DESCRIPTOR_STRUCT_PTR temp = NULL;
    uint_8 i;

    for (i = 0; i < 2; i++) {
        if (usb_host_ptr->CURRENT_ISO_HEAD == -1) {
            break;
        }

        pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_ISO_HEAD];

        if (i == 0 || temp != pipe_descr_ptr) {
            temp = pipe_descr_ptr;
            _usb_host_vusb11_initiate_correct_bdt(usb_host_ptr, pipe_descr_ptr);
            _usb_host_update_current_head(usb_host_ptr, pipe_descr_ptr->PIPETYPE);
        }
    }
}

void _usb_host_vusb11_initiate_correct_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    if (!pipe_descr_ptr->PACKETPENDING) {
        return;
    }

    if (pipe_descr_ptr->PIPETYPE == USB_CONTROL_PIPE) {
        if (pipe_descr_ptr->FIRST_PHASE) {
            _usb_host_vusb11_init_setup_bdt(usb_host_ptr, pipe_descr_ptr);
        } else {
            if (pipe_descr_ptr->SEND_PHASE) {
                if (pipe_descr_ptr->TODO2 == 0) {
                    _usb_host_vusb11_send_in_token(usb_host_ptr, pipe_descr_ptr);
                } else {
                    _usb_host_vusb11_send_out_token(usb_host_ptr, pipe_descr_ptr);
                }
            } else {
                if (pipe_descr_ptr->TODO1 == 0) {
                    _usb_host_vusb11_send_out_token(usb_host_ptr, pipe_descr_ptr);
                } else {
                    _usb_host_vusb11_send_in_token(usb_host_ptr, pipe_descr_ptr);
                }
            }
        }
    } else {
        if (pipe_descr_ptr->DIRECTION == USB_SEND) {
            _usb_host_vusb11_init_send_bdt(usb_host_ptr, pipe_descr_ptr);
        } else {
            _usb_host_vusb11_init_recv_bdt(usb_host_ptr, pipe_descr_ptr);
        }
    }
}

void _usb_hci_vusb11_isr(_usb_host_handle handle) {
    uint_8 direction;
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    uint_32 status;
    uint_32 error_status;

    while (TRUE) {
        error_status = usb_host_ptr->DEV_PTR->ERRORSTATUS & usb_host_ptr->DEV_PTR->ERRORENABLE;
        usb_host_ptr->DEV_PTR->ERRORSTATUS = error_status;

        status = usb_host_ptr->DEV_PTR->INTSTATUS & usb_host_ptr->DEV_PTR->INTENABLE;
        if (status == 0) {
            break;
        }

        direction = USB_STAT_TX(usb_host_ptr->DEV_PTR->STATUS);

        usb_host_ptr->DEV_PTR->INTSTATUS = status;

        if (status & USB_ISTAT_RST) {
            usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_ATTACHEN | USB_INTEN_SOFTOKEN;
            usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
            _usb_host_vusb11_process_attach(usb_host_ptr);
            continue;
        }

        if (status & USB_ISTAT_TOKEN_DONE) {
            usb_host_ptr->SND_TOK_IN_PROGRESS = FALSE;
            usb_host_ptr->TOK_THIS_FRAME++;
            usb_host_ptr->USB_PKT_PENDING--;
            _usb_host_vusb11_process_token_done(usb_host_ptr, direction, error_status);
            if (!usb_host_ptr->SND_TOK_IN_PROGRESS) {
                _usb_host_vusb11_sched_pending_pkts(usb_host_ptr);
            }
        }

        if (status & USB_ISTAT_SOFTOK) {
            usb_host_ptr->TOK_THIS_FRAME = 0;
            if (!usb_host_ptr->SND_TOK_IN_PROGRESS) {
                _usb_host_vusb11_sched_iso_pkts(usb_host_ptr);
            }
            _usb_host_update_interval_for_pipes(usb_host_ptr);
            if (!usb_host_ptr->SND_TOK_IN_PROGRESS) {
                _usb_host_vusb11_sched_pending_pkts(usb_host_ptr);
            }
        }

        if (status & USB_ISTAT_ATTACH) {
            _usb_host_vusb11_process_attach(usb_host_ptr);
        }

        if (status & USB_ISTAT_RESUME) {
            usb_host_ptr->DEV_PTR->INTENABLE &= ~USB_INTEN_RESUMEEN;
            _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_RESUME, 0);
        }
    }
}

uint_8 _usb_hci_vusb11_init(_usb_host_handle handle) {
    uint_8 which;
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    which = usb_host_ptr->CTLR_NUM;

    usb_host_ptr->USB_BDT_PAGE = (HOST_BDT_STRUCT_PTR)PHYS_TO_K1(USB_BDT_BUF(which, 0));
    usb_host_ptr->DEV_PTR = (USB_STRUCT_PTR)PHYS_TO_K1(USB_REG_GRP0_ALT(which));
    usb_host_ptr->ENDPT_HOST_RG = (USB_REGISTER_PTR)(usb_host_ptr->DEV_PTR + 1);
    usb_host_ptr->ENDPT_HOST_RG[0] = 0;
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_NONE;
    usb_host_ptr->DEV_PTR->INTSTATUS = USB_ISTAT_ALL;
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_ODD_RST | USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(0);
    _usb_host_vusb11_set_bdt_page(usb_host_ptr);
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->SOFTHRESHOLDLO = USB_SOF_THRESHOLD(USB_MAX_PACKET_SIZE_FULLSPEED);
    usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_ATTACHEN;
    return 0;
}

void _usb_host_delay(_usb_host_handle handle, u32 delay) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    MP_STRUCT16 o1;
    MP_STRUCT16 o2;
    volatile uint_32 dummy_delay;

    o1.B.L = usb_host_ptr->DEV_PTR->FRAMENUMLO;
    o1.B.H = usb_host_ptr->DEV_PTR->FRAMENUMHI;
    o1.W = (o1.W + delay) & 0x7FF;

    dummy_delay = 0xFFFF0000;
    do {
        o2.B.L = usb_host_ptr->DEV_PTR->FRAMENUMLO;
        o2.B.H = usb_host_ptr->DEV_PTR->FRAMENUMHI;
        dummy_delay++;
        osYieldThread();
    } while (o1.W != o2.W && dummy_delay != 0);
}

void _usb_host_vusb11_reset_the_device(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_RESET | USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
    _usb_host_delay(usb_host_ptr, 10);
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_EN;
    _usb_host_delay(usb_host_ptr, 10);
}

void _usb_host_vusb11_process_attach(_usb_host_handle handle) {
    uint_32 b;
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->DEV_PTR->ERRORENABLE = USB_ERREN_ALL;

    _usb_host_delay(usb_host_ptr, 50);

    b = usb_host_ptr->DEV_PTR->CONTROL;

    if (b & USB_CTL_SE0) {
        _usb_host_vusb11_process_detach(usb_host_ptr);
        return;
    }

    if (usb_host_ptr->DEV_PTR->ADDRESS & USB_ADDR_LSEN) {
        if (b & USB_CTL_JSTATE) {
            usb_host_ptr->SPEED = USB_ADDR_LSEN;
            usb_host_ptr->NO_HUB = USB_ENDPT_HOST_WO_HUB;
        } else {
            usb_host_ptr->SPEED = 0;
            usb_host_ptr->NO_HUB = 0;
        }
    } else {
        if (b & USB_CTL_JSTATE) {
            usb_host_ptr->SPEED = 0;
            usb_host_ptr->NO_HUB = 0;
            usb_host_ptr->DEV_PTR->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(0);
        } else {
            usb_host_ptr->SPEED = USB_ADDR_LSEN;
            usb_host_ptr->NO_HUB = USB_ENDPT_HOST_WO_HUB;
            usb_host_ptr->DEV_PTR->ADDRESS = USB_ADDR_LSEN | USB_ADDR_ADDR(0);
        }
    }
    usb_host_ptr->ENDPT_HOST_RG[0] = usb_host_ptr->NO_HUB | USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN;
    _usb_host_vusb11_reset_the_device(usb_host_ptr);
    usb_host_ptr->DEV_PTR->INTSTATUS = USB_ISTAT_ATTACH | USB_ISTAT_RST;
    usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_TOKDNEEN | USB_INTEN_SOFTOKEN | USB_INTEN_USBRSTEN;
    usb_host_ptr->USB_OUT = 0x18;
    usb_host_ptr->USB_IN = 8;
    _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_ATTACH, 0);
}

void _usb_host_vusb11_process_detach(_usb_host_handle handle) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->SND_TOK_IN_PROGRESS = FALSE;
    _usb_host_vusb11_set_bdt_page(usb_host_ptr);
    usb_host_ptr->DEV_PTR->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(0);
    usb_host_ptr->SPEED = 0;
    usb_host_ptr->NO_HUB = 0;
    usb_host_ptr->ENDPT_HOST_RG[0] = USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN;
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_ODD_RST | USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->CONTROL = USB_CTL_HOST_MODE_EN | USB_CTL_USB_DISABLE;
    usb_host_ptr->DEV_PTR->INTSTATUS = USB_ISTAT_ALL;
    usb_host_ptr->DEV_PTR->INTENABLE = USB_INTEN_ATTACHEN;
    _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_DETACH, 0);
}

void _usb_host_vusb11_send_in_token(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    uint_16 newPid;
    HOST_BDT_STRUCT_PTR real_bdt_p;
    HOST_BDT_STRUCT copy_bdt;

    real_bdt_p = _usb_host_vusb11_get_next_in_bdt(usb_host_ptr);
    _usb_bdt_copy_swab_host(real_bdt_p, &copy_bdt);
    newPid = pipe_descr_ptr->NEXTDATA01 | USB_BD_DTS | USB_BD_OWN;
    copy_bdt.ADDR.W = KDM_TO_PHYS(pipe_descr_ptr->RX_PTR);
    if (pipe_descr_ptr->MUSTSENDNULL) {
        newPid |= USB_BD_DATA1;
        copy_bdt.BC = 0;
        pipe_descr_ptr->MUSTSENDNULL = FALSE;
    } else {
        copy_bdt.BC = 0xFF;
    }
    copy_bdt.PID = newPid;
    _usb_bdt_copy_swab_host(&copy_bdt, real_bdt_p);

    _usb_host_buffer_bdt(usb_host_ptr, &copy_bdt);
    _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
    _usb_host_vusb11_send_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
    pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;
}

void _usb_host_vusb11_send_out_token(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT* pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_BDT_STRUCT_PTR real_bdt_ptr;
    HOST_BDT_STRUCT copy_bdt;

    real_bdt_ptr = _usb_host_vusb11_get_next_out_bdt(usb_host_ptr);
    _usb_bdt_copy_swab_host(real_bdt_ptr, &copy_bdt);
    copy_bdt.ADDR.W = KDM_TO_PHYS(pipe_descr_ptr->TX2_PTR);
    if (pipe_descr_ptr->MUSTSENDNULL) {
        copy_bdt.BC = 0;
        copy_bdt.PID = pipe_descr_ptr->NEXTDATA01 | USB_BD_DATA1 | USB_BD_OWN;
        pipe_descr_ptr->MUSTSENDNULL = FALSE;
    } else {
        if (pipe_descr_ptr->TODO2 > pipe_descr_ptr->MAX_PKT_SIZE) {
            copy_bdt.BC = pipe_descr_ptr->MAX_PKT_SIZE;
        } else {
            copy_bdt.BC = pipe_descr_ptr->TODO2;
        }
        copy_bdt.PID = pipe_descr_ptr->NEXTDATA01 | USB_BD_DATA0 | USB_BD_OWN;
    }
    _usb_bdt_copy_swab_host(&copy_bdt, real_bdt_ptr);

    _usb_host_buffer_bdt(usb_host_ptr, &copy_bdt);
    _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
    _usb_host_vusb11_send_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
    pipe_descr_ptr->PACKETPENDING = FALSE;
    pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;
}

void _usb_host_vusb11_send_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    int waitprint = 0;
    int waitprintbackoff = 0x40;

    usb_host_ptr->SND_TOK_IN_PROGRESS = TRUE;
    usb_host_ptr->USB_PKT_PENDING++;

    while (usb_host_ptr->DEV_PTR->CONTROL & USB_CTL_TX_SUSPEND_TOKEN_BUSY) {
        if (waitprint-- == 0) {
            waitprint = waitprintbackoff;
            waitprintbackoff *= 4;
        }
    }
    usb_host_ptr->ENDPT_HOST_RG[0] = usb_host_ptr->NO_HUB | bType;
    usb_host_ptr->DEV_PTR->ADDRESS = usb_host_ptr->SPEED | bAddress;
    usb_host_ptr->DEV_PTR->TOKEN = bToken;
}

void _usb_host_vusb11_process_token_done(_usb_host_handle handle, uint_8 direction, uint_32 error_status) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    UNUSED MP_STRUCT mp_struct;
    uint_32 status;
    HOST_BDT_STRUCT copy_bdt;
    PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr = &usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->CURRENT_PIPE_ID];
    uint_16 bc;
    uint_8 state;

    if (direction == USB_SEND) {
        _usb_bdt_copy_swab_host(BDT_FOR_DIR(usb_host_ptr->USB_BDT_PAGE, usb_host_ptr->USB_OUT), &copy_bdt);
    } else {
        _usb_bdt_copy_swab_host(BDT_FOR_DIR(usb_host_ptr->USB_BDT_PAGE, usb_host_ptr->USB_IN), &copy_bdt);
    }

    if (usb_host_ptr->USB_TOKEN_BUFFERED) {
        if (!usb_host_ptr->NEXT_PKT_QUEUED) {
            usb_host_ptr->USB_TOKEN_BUFFERED = FALSE;
        } else if (usb_host_ptr->CURRENT_PIPE_ID != usb_host_ptr->NEXT_PIPE_ID) {
            usb_host_ptr->USB_TOKEN_BUFFERED = FALSE;
        } else {
            usb_host_ptr->NEXT_PKT_QUEUED = FALSE;
        }
    } else if (usb_host_ptr->NEXT_PKT_QUEUED) {
        if (usb_host_ptr->CURRENT_PIPE_ID == usb_host_ptr->NEXT_PIPE_ID) {
            usb_host_ptr->NEXT_PKT_QUEUED = FALSE;
        }
    }

    bc = copy_bdt.BC;
    // In host mode, this field is used to report the last returned PID or a transfer status indication
    status = BDTCTL_PID(copy_bdt.PID) << 2;

    if (pipe_descr_ptr->PIPETYPE != USB_ISOCHRONOUS_PIPE && (status == (USB_TOKEN_TOKENPID_TIMEOUT << 2) ||
        status == (USB_TOKEN_TOKENPID_NAK << 2) || status == (USB_TOKEN_TOKENPID_MDATA << 2) || error_status != 0)) {
        pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;

        if (status == (USB_TOKEN_TOKENPID_NAK << 2) && pipe_descr_ptr->CURRENT_NAK_COUNT != 0) {
            pipe_descr_ptr->CURRENT_NAK_COUNT--;
            if (pipe_descr_ptr->CURRENT_NAK_COUNT == 0) {
                pipe_descr_ptr->CURRENT_INTERVAL = 1;
            }
        }

        if (status & (1 << 2)) {
            _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_0xFC, pipe_descr_ptr->SOFAR);
        }

        if (usb_host_ptr->NEXT_PKT_QUEUED) {
            usb_host_ptr->CURRENT_PIPE_ID = usb_host_ptr->NEXT_PIPE_ID;
            _usb_host_vusb11_resend_token(usb_host_ptr, usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->NEXT_PIPE_ID].DIRECTION);
        }
        return;
    }

    pipe_descr_ptr->CURRENT_NAK_COUNT = pipe_descr_ptr->NAK_COUNT;

    if (pipe_descr_ptr->PIPETYPE != USB_CONTROL_PIPE) {
        if (direction == USB_SEND) {
            pipe_descr_ptr->TX1_PTR = &pipe_descr_ptr->TX1_PTR[bc];
        } else {
            pipe_descr_ptr->RX_PTR = &pipe_descr_ptr->RX_PTR[bc];
        }
        pipe_descr_ptr->TODO1 -= bc;
        pipe_descr_ptr->SOFAR += bc;
    } else {
        if (pipe_descr_ptr->SEND_PHASE && !pipe_descr_ptr->FIRST_PHASE) {
            pipe_descr_ptr->TX2_PTR = &pipe_descr_ptr->TX2_PTR[bc];
            pipe_descr_ptr->TODO2 -= bc;
            pipe_descr_ptr->SOFAR += bc;
        }
        if (direction == USB_RECV) {
            pipe_descr_ptr->RX_PTR = &pipe_descr_ptr->RX_PTR[bc];
            pipe_descr_ptr->TODO1 -= bc;
            pipe_descr_ptr->SOFAR += bc;
        }
    }

    if (usb_host_ptr->NEXT_PKT_QUEUED) {
        usb_host_ptr->CURRENT_PIPE_ID = usb_host_ptr->NEXT_PIPE_ID;
        _usb_host_vusb11_resend_token(usb_host_ptr, usb_host_ptr->PIPE_DESCRIPTOR_BASE_PTR[usb_host_ptr->NEXT_PIPE_ID].DIRECTION);
    }

    state = pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE;
    switch (state) {
        case 0:
            if ((pipe_descr_ptr->TODO1 == 0 || bc < pipe_descr_ptr->MAX_PKT_SIZE) &&
                (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE || pipe_descr_ptr->DONT_ZERO_TERMINATE || bc != pipe_descr_ptr->MAX_PKT_SIZE)) {
                pipe_descr_ptr->STATUS = 0;
                pipe_descr_ptr->PACKETPENDING = FALSE;
                _usb_host_update_current_head(usb_host_ptr, pipe_descr_ptr->PIPETYPE);
                _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(pipe_descr_ptr->PIPE_ID), pipe_descr_ptr->SOFAR);
            }
            break;
        case 1:
            if (pipe_descr_ptr->SEND_PHASE) {
                if (pipe_descr_ptr->TODO2 == 0) {
                    pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE = 2;
                    pipe_descr_ptr->MUSTSENDNULL = TRUE;
                    _usb_host_vusb11_send_in_token(usb_host_ptr, pipe_descr_ptr);
                } else {
                    _usb_host_vusb11_send_out_token(usb_host_ptr, pipe_descr_ptr);
                }
            } else {
                if (pipe_descr_ptr->TODO1 == 0 || (bc < pipe_descr_ptr->MAX_PKT_SIZE && !pipe_descr_ptr->FIRST_PHASE)) {
                    pipe_descr_ptr->MUSTSENDNULL = TRUE;
                    _usb_host_vusb11_send_out_token(usb_host_ptr, pipe_descr_ptr);
                    pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE++;
                } else {
                    _usb_host_vusb11_send_in_token(usb_host_ptr, pipe_descr_ptr);
                }
            }
            pipe_descr_ptr->FIRST_PHASE = FALSE;
            break;
        case 2:
            pipe_descr_ptr->PACKETPENDING = FALSE;
            pipe_descr_ptr->STATUS = 0;
            pipe_descr_ptr->SEND_PHASE = FALSE;
            _usb_host_update_current_head(usb_host_ptr, pipe_descr_ptr->PIPETYPE);
            _usb_host_call_service(usb_host_ptr, USB_HOST_SERVICE_PIPE(pipe_descr_ptr->PIPE_ID), pipe_descr_ptr->SOFAR);
            break;
    }
}

void _usb_host_vusb11_init_send_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_BDT_STRUCT_PTR bdt_ptr;

    bdt_ptr = &usb_host_ptr->USB_TEMP_SND_BDT;
    pipe_descr_ptr->STATUS = 3;
    bdt_ptr->ADDR.W = KDM_TO_PHYS(pipe_descr_ptr->TX1_PTR);
    bdt_ptr->PID = pipe_descr_ptr->NEXTDATA01 | USB_BD_OWN;

    pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;
    if (pipe_descr_ptr->MAX_PKT_SIZE < pipe_descr_ptr->TODO1) {
        usb_host_ptr->USB_TEMP_SND_BDT.BC = pipe_descr_ptr->MAX_PKT_SIZE;
    } else {
        usb_host_ptr->USB_TEMP_SND_BDT.BC = pipe_descr_ptr->TODO1;
    }

    if (!usb_host_ptr->USB_TOKEN_BUFFERED) {
        usb_host_ptr->CURRENT_PIPE_ID = pipe_descr_ptr->PIPE_ID;
        pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE = 0;
        _usb_host_buffer_bdt(usb_host_ptr, bdt_ptr);
        if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
            _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_CTL_DISABLE | USB_ENDPT_TX_EN);
        } else {
            _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
        }
        _usb_host_vusb11_resend_token(usb_host_ptr, USB_SEND);
    } else if (!usb_host_ptr->NEXT_PKT_QUEUED) {
        _usb_host_buffer_next_bdt(usb_host_ptr, bdt_ptr);
        if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
            _usb_host_buffer_next_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_CTL_DISABLE | USB_ENDPT_TX_EN);
        } else {
            _usb_host_buffer_next_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_OUT(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
        }
        usb_host_ptr->NEXT_PIPE_ID = pipe_descr_ptr->PIPE_ID;
    }
}

void _usb_host_vusb11_init_recv_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_BDT_STRUCT_PTR bdt_ptr = &usb_host_ptr->USB_TEMP_RCV_BDT;

    pipe_descr_ptr->STATUS = 3;

    bdt_ptr->ADDR.W = KDM_TO_PHYS(pipe_descr_ptr->RX_PTR);
    bdt_ptr->PID = pipe_descr_ptr->NEXTDATA01 | USB_BD_DTS | USB_BD_OWN;

    pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;

    if (pipe_descr_ptr->MAX_PKT_SIZE < pipe_descr_ptr->TODO1) {
        usb_host_ptr->USB_TEMP_RCV_BDT.BC = pipe_descr_ptr->MAX_PKT_SIZE;
    } else {
        usb_host_ptr->USB_TEMP_RCV_BDT.BC = pipe_descr_ptr->TODO1;
    }

    if (!usb_host_ptr->USB_TOKEN_BUFFERED) {
        pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE = 0;
        usb_host_ptr->CURRENT_PIPE_ID = pipe_descr_ptr->PIPE_ID;
        _usb_host_buffer_bdt(usb_host_ptr, bdt_ptr);
        if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
            _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_CTL_DISABLE | USB_ENDPT_RX_EN);
        } else {
            _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
        }
        _usb_host_vusb11_resend_token(usb_host_ptr, USB_RECV);
    } else if (!usb_host_ptr->NEXT_PKT_QUEUED) {
        _usb_host_buffer_next_bdt(usb_host_ptr, bdt_ptr);
        if (pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) {
            _usb_host_buffer_next_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_CTL_DISABLE | USB_ENDPT_RX_EN);
        } else {
            _usb_host_buffer_next_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_IN(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
        }
        usb_host_ptr->NEXT_PIPE_ID = pipe_descr_ptr->PIPE_ID;
    }
}

void _usb_host_vusb11_init_setup_bdt(_usb_host_handle handle, PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_BDT_STRUCT_PTR real_bdt_ptr;
    HOST_BDT_STRUCT copy_bdt;

    pipe_descr_ptr->USB_PIPE_TRANSACTION_STATE = 1;
    pipe_descr_ptr->STATUS = 3;
    usb_host_ptr->CURRENT_PIPE_ID = pipe_descr_ptr->PIPE_ID;

    real_bdt_ptr = _usb_host_vusb11_get_next_out_bdt(usb_host_ptr);
    _usb_bdt_copy_swab_host(real_bdt_ptr, &copy_bdt);
    copy_bdt.BC = 8;
    copy_bdt.ADDR.W = KDM_TO_PHYS(pipe_descr_ptr->TX1_PTR);
    copy_bdt.PID = pipe_descr_ptr->NEXTDATA01 | USB_BD_OWN;
    _usb_bdt_copy_swab_host(&copy_bdt, real_bdt_ptr);

    pipe_descr_ptr->NEXTDATA01 ^= USB_BD_DATA01;
    _usb_host_buffer_bdt(usb_host_ptr, &copy_bdt);
    _usb_host_buffer_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_SETUP(0), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
    _usb_host_vusb11_send_token(usb_host_ptr, pipe_descr_ptr->ADDRESS, USB_TOKEN_SETUP(pipe_descr_ptr->EP), USB_ENDPT_RETRY_DIS | USB_ENDPT_RX_EN | USB_ENDPT_TX_EN | USB_ENDPT_HSHK_EN);
}

void _usb_host_vusb11_resend_token(_usb_host_handle handle, uint_8 bDirection) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
    HOST_BDT_STRUCT_PTR real_bdt_ptr;
    HOST_BDT_STRUCT copy_bdt;

    if (bDirection == USB_SEND) {
        real_bdt_ptr = _usb_host_vusb11_get_next_out_bdt(usb_host_ptr);
    } else {
        real_bdt_ptr = _usb_host_vusb11_get_next_in_bdt(usb_host_ptr);
    }

    _usb_bdt_copy_swab_host(real_bdt_ptr, &copy_bdt);

    if (usb_host_ptr->USB_TOKEN_BUFFERED) {
        copy_bdt.BC = usb_host_ptr->USB_BUFFERED_BDT.BC;
        copy_bdt.ADDR = usb_host_ptr->USB_BUFFERED_BDT.ADDR;
        copy_bdt.PID = usb_host_ptr->USB_BUFFERED_BDT.PID;
        _usb_bdt_copy_swab_host(&copy_bdt, real_bdt_ptr);

        _usb_host_vusb11_send_token(usb_host_ptr, usb_host_ptr->USB_BUFFERED_ADDRESS, usb_host_ptr->USB_BUFFERED_TOKEN, usb_host_ptr->USB_BUFFERED_TYPE);
        _usb_host_buffer_token(usb_host_ptr, usb_host_ptr->USB_BUFFERED_ADDRESS, usb_host_ptr->USB_BUFFERED_TOKEN, usb_host_ptr->USB_BUFFERED_TYPE);
    } else if (usb_host_ptr->NEXT_PKT_QUEUED) {
        copy_bdt.BC = usb_host_ptr->USB_NEXT_BUFFERED_BDT.BC;
        copy_bdt.ADDR = usb_host_ptr->USB_NEXT_BUFFERED_BDT.ADDR;
        copy_bdt.PID = usb_host_ptr->USB_NEXT_BUFFERED_BDT.PID;
        _usb_bdt_copy_swab_host(&copy_bdt, real_bdt_ptr);

        _usb_host_vusb11_send_token(usb_host_ptr, usb_host_ptr->USB_NEXT_BUFFERED_ADDRESS, usb_host_ptr->USB_NEXT_BUFFERED_TOKEN, usb_host_ptr->USB_NEXT_BUFFERED_TYPE);
        _usb_host_buffer_next_token(usb_host_ptr, usb_host_ptr->USB_NEXT_BUFFERED_ADDRESS, usb_host_ptr->USB_NEXT_BUFFERED_TOKEN, usb_host_ptr->USB_NEXT_BUFFERED_TYPE);
    }
}

void _usb_host_buffer_bdt(_usb_host_handle handle, HOST_BDT_STRUCT* BDT_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_BUFFERED_BDT.PID = BDT_ptr->PID;
    usb_host_ptr->USB_BUFFERED_BDT.BC = BDT_ptr->BC;
    usb_host_ptr->USB_BUFFERED_BDT.ADDR.W = BDT_ptr->ADDR.W;
}

void _usb_host_buffer_next_bdt(_usb_host_handle handle, HOST_BDT_STRUCT* BDT_ptr) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_NEXT_BUFFERED_BDT.PID = BDT_ptr->PID;
    usb_host_ptr->USB_NEXT_BUFFERED_BDT.BC = BDT_ptr->BC;
    usb_host_ptr->USB_NEXT_BUFFERED_BDT.ADDR.W = BDT_ptr->ADDR.W;
}

void _usb_host_buffer_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_BUFFERED_ADDRESS = bAddress;
    usb_host_ptr->USB_BUFFERED_TOKEN = bToken;
    usb_host_ptr->USB_BUFFERED_TYPE = bType;
    usb_host_ptr->USB_TOKEN_BUFFERED = TRUE;
}

void _usb_host_buffer_next_token(_usb_host_handle handle, uint_8 bAddress, uint_8 bToken, uint_8 bType) {
    USB_HOST_STATE_STRUCT_PTR usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

    usb_host_ptr->USB_NEXT_BUFFERED_ADDRESS = bAddress;
    usb_host_ptr->USB_NEXT_BUFFERED_TOKEN = bToken;
    usb_host_ptr->USB_NEXT_BUFFERED_TYPE = bType;
    usb_host_ptr->NEXT_PKT_QUEUED = TRUE;
}
