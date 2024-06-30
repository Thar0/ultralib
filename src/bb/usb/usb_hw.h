#ifndef USB_HW_H_
#define USB_HW_H_

// This is based on datasheets for microcontrollers such as MC9S08JM60, MC9S08JS* by Freescale/NXP, which appears to be
// a very similar hardware interface. Some notable differences are that all 16 endpoints on iQue are bidirectional and
// double-buffered, other differences are not known.

#define USB_NUM_ENDPOINTS   16


#define USB_BASE_REG_0          0x04900000
#define USB_BASE_REG_1          0x04A00000

#define USB_BASE_REG(which)     (((which) == 0) ? USB_BASE_REG_0 : USB_BASE_REG_1)
#define USB_BASE_REG_ALT(which) (((which) != 0) ? USB_BASE_REG_1 : USB_BASE_REG_0)


// [7:0] Peripheral ID
#define USB_PERID_REG(which)        (USB_BASE_REG(which) + 0x00)
// [7:0] Peripheral ID Bitwise Complement
#define USB_IDCOMP_REG(which)       (USB_BASE_REG(which) + 0x04)
// [7:0] Revision Number
#define USB_REV_REG(which)          (USB_BASE_REG(which) + 0x08)
// [7:3] IRQNUM         Assigned Interrupt Request Number
// [2:1] RESERVED1
// [0]   IEHOST         When this bit is set, the USB peripheral is operating in host mode.
#define USB_ADDINFO_REG(which)      (USB_BASE_REG(which) + 0x0C)

// OTG_INT_STAT
// [7] IDCHG            This bit is set when a change in the ID Signal from the USB connector is sensed.
// [6] ONEMSEC          This bit is set when the 1 millisecond timer expires.
// [5] LINE_STATE_CHG   This bit is set when the USB line state changes.
// [4] RESERVED4
// [3] SESSVLDCHG       This bit is set when a change in VBUS is detected indicating a session valid or a session no longer valid.
// [2] B_SESS_CHG       This bit is set when a change in VBUS is detected on a B device.
// [1] RESERVED1
// [0] AVBUSCHG         This bit is set when a change in VBUS is detected on an A device.
#define USB_OTGISTAT_REG(which)     (USB_BASE_REG(which)     + 0x10)
#define USB_OTGISTAT_REG_ALT(which) (USB_BASE_REG_ALT(which) + 0x10)
#define USB_OTGISTAT_IDCHG          (1 << 7)
#define USB_OTGISTAT_ONEMSEC        (1 << 6)
#define USB_OTGISTAT_LINE_STATE_CHG (1 << 5)
#define USB_OTGISTAT_RESERVED4      (1 << 4)
#define USB_OTGISTAT_SESSVLDCHG     (1 << 3)
#define USB_OTGISTAT_B_SESS_CHG     (1 << 2)
#define USB_OTGISTAT_RESERVED1      (1 << 1)
#define USB_OTGISTAT_AVBUSCHG       (1 << 0)
#define USB_OTGISTAT_ALL            0xFF
#define USB_OTGISTAT_NONE           0x00
// OTG_INT_ENABLE
// [7] IDEN             ID Interrupt Enable
// [6] ONEMSECEN        One Millisecond Interrupt Enable
// [5] LINESTATEEN      Line State Change Interrupt Enable
// [4] RESERVED4
// [3] SESSVLDEN        Session Valid Interrupt Enable
// [2] BSESSEN          B Session END Interrupt Enable
// [1] RESERVED1
// [0] AVBUSEN          A VBUS Valid Interrupt Enable
#define USB_OTGICR_REG(which)       (USB_BASE_REG(which) + 0x14)
#define USB_OTGICR_IDEN             (1 << 7)
#define USB_OTGICR_ONEMSECEN        (1 << 6)
#define USB_OTGICR_LINESTATEEN      (1 << 5)
#define USB_OTGICR_RESERVED4        (1 << 4)
#define USB_OTGICR_SESSVLDEN        (1 << 3)
#define USB_OTGICR_BSESSEN          (1 << 2)
#define USB_OTGICR_RESERVED1        (1 << 1)
#define USB_OTGICR_AVBUSEN          (1 << 0)
#define USB_OTGICR_ALL              0xFF
#define USB_OTGICR_NONE             0x00
// OTG_STAT
// [7] ID               Indicates the current state of the ID pin on the USB connector (0 = Type A, 1 = Type B or nothing)
// [6] ONEMSECEN        This bit is reserved for the 1ms count, but it is not useful to software.
// [5] LINESTATESTABLE  Indicates that the internal signals that control the LINE_STATE_CHG field of OTGISTAT are stable for at least 1 millisecond.
// [4] RESERVED4
// [3] SESS_VLD         The VBUS voltage is above(=1)/below(=0) the B session valid threshold
// [2] BSESSEND         The VBUS voltage is above(=0)/below(=1) the B session end threshold.
// [1] RESERVED1
// [0] AVBUSVLD         The VBUS voltage is above(=1)/below(=0) the A VBUS Valid threshold.
#define USB_OTGSTAT_REG(which)      (USB_BASE_REG(which)     + 0x18)
#define USB_OTGSTAT_REG_ALT(which)  (USB_BASE_REG_ALT(which) + 0x18)
#define USB_OTGSTAT_ID              (1 << 7)
#define USB_OTGSTAT_ONEMSECEN       (1 << 6)
#define USB_OTGSTAT_LINESTATESTABLE (1 << 5)
#define USB_OTGSTAT_RESERVED4       (1 << 4)
#define USB_OTGSTAT_SESS_VLD        (1 << 3)
#define USB_OTGSTAT_BSESSEND        (1 << 2)
#define USB_OTGSTAT_RESERVED1       (1 << 1)
#define USB_OTGSTAT_AVBUSVLD        (1 << 0)
// OTG_CTL
// [7]   DPHIGH         D+ Data Line pullup resistor enable
// [6]   RESERVED6
// [5]   DPLOW          D+ Data Line pull-down resistor enable
// [4]   DMLOW          D- Data Line pull-down resistor enable
// [3]   RESERVED3
// [2]   OTGEN          On-The-Go pullup/pulldown resistor enable
// [1:0] RESERVED0
#define USB_OTGCTL_REG(which)       (USB_BASE_REG(which) + 0x1C)
#define USB_OTGCTL_DPHIGH           (1 << 7)
#define USB_OTGCTL_RESERVED6        (1 << 6)
#define USB_OTGCTL_DPLOW            (1 << 5)
#define USB_OTGCTL_DMLOW            (1 << 4)
#define USB_OTGCTL_RESERVED3        (1 << 3)
#define USB_OTGCTL_OTGEN            (1 << 2)
#define USB_OTGCTL_RESERVED1        (1 << 1)
#define USB_OTGCTL_RESERVED0        (1 << 0)


