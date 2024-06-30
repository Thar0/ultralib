#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

uint_8 _usb_dci_vusb11_init(_usb_device_handle handle) {
    uint_32 i;
    uint_32 which;
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    which = usb_dev_ptr->CTLR_NUM;

    // Set register addresses
    usb_dev_ptr->USB = (volatile USB_STRUCT_PTR)PHYS_TO_K1(USB_REG_GRP0(which));
    usb_dev_ptr->ENDPT_REGS = (volatile USB_REGISTER_PTR)((uint_8_ptr)usb_dev_ptr->USB + sizeof(*usb_dev_ptr->USB));
    usb_dev_ptr->USB_BDT_PAGE = (VUSB_ENDPOINT_BDT_STRUCT_PTR)PHYS_TO_K1(USB_BDT_BUF(which, 0));

    // Disable control and interrupts
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_DISABLE;
    usb_dev_ptr->USB->INTENABLE = USB_INTEN_NONE;
    // Ack all interrupts and errors
    usb_dev_ptr->USB->INTSTATUS = USB_ISTAT_ALL;
    usb_dev_ptr->USB->ERRORENABLE = USB_ERREN_ALL;

    // Set state
    usb_dev_ptr->USB_STATE = USB_SELF_POWERED | USB_REMOTE_WAKEUP;
    usb_dev_ptr->USB_CURR_CONFIG = 0;

    // Set BDT page registers
    usb_dev_ptr->USB->BDTPAGE1 = USB_BDTPAGE1_ADDR(KDM_TO_PHYS(usb_dev_ptr->USB_BDT_PAGE));
    usb_dev_ptr->USB->BDTPAGE2 = USB_BDTPAGE2_ADDR(KDM_TO_PHYS(usb_dev_ptr->USB_BDT_PAGE));
    usb_dev_ptr->USB->BDTPAGE3 = USB_BDTPAGE3_ADDR(KDM_TO_PHYS(usb_dev_ptr->USB_BDT_PAGE));

    // Reset endpoint registers
    for (i = 0; i < usb_dev_ptr->MAX_ENDPOINTS; i++) {
        usb_dev_ptr->ENDPT_REGS[i] = 0;
    }

    usb_dev_ptr->USB_SOF_COUNT = (uint_16)-1;

    // Set control and enable the USB reset interrupt
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_DISABLE | USB_CTL_ODD_RST;
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_EN;
    usb_dev_ptr->SERVICE_HEAD_PTR = NULL;
    usb_dev_ptr->USB->INTENABLE = USB_INTEN_USBRSTEN;

    return USB_DEV_ERR_OK;
}

