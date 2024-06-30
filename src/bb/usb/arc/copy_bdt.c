#include "PR/os_internal.h"
#include "PR/rcp.h"

#include "../usb.h"
#ident "$Revision: 1.1 $"

// Copy a BD, byteswapping it
void _usb_bdt_copy_swab(HOST_BDT_STRUCT_PTR from_bdt, HOST_BDT_STRUCT_PTR to_bdt) {
    uint_32 x;
    uint_32 y;

    x = from_bdt->ADDR.W;
    y = BSWAP32(x);
    to_bdt->ADDR.W = y;

    x = *(uint_32*)&from_bdt->BC;
    y = BSWAP32(x);
    *(uint_32*)&to_bdt->BC = y;
}