#define USB_REG_GRP0(which)         (USB_BASE_REG(which) | 0x80)
#define USB_REG_GRP0_ALT(which)     (USB_BASE_REG_ALT(which) | 0x80)

// INTSTATUS
// [7] STALL            Stall Interrupt
// [6] ATTACH           Attach Interrupt
// [5] RESUME           Resume Interrupt
// [4] SLEEP            Sleep Interrupt
// [3] TOKDNE           Token Done Interrupt
// [2] SOFTOK           SOF Token Interrupt
// [1] ERROR            Error Interrupt
// [0] USBRST           Reset Interrupt
#define USB_ISTAT_REG(which)        (USB_BASE_REG(which) + 0x80)
#define USB_ISTAT_STALL             (1 << 7)
#define USB_ISTAT_ATTACH            (1 << 6)
#define USB_ISTAT_RESUME            (1 << 5)
#define USB_ISTAT_SLEEP             (1 << 4)
#define USB_ISTAT_TOKEN_DONE        (1 << 3)
#define USB_ISTAT_SOFTOK            (1 << 2)
#define USB_ISTAT_ERROR             (1 << 1)
#define USB_ISTAT_RST               (1 << 0)
#define USB_ISTAT_ALL               0xFF
#define USB_ISTAT_NONE              0x00
// INTENABLE
// [7] STALLEN          Stall Interrupt Enable
// [6] ATTACHEN         Attach Interrupt Enable
// [5] RESUMEEN         Resume Interrupt Enable
// [4] SLEEPEN          Sleep Interrupt Enable
// [3] TOKDNEEN         Token Done Interrupt Enable
// [2] SOFTOKEN         SOF Token Interrupt Enable
// [1] ERROREN          Error Interrupt Enable
// [0] USBRSTEN         Reset Interrupt Enable
#define USB_INTEN_REG(which)        (USB_BASE_REG(which) + 0x84)
#define USB_INTEN_STALLEN           (1 << 7)
#define USB_INTEN_ATTACHEN          (1 << 6)
#define USB_INTEN_RESUMEEN          (1 << 5)
#define USB_INTEN_SLEEPEN           (1 << 4)
#define USB_INTEN_TOKDNEEN          (1 << 3)
#define USB_INTEN_SOFTOKEN          (1 << 2)
#define USB_INTEN_ERROREN           (1 << 1)
#define USB_INTEN_USBRSTEN          (1 << 0)
#define USB_INTEN_ALL               0xFF
#define USB_INTEN_NONE              0x00
// ERRORSTATUS
// [7] BTSERR           This bit is set when a bit stuff error is detected.
// [6] RESERVED6
// [5] DMAERR           This bit is set if the USB Module has requested a DMA access to read a new BDT but has not been given the bus before it needs to receive or transmit data.
// [4] BTOERR           This bit is set when a bus turnaround timeout error occurs.
// [3] DFN8             This bit is set if the data field received was not a multiple of 8 bits in length.
// [2] CRC16            This bit is set when a data packet is rejected due to a CRC16 error.
// [1] CRC5EOF
// [0] PIDERR           This bit is set when the PID check field fails.
#define USB_ERRSTAT_REG(which)      (USB_BASE_REG(which) + 0x88)
#define USB_ERRSTAT_BTSERR          (1 << 7)
#define USB_ERRSTAT_RESERVED6       (1 << 6)
#define USB_ERRSTAT_DMAERR          (1 << 5)
#define USB_ERRSTAT_BTOERR          (1 << 4)
#define USB_ERRSTAT_DFN8            (1 << 3)
#define USB_ERRSTAT_CRC16           (1 << 2)
#define USB_ERRSTAT_CRC5EOF         (1 << 1)
#define USB_ERRSTAT_PIDERR          (1 << 0)
#define USB_ERRSTAT_ALL             0xFF
#define USB_ERRSTAT_NONE            0x00
// ERRORENABLE
// [7] BTSERREN         BTSERR Interrupt Enable
// [6] RESERVED6
// [5] DMAERREN         DMAERR Interrupt Enable
// [4] BTOERREN         BTOERR Interrupt Enable
// [3] DFN8EN           DFN8 Interrupt Enable
// [2] CRC16EN          CRC16 Interrupt Enable
// [1] CRC5EOFEN        CRC5/EOF Interrupt Enable
// [0] PIDERREN         PIDERR Interrupt Enable
#define USB_ERREN_REG(which)        (USB_BASE_REG(which) + 0x8C)
#define USB_ERREN_BTSERREN          (1 << 7)
#define USB_ERREN_RESERVED6         (1 << 6)
#define USB_ERREN_DMAERREN          (1 << 5)
#define USB_ERREN_BTOERREN          (1 << 4)
#define USB_ERREN_DFN8EN            (1 << 3)
#define USB_ERREN_CRC16EN           (1 << 2)
#define USB_ERREN_CRC5EOFEN         (1 << 1)
#define USB_ERREN_PIDERREN          (1 << 0)
#define USB_ERREN_ALL               0xFF
#define USB_ERREN_NONE              0x00
// STATUS
// [7:4] ENDP           This four-bit field encodes the endpoint address that received or transmitted the previous token.
// [3]   TX             The most recent transaction was a 1=transmit 0=receive operation.
// [2]   ODD            This bit is set if the last buffer descriptor updated was in the odd bank of the BDT.
// [1:0] RESERVED0
#define USB_STAT_REG(which)         (USB_BASE_REG(which) + 0x90)
#define USB_STAT_TX(reg)            (((reg) >> 3) & 1)
#define USB_STAT_ODD(reg)           (((reg) >> 2) & 1)
#define USB_STAT_EP(reg)            ((reg) >> 4)
// CONTROL
// [7] JSTATE               Live USB differential receiver JSTATE signal
// [6] SE0                  Live USB Single Ended Zero signal
// [5] TXSUSPENDTOKENBUSY   In Host mode, TOKEN_BUSY is set when the USB module is busy executing a USB token.
// [4] RESET                Setting this bit enables the USB Module to generate USB reset signaling.
// [3] HOSTMODEEN           When set to 1, this bit enables the USB Module to operate in Host mode.
// [2] RESUME               When set to 1 this bit enables the USB Module to execute resume signaling.
// [1] ODDRST               Setting this bit to 1 resets all the BDT ODD ping/pong fields to 0, which then specifies the EVEN BDT bank.
// [0] USBENSOFEN           USB Enable
#define USB_CTL_REG(which)          (USB_BASE_REG(which) + 0x94)
#define USB_CTL_JSTATE                  (1 << 7)
#define USB_CTL_SE0                     (1 << 6)
#define USB_CTL_TX_SUSPEND_TOKEN_BUSY   (1 << 5)
#define USB_CTL_RESET                   (1 << 4)
#define USB_CTL_HOST_MODE_EN            (1 << 3)
#define USB_CTL_RESUME                  (1 << 2)
#define USB_CTL_ODD_RST                 (1 << 1)
#define USB_CTL_USB_EN                  (1 << 0)
#define USB_CTL_USB_DISABLE             (0 << 0)
// ADDRESS
// [7]   LSEN           Low Speed Enable
// [6:0] ADDR           Address
#define USB_ADDR_REG(which)         (USB_BASE_REG(which) + 0x98)
#define USB_ADDR_LSEN               (1 << 7)
#define USB_ADDR_FSEN               (0 << 7)
#define USB_ADDR_ADDR(x)            (x)
// BDTPAGE1
// [7:1] BDTBA          Bits 15 through 9 of the Buffer Descriptor Table.
// [0]   RESERVED0
#define USB_BDTPAGE1_REG(which)     (USB_BASE_REG(which) + 0x9C)
#define USB_BDTPAGE1_ADDR(addr)     (((addr) >> 0x08) & 0xFF)
// FRAMENUMLO
// [7:0] FRM            This 8-bit field and the 3-bit field in the Frame Number Register High are used to compute the address where the current Buffer Descriptor Table (BDT) resides in system memory.
#define USB_FRMNUML_REG(which)      (USB_BASE_REG(which) + 0xA0)
// FRAMENUMHI
// [7:3] RESERVED3
// [2:0] FRM            This 3-bit field and the 8-bit field in the Frame Number Register Low are used to compute the address where the current Buffer Descriptor Table (BDT) resides in system memory.
#define USB_FRMNUMH_REG(which)      (USB_BASE_REG(which) + 0xA4)
// TOKEN
// [7:4] TOKENPID       Contains the token type executed by the USB module.
//      - 0001 - OUT Token.   USB Module performs an OUT (TX) transaction.
//      - 1001 - IN Token.    USB Module performs an In (RX) transaction.
//      - 1101 - SETUP Token. USB Module performs a SETUP (TX) transaction
// [3:0] TOKENENDPT     Holds the Endpoint address for the token command.
#define USB_TOKEN_REG(which)        (USB_BASE_REG(which) + 0xA8)
#define USB_TOKEN_OUT(ep)           ((( 1) << 4) | (ep))
#define USB_TOKEN_IN(ep)            ((( 9) << 4) | (ep))
#define USB_TOKEN_SETUP(ep)         (((13) << 4) | (ep)) 
// SOFTHRESHOLDLO
// [7:0] CNT            SOF count threshold (low)
#define USB_SOFTHLDL_REG(which)     (USB_BASE_REG(which) + 0xAC)
#define USB_SOF_THRESHOLD(n)        (10 + (n))
// BDTPAGE2
// [7:0] BDTBA          Provides address bits 23 through 16 of the BDT base address that defines the location of Buffer Descriptor Table resides in system memory.
#define USB_BDTPAGE2_REG(which)     (USB_BASE_REG(which) + 0xB0)
#define USB_BDTPAGE2_ADDR(addr)     (((addr) >> 0x10) & 0xFF)
// BDTPAGE3
// [7:0] BDTBA          Provides address bits 31 through 24 of the BDT base address that defines the location of Buffer Descriptor Table resides in system memory.
#define USB_BDTPAGE3_REG(which)     (USB_BASE_REG(which) + 0xB4)
#define USB_BDTPAGE3_ADDR(addr)     (((addr) >> 0x18) & 0xFF)
// SOFTHRESHOLDHI
// [7:6] RESERVED6
// [5:0] CNT            SOF count threshold (hi)
#define USB_SOFTHLDH_REG(which)     (USB_BASE_REG(which) + 0xB8)