// USB Device Mode Interrupt Service Routine (ISR)
void _usb_dci_vusb11_isr(_usb_device_handle handle) {
    XD_STRUCT_PTR pxd;
    BDT_STRUCT_PTR temp_BDT_PTR;
    BDT_STRUCT temp_bdt;
    BDT_STRUCT copy_bdt;
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    uint_8 direction;
    uint_8 status;
    uint_16 wBytes;
    int owns;

    while (TRUE) {
        // Clear error status?
        status = usb_dev_ptr->USB->ERRORSTATUS;
        usb_dev_ptr->USB->ERRORSTATUS = status;

        // Get raised USB interrupts
        status = usb_dev_ptr->USB->INTSTATUS & usb_dev_ptr->USB->INTENABLE;
        if (status == 0) {
            // No interrupts left to service, exit
            return;
        }

        if ((status & (USB_ISTAT_SOFTOK | USB_ISTAT_TOKEN_DONE)) == USB_ISTAT_SOFTOK) {
            // Start-of-Frame Token received but no Token Done? Check every recv/send even/odd BD on endpoint 2?
            // Start-of-Frame Token arrives approx. every 1ms, intended for keeping sync for isochronous endpoints.
            // Since this is hardcoded to EP2, may be left-over debug code? (rdb_slave is over EP2)
            // What does it mean to check if an endpoint had unserviced transfers every ~1ms, if not some debug feature?
            int error = 0;

            usb_dev_ptr->BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[2][USB_RECV][USB_BD_EVEN];
            _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &copy_bdt);
            wBytes = copy_bdt.BC;
            owns = BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL) != 0;
            if (!owns && wBytes != 0) {
                error = 1;
            }

            usb_dev_ptr->BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[2][USB_RECV][USB_BD_ODD];
            _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &copy_bdt);
            wBytes = copy_bdt.BC;
            owns = BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL) != 0;
            if (!owns && wBytes != 0) {
                error = 2;
            }

            usb_dev_ptr->BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[2][USB_SEND][USB_BD_EVEN];
            _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &copy_bdt);
            wBytes = copy_bdt.BC;
            owns = BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL) != 0;
            if (!owns && wBytes != 0) {
                error = 3;
            }

            usb_dev_ptr->BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[2][USB_SEND][USB_BD_ODD];
            _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &copy_bdt);
            wBytes = copy_bdt.BC;
            owns = BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL) != 0;
            if (!owns && wBytes != 0) {
                error = 4;
            }

            if (error != 0) {
                // If any EP2 BDs are not owned by the SIE and have non-zero byte counts?
                int newstatus = usb_dev_ptr->USB->INTSTATUS & usb_dev_ptr->USB->INTENABLE;

                if (!(newstatus & USB_ISTAT_TOKEN_DONE)) {
                    // If there is no token done, stuff one in anyway? Why?
                    status |= USB_ISTAT_TOKEN_DONE;
                }
            }
        }

        if (status & USB_ISTAT_TOKEN_DONE) {
            // Token done part 1
            // The STATUS register is actually the head of a 4-entry FIFO, so the SIE can keep working while previous
            // transfers are handled. The contents of STATUS is only valid when TOKEN_DONE is raised in ISTAT, the FIFO
            // is advanced whenever the TOKEN_DONE interrupt is ack'd. Read STATUS here before interrupts are ack'd.
            // (MC9S08JS16RM datasheet sections 15.3.5 and 15.3.9)
            uint_8 temp_status = usb_dev_ptr->USB->STATUS;
            uint_8 odd;

            // Determine which endpoint the transfer is for and the BD to access
            direction = USB_STAT_TX(temp_status);
            odd = USB_STAT_ODD(temp_status);
            usb_dev_ptr->EP = USB_STAT_EP(temp_status);
            usb_dev_ptr->BDT_PTR = &usb_dev_ptr->USB_BDT_PAGE[usb_dev_ptr->EP][direction][odd];

            // Fetch the BD from the BDT, and the number of bytes
            _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &copy_bdt);
            wBytes = copy_bdt.BC;
        }

        // Ack interrupts
        usb_dev_ptr->USB->INTSTATUS = status;

        if (status & USB_ISTAT_RST) {
            // Bus reset
            _usb_dci_vusb11_bus_reset(handle);
            _usb_device_call_service(handle, USB_SERVICE_RESET_EP0, FALSE, USB_RECV, NULL, 0);

            // Queue a transfer to receive a control setup on endpoint 0
            usb_dev_ptr->XDRECV[0].SETUP_BUFFER_QUEUED = FALSE;
            usb_dev_ptr->XDSEND[0].SETUP_BUFFER_QUEUED = FALSE;
            _usb_device_recv_data(handle, 0, (uchar_ptr)usb_dev_ptr->XDRECV[0].SETUP_BUFFER, sizeof(usb_dev_ptr->XDRECV[0].SETUP_BUFFER));
            usb_dev_ptr->XDRECV[0].SETUP_BUFFER_QUEUED = TRUE;
            usb_dev_ptr->XDSEND[0].SETUP_BUFFER_QUEUED = TRUE;
            break;
        }

        if (status & USB_ISTAT_TOKEN_DONE) {
            // Token done part 2
            if (BDTCTL_PID(copy_bdt.REGISTER.BDTCTL) == USB_TOKEN_TOKENPID_SETUP && (usb_dev_ptr->USB->CONTROL & USB_CTL_TX_SUSPEND_TOKEN_BUSY)) {
                // Suspend received

                // Unstall send (even) BD?
                _usb_bdt_copy_swab_device(&usb_dev_ptr->USB_BDT_PAGE[usb_dev_ptr->EP][USB_SEND][USB_BD_EVEN], &temp_bdt);
                temp_BDT_PTR = &temp_bdt;

                if (BDTCTL_OWNS(temp_BDT_PTR->REGISTER.BDTCTL)) {
                    if (temp_BDT_PTR->REGISTER.BDTCTL & USB_BD_STALL) {
                        temp_BDT_PTR->REGISTER.BDTCTL &= ~(USB_BD_OWN | USB_BD_STALL);
                        _usb_bdt_copy_swab_device(&temp_bdt, &usb_dev_ptr->USB_BDT_PAGE[usb_dev_ptr->EP][USB_SEND][USB_BD_EVEN]);
                    }
                }

                // Unstall send (odd) BD?
                _usb_bdt_copy_swab_device(&usb_dev_ptr->USB_BDT_PAGE[usb_dev_ptr->EP][USB_SEND][USB_BD_ODD], &temp_bdt);
                temp_BDT_PTR = &temp_bdt;

                if (BDTCTL_OWNS(temp_BDT_PTR->REGISTER.BDTCTL)) {
                    if (temp_BDT_PTR->REGISTER.BDTCTL & USB_BD_STALL) {
                        temp_BDT_PTR->REGISTER.BDTCTL &= ~(USB_BD_OWN | USB_BD_STALL);
                        _usb_bdt_copy_swab_device(&temp_bdt, &usb_dev_ptr->USB_BDT_PAGE[usb_dev_ptr->EP][USB_SEND][USB_BD_ODD]);
                    }
                }

                // Unstall endpoint
                usb_dev_ptr->ENDPT_REGS[usb_dev_ptr->EP] &= ~USB_ENDPT_STALLED;
                usb_dev_ptr->XDSEND[usb_dev_ptr->EP].CTL &= ~USB_ENDPT_STALLED;
                usb_dev_ptr->XDRECV[usb_dev_ptr->EP].CTL &= ~USB_ENDPT_STALLED;

                // Cancel any pending send transfer
                if (usb_dev_ptr->XDSEND[usb_dev_ptr->EP].PACKETPENDING != 0) {
                    _usb_device_cancel_transfer(handle, usb_dev_ptr->EP, USB_SEND);
                }

                usb_dev_ptr->USB->CONTROL &= ~USB_CTL_TX_SUSPEND_TOKEN_BUSY;
            }

            if (direction == USB_SEND) {
                pxd = &usb_dev_ptr->XDSEND[usb_dev_ptr->EP];
            } else {
                pxd = &usb_dev_ptr->XDRECV[usb_dev_ptr->EP];
            }

            // Update transfer progress
            pxd->SOFAR += wBytes;
            pxd->UNACKNOWLEDGEDBYTES -= wBytes;

            if (BDTCTL_PID(copy_bdt.REGISTER.BDTCTL) == USB_TOKEN_TOKENPID_SETUP || !(pxd->CTL & USB_ENDPT_HSHK_EN)) {
                // Is a setup packet or isochronous, these always fit in 1 packet and require no NULL termination

                // Unstall endpoint 0
                usb_dev_ptr->ENDPT_REGS[0] &= ~USB_ENDPT_STALLED;

                // Reset the BD for this transfer
                copy_bdt.REGISTER.BDTCTL = 0;
                copy_bdt.BC = 0;
                copy_bdt.ADDRESS = NULL;
                pxd->PACKETPENDING--;
                _usb_bdt_copy_swab_device(&copy_bdt, usb_dev_ptr->BDT_PTR);

                if (pxd->PACKETPENDING != 0) {
                    // Clear any buffered BD for this transfer if there is one? Since this was a single-packet transfer there
                    // should be no next transfer?
                    _usb_bdt_copy_swab_device(BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR), &copy_bdt);
                    if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
                        pxd->NEXTODDEVEN ^= 1;
                        pxd->NEXTDATA01 ^= USB_BD_DATA01;
                        pxd->PACKETPENDING--;
                    }
                    copy_bdt.REGISTER.BDTCTL = 0;
                    copy_bdt.BC = 0;
                    copy_bdt.ADDRESS = NULL;
                    _usb_bdt_copy_swab_device(&copy_bdt, BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR));
                }

                pxd->SETUP_BUFFER_QUEUED = FALSE;

                // Call out to endpoint service
                pxd->BDTCTL = 0;
                pxd->STATUS = USB_STATUS_IDLE;
                _usb_device_call_service(handle, USB_SERVICE_EP(usb_dev_ptr->EP), !!(pxd->CTL & USB_ENDPT_HSHK_EN), direction, pxd->STARTADDRESS, pxd->SOFAR);

                if (pxd->CTL & USB_ENDPT_HSHK_EN) {
                    // Is a setup packet, isochronous does not have handshakes
                    pxd->SETUP_BUFFER_QUEUED = FALSE;

                    if (direction == USB_SEND) {
                        usb_dev_ptr->XDRECV[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = FALSE;
                    } else {
                        usb_dev_ptr->XDSEND[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = FALSE;
                    }

                    // Queue a new transfer to receive next setup packet
                    _usb_device_recv_data(handle, usb_dev_ptr->EP,
                                          (uchar_ptr)usb_dev_ptr->XDRECV[usb_dev_ptr->EP].SETUP_BUFFER,
                                          sizeof(usb_dev_ptr->XDRECV[usb_dev_ptr->EP].SETUP_BUFFER));
                    pxd->SETUP_BUFFER_QUEUED = TRUE;

                    if (direction == USB_SEND) {
                        usb_dev_ptr->XDRECV[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = TRUE;
                    } else {
                        usb_dev_ptr->XDSEND[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = TRUE;
                    }
                }
            } else if (wBytes == pxd->MAXPACKET) {
                // Reset the BD for this transfer
                copy_bdt.REGISTER.BDTCTL = 0;
                copy_bdt.BC = 0;
                copy_bdt.ADDRESS = NULL;
                pxd->PACKETPENDING--;
                _usb_bdt_copy_swab_device(&copy_bdt, usb_dev_ptr->BDT_PTR);

                if (pxd->TODO != 0 || pxd->MUSTSENDNULL) {
                    // Still data left (or must send NULL termination, achieved by submitting a BD with BC=0), queue
                    // another BD for another packet transfer
                    pxd->STATUS = USB_STATUS_TRANSFER_IN_PROGRESS;
                    _usb_dci_vusb11_submit_BDT(usb_dev_ptr->BDT_PTR, pxd);
                } else if (pxd->PACKETPENDING != 0 && pxd->UNACKNOWLEDGEDBYTES != 0) {
                    // Still in progress, can't clear buffered BD or call the service yet?
                    pxd->STATUS = USB_STATUS_TRANSFER_IN_PROGRESS;
                } else {
                    // Transfer done for transfers that were a multiple of the packet size?
                    if (pxd->PACKETPENDING != 0) {
                        // Clear any buffered BD for this transfer if there is one? Since the transfer is done
                        _usb_bdt_copy_swab_device(BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR), &copy_bdt);
                        if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
                            pxd->NEXTODDEVEN ^= 1;
                            pxd->NEXTDATA01 ^= USB_BD_DATA01;
                            pxd->PACKETPENDING--;
                        }
                        copy_bdt.REGISTER.BDTCTL = 0;
                        copy_bdt.BC = 0;
                        copy_bdt.ADDRESS = NULL;
                        _usb_bdt_copy_swab_device(&copy_bdt, BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR));
                    }

                    // Call out to endpoint service
                    pxd->BDTCTL = 0;
                    pxd->STATUS = USB_STATUS_IDLE;
                    _usb_device_call_service(handle, USB_SERVICE_EP(usb_dev_ptr->EP), FALSE, direction, pxd->STARTADDRESS, pxd->SOFAR);
                }
            } else if (wBytes < pxd->MAXPACKET) {
                // Reset the BD for this transfer
                copy_bdt.REGISTER.BDTCTL = 0;
                copy_bdt.BC = 0;
                copy_bdt.ADDRESS = NULL;
                pxd->PACKETPENDING--;
                _usb_bdt_copy_swab_device(&copy_bdt, usb_dev_ptr->BDT_PTR);

                // Transfer must be guaranteed done since it's less than the max packet size?
                if (pxd->PACKETPENDING != 0) {
                    // Clear any buffered BD for this transfer if there is one? Since the transfer is done
                    _usb_bdt_copy_swab_device(BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR), &copy_bdt);
                    if (BDTCTL_OWNS(copy_bdt.REGISTER.BDTCTL)) {
                        pxd->NEXTODDEVEN ^= 1;
                        pxd->NEXTDATA01 ^= USB_BD_DATA01;
                        pxd->PACKETPENDING--;
                    }
                    copy_bdt.REGISTER.BDTCTL = 0;
                    copy_bdt.BC = 0;
                    copy_bdt.ADDRESS = NULL;
                    _usb_bdt_copy_swab_device(&copy_bdt, BDT_SWAP_ODD_EVEN(usb_dev_ptr->BDT_PTR));
                }

                // Call out to endpoint service
                pxd->BDTCTL = 0;
                pxd->STATUS = USB_STATUS_IDLE;
                _usb_device_call_service(handle, USB_SERVICE_EP(usb_dev_ptr->EP), FALSE, direction, pxd->STARTADDRESS, pxd->SOFAR);

                if (pxd->TYPE == USB_CONTROL_PIPE && direction == USB_RECV && wBytes == 0) {
                    // If this is a control pipe that receives from the host, queue a transfer to receive the next control setup

                    pxd->SETUP_BUFFER_QUEUED = FALSE;
                    usb_dev_ptr->XDSEND[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = FALSE;

                    _usb_device_recv_data(handle, usb_dev_ptr->EP, (uchar_ptr)pxd->SETUP_BUFFER, sizeof(pxd->SETUP_BUFFER));

                    pxd->SETUP_BUFFER_QUEUED = TRUE;
                    usb_dev_ptr->XDSEND[usb_dev_ptr->EP].SETUP_BUFFER_QUEUED = TRUE;
                }
            }
        }
        // Handle other events, calling out to the appropriate service handler
        if (status & USB_ISTAT_RESUME) {
            _usb_device_call_service(handle, USB_SERVICE_RESUME, FALSE, USB_RECV, NULL, 0);
        }
        if (status & USB_ISTAT_SOFTOK) {
            _usb_device_call_service(handle, USB_SERVICE_SOF, FALSE, USB_RECV, NULL, 0);
        }
        if (status & USB_ISTAT_SLEEP) {
            _usb_device_call_service(handle, USB_SERVICE_SUSPEND, FALSE, USB_RECV, NULL, 0);
        }
        if (status & USB_ISTAT_ERROR) {
            _usb_device_call_service(handle, USB_SERVICE_ERROR, FALSE, USB_RECV, NULL, usb_dev_ptr->USB->ERRORSTATUS);
        }
        if (status & USB_ISTAT_STALL) {
            _usb_device_call_service(handle, USB_SERVICE_STALL, FALSE, USB_RECV, NULL, usb_dev_ptr->USB->INTSTATUS & USB_ISTAT_STALL);
        }
    }
}