// [7] HOSTWOHUB        Host mode ep0 only. When set this bit allows the host to communicate to a directly connected low speed device.
// [6] RETRYDIS         Host mode ep0 only. When set this bit causes the host to not retry NAK'ed (Negative Acknowledgement) transactions.
// [5] RESERVED5
// [4] EPCTLDIS         This bit, when set, disables control (SETUP) transfers.
// [3] EPRXEN           This bit, when set, enables the endpoint for RX transfers.
// [2] EPTXEN           This bit, when set, enables the endpoint for TX transfers.
// [1] EPSTALL          When set this bit indicates that the endpoint is stalled.
// [0] EPHSHK           When set this bit enables an endpoint to perform handshaking during a transaction to this endpoint.
#define USB_ENDPT_REG(which, num)   (USB_BASE_REG(which) + 0xC0 + (num) * 4) // 16 endpoints: 0xC0 to 0x100
#define USB_ENDPT_HOST_WO_HUB   (1 << 7)
#define USB_ENDPT_RETRY_DIS     (1 << 6)
#define USB_ENDPT_CTL_DISABLE   (1 << 4)
#define USB_ENDPT_RX_EN         (1 << 3)
#define USB_ENDPT_TX_EN         (1 << 2)
#define USB_ENDPT_STALLED       (1 << 1)
#define USB_ENDPT_HSHK_EN       (1 << 0)