void _usb_dci_vusb11_submit_BDT(BDT_STRUCT_PTR pBDT, XD_STRUCT_PTR pxd) {
    uint_16 wActualBytes;
    BDT_STRUCT pid;

    // Swap odd/even BD
    pxd->NEXTODDEVEN ^= 1;

    // Obtain BD from BDT
    _usb_bdt_copy_swab_device(pBDT, &pid);

    // Set DRAM address of send/recv buffer for DMA
    pid.ADDRESS = (uint_8_ptr)KDM_TO_PHYS(pxd->NEXTADDRESS);

    // Clamp to max packet size: actual bytes = MIN(TODO, MAXPACKET)
    wActualBytes = pxd->TODO;
    if (pxd->TODO > pxd->MAXPACKET) {
        wActualBytes = pxd->MAXPACKET;
    }

    // Invalidate the data cache at the data DRAM address
    osInvalDCache(pxd->NEXTADDRESS, wActualBytes);

    // Configure BDTCTL, to be owned by the SIE
    pid.REGISTER.BDTCTL = pxd->BDTCTL;
    pid.REGISTER.BDTCTL |= USB_BD_OWN | USB_BD_DATA0 | USB_BD_DTS;
    if ((pxd->BDTCTL & USB_BD_DATA01) == USB_BD_DATA0) {
        pid.REGISTER.BDTCTL &= ~USB_BD_DATA01; // Clears DATA01 bit
        pid.REGISTER.BDTCTL |= pxd->NEXTDATA01; // Uses whichever DATA0/DATA1 was next
    } else {
        pxd->BDTCTL &= ~USB_BD_DATA01; // Set DATA0
        pxd->NEXTDATA01 = USB_BD_DATA1; // Next is DATA1
    }
    // Swap data0/1 for next packet
    pxd->NEXTDATA01 ^= USB_BD_DATA01;
    // Update number of pending packets
    pxd->PACKETPENDING++;
    // If a handshake is required (non-isochronous) and the transfer size is equal to the maximum packet size, a NULL
    // terminator will be required if:
    //  - Not a control pipe and requires NULL
    //  - A control pipe and a setup buffer is queued
    pxd->MUSTSENDNULL = FALSE;
    if ((pxd->CTL & USB_ENDPT_HSHK_EN) && pxd->TODO == pxd->MAXPACKET &&
        ((pxd->TYPE != USB_CONTROL_PIPE && !pxd->DONT_ZERO_TERMINATE) ||
         (pxd->TYPE == USB_CONTROL_PIPE && pxd->SETUP_BUFFER_QUEUED))) {
        pxd->MUSTSENDNULL = TRUE;
    }
    // Update address
    pxd->NEXTADDRESS += wActualBytes;
    // Write number of bytes to transfer
    pid.BC = wActualBytes;
    // Decrement remaining transfer length
    pxd->TODO -= wActualBytes;

    // Emplace new BD
    _usb_bdt_copy_swab_device(&pid, pBDT);
}

void _usb_dci_vusb11_read_setup(_usb_device_handle handle, uint_8 ep_num, uchar_ptr buff_ptr) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    BDT_STRUCT bdt;

    // Obtain BD from BDT
    _usb_bdt_copy_swab_device(usb_dev_ptr->BDT_PTR, &bdt);

    // Copy from DMA buffer to destination
    bcopy((void*)PHYS_TO_K1(bdt.ADDRESS), buff_ptr, sizeof(SETUP_STRUCT));
}

void _usb_dci_vusb11_submit_transfer(_usb_device_handle handle, uint_8 direction, uint_8 ep) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    XD_STRUCT_PTR pxd;

    if (direction == USB_SEND) {
        pxd = &usb_dev_ptr->XDSEND[ep];
    } else {
        pxd = &usb_dev_ptr->XDRECV[ep];
    }

    // Update BDTCTL
    if (pxd->TYPE == USB_CONTROL_PIPE) {
        if ((pxd->CTL & USB_ENDPT_STALLED) && direction == USB_RECV) {
            pxd->BDTCTL = USB_BD_DATA1 | USB_BD_STALL;
        } else {
            pxd->BDTCTL = USB_BD_DATA1;
        }
    } else {
        pxd->BDTCTL = USB_BD_DATA0;
    }
    // To be owned by the SIE
    pxd->BDTCTL |= USB_BD_OWN;

    // Submit new BD for (ep,direction,odd/even)
    _usb_dci_vusb11_submit_BDT(&usb_dev_ptr->USB_BDT_PAGE[ep][direction][pxd->NEXTODDEVEN & 1], pxd);
    if (pxd->TODO != 0 || pxd->MUSTSENDNULL) {
        // Submit new BD for (ep,direction,even/odd) if the transfer is greater than one packet to start
        // double-buffering transfers
        _usb_dci_vusb11_submit_BDT(&usb_dev_ptr->USB_BDT_PAGE[ep][direction][pxd->NEXTODDEVEN & 1], pxd);
    }
}