// TODO anything at +0x40000 ? (for mirroring / hw impl reasons)

// [0] Access enable
#define USB_ACCESS_REG(which)       (USB_BASE_REG(which) + 0x40010)


// TODO anything between these?

#define USB_BDT_BUF(which, n)       (USB_BASE_REG_ALT(which) + 0x80000 + (n))

// Buffer Descriptors are double-buffered for maximum throughput
#define USB_BD_EVEN 0
#define USB_BD_ODD  1

// BDTCTL bits
// https://www.nxp.com/docs/en/application-note/AN3560.pdf

// OWN indicates whether the CPU or the SIE owns this BD. 1 = SIE, 0 = CPU
// The SIE will set this bit to 0 when the CPU can use it, and the CPU must set it back to 1 so the SIE can act on it.
#define USB_BD_OWN      (1 << 7)
// Sets the type of data packet for bulk transfers using this BD.
#define USB_BD_DATA01   (1 << 6)
#define USB_BD_DATA0        (0 << 6)            // Data packet PID even
#define USB_BD_DATA1        (1 << 6)            // Data packet PID odd
// These bits [5:2] are updated by the SIE after a new token packet is received with the token PID.
#define USB_BD_PID(n)   (((n) & 0xF) << 2)
// These bits [5:2] are written by the CPU to configure the BD
#define USB_BD_KEEP     (1 << 5)        // BD Keep Enable
#define USB_BD_NINC     (1 << 4)        // Address Increment Disable
#define USB_BD_DTS      (1 << 3)        // DTS (data toggle synchronization) enable/disable
#define USB_BD_STALL    (1 << 2)        // Set by the CPU to stall the next transaction.

// PID (USB 1.1 spec Table 8-1)
// Token
#define USB_TOKEN_TOKENPID_OUT      0x1
#define USB_TOKEN_TOKENPID_IN       0x9
#define USB_TOKEN_TOKENPID_SOF      0x5
#define USB_TOKEN_TOKENPID_SETUP    0xD
// Data
#define USB_TOKEN_TOKENPID_DATA0    0x3
#define USB_TOKEN_TOKENPID_DATA1    0xB
#define USB_TOKEN_TOKENPID_DATA2    0x7
#define USB_TOKEN_TOKENPID_MDATA    0xF
// Handshake
#define USB_TOKEN_TOKENPID_ACK      0x2
#define USB_TOKEN_TOKENPID_NAK      0xA
#define USB_TOKEN_TOKENPID_STALL    0xE
#define USB_TOKEN_TOKENPID_NYET     0x6
// Special
#define USB_TOKEN_TOKENPID_PRE      0xC
#define USB_TOKEN_TOKENPID_ERR      0xC
#define USB_TOKEN_TOKENPID_SPLIT    0x8
#define USB_TOKEN_TOKENPID_PING     0x4
// Implementation-specific
#define USB_TOKEN_TOKENPID_TIMEOUT  0

// TODO anything more?



#endif