void _usb_dci_vusb11_bus_reset(_usb_device_handle handle) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;
    uchar i;

    __Usb_Reset_Count[usb_dev_ptr->CTLR_NUM]++;

    // Clear the BDT
    bzero(usb_dev_ptr->USB_BDT_PAGE, USB_NUM_ENDPOINTS * sizeof(*usb_dev_ptr->USB_BDT_PAGE));

    // Reset the device state
    usb_dev_ptr->USB_STATE = USB_REMOTE_WAKEUP;
    usb_dev_ptr->USB_CURR_CONFIG = 0;
    // Clearing the TOKEN_DONE interrupt advances the 4-entry STATUS FIFO, so clearing it 4 times like this empties the
    // STATUS FIFO completely.
    // (MC9S08JS16RM datasheet sections 15.3.5 and 15.3.9)
    usb_dev_ptr->USB->INTSTATUS = USB_ISTAT_TOKEN_DONE;
    usb_dev_ptr->USB->INTSTATUS = USB_ISTAT_TOKEN_DONE;
    usb_dev_ptr->USB->INTSTATUS = USB_ISTAT_TOKEN_DONE;
    usb_dev_ptr->USB->INTSTATUS = USB_ISTAT_TOKEN_DONE;
    // Clear the bus address
    usb_dev_ptr->USB->ADDRESS = USB_ADDR_FSEN | USB_ADDR_ADDR(0);
    // Reset control
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_EN | USB_CTL_ODD_RST;

    // Reset all transfer descriptors
    bzero(usb_dev_ptr->XDSEND, usb_dev_ptr->MAX_ENDPOINTS * sizeof(*usb_dev_ptr->XDSEND));
    bzero(usb_dev_ptr->XDRECV, usb_dev_ptr->MAX_ENDPOINTS * sizeof(*usb_dev_ptr->XDRECV));
    for (i = 0; i < usb_dev_ptr->MAX_ENDPOINTS; i++) {
        usb_dev_ptr->XDSEND[i].STATUS = USB_STATUS_DISABLED;
        usb_dev_ptr->XDRECV[i].STATUS = USB_STATUS_DISABLED;
    }

    // Set control and enable all interrupts
    usb_dev_ptr->USB->CONTROL = USB_CTL_USB_EN;
    usb_dev_ptr->USB->INTENABLE = USB_INTEN_ALL;
}

// Initialize an endpoint via the corresponding endpoint hardware register, reflect the state in the transfer descriptor
void _usb_dci_vusb11_init_endpoint(_usb_device_handle handle, uint_8 ep, XD_STRUCT_PTR pxd) {
    USB_DEV_STATE_STRUCT_PTR usb_dev_ptr = (USB_DEV_STATE_STRUCT_PTR)handle;

    switch (pxd->TYPE) {
        case USB_ISOCHRONOUS_PIPE:
            pxd->CTL = USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS;
            usb_dev_ptr->ENDPT_REGS[ep] = USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS;
            break;
        case USB_CONTROL_PIPE:
            pxd->CTL = USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_RETRY_DIS;
            usb_dev_ptr->ENDPT_REGS[ep] = USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_RETRY_DIS;
            break;
        case USB_INTERRUPT_PIPE:
        case USB_BULK_PIPE:
            pxd->CTL = USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS;
            usb_dev_ptr->ENDPT_REGS[ep] = USB_ENDPT_HSHK_EN|USB_ENDPT_TX_EN|USB_ENDPT_RX_EN|USB_ENDPT_CTL_DISABLE|USB_ENDPT_RETRY_DIS;
            break;
    }
}
