#ifndef _FINALROM

#include "PR/os_internal.h"
#include "PR/rdb.h"

#include "macros.h"

#ifdef BBPLAYER
#define MSGBUF_LEN 2
#else
#define MSGBUF_LEN 1
#endif

static int readHostInitialized = FALSE;
static OSMesgQueue readHostMesgQueue ALIGNED(8);
static OSMesg readHostMesgBuf[MSGBUF_LEN];

u32 __osRdb_Read_Data_Buf;
u32 __osRdb_Read_Data_Ct;

#ifdef BBPLAYER
u32 __osReadHost(void* dramAddr, u32 nbytes) {
    unsigned char tstr[4];
    u32 sent = 0;
    u32 nBytesRead;
    u32 saveMask;
    u32 saveCt;

    if (!readHostInitialized) {
        osCreateMesgQueue(&readHostMesgQueue, readHostMesgBuf, ARRLEN(readHostMesgBuf));
        osSetEventMesg(OS_EVENT_RDB_READ_DONE, &readHostMesgQueue, (OSMesg)0x13);
        osSetEventMesg(OS_EVENT_RDB_UNK32, &readHostMesgQueue, (OSMesg)0x14);
        readHostInitialized = TRUE;
    }

    saveMask = __osDisableInt();
    __osRdb_Read_Data_Buf = dramAddr;
    __osRdb_Read_Data_Ct = nbytes;
    __osRestoreInt(saveMask);

    while (sent == 0) {
        if (osRecvMesg(&readHostMesgQueue, NULL, OS_MESG_NOBLOCK) == 0) {
            osRecvMesg(&readHostMesgQueue, NULL, OS_MESG_NOBLOCK);
            break;
        }
        tstr[0] = 0;
        sent += __osRdbSend(tstr, 1, RDB_TYPE_GtoH_READY_FOR_DATA);
    }

    if (sent != 0) {
        osRecvMesg(&readHostMesgQueue, NULL, OS_MESG_BLOCK);
        osRecvMesg(&readHostMesgQueue, NULL, OS_MESG_NOBLOCK);
    }

    saveMask = __osDisableInt();
    nBytesRead = nbytes - __osRdb_Read_Data_Ct;
    __osRdb_Read_Data_Buf = 0;
    __osRdb_Read_Data_Ct = 0;
    __osRestoreInt(saveMask);
    return nBytesRead;
}

void osReadHost(void* dramAddr, u32 nbytes) {
    u32 nBytesRead = 0;

    while (nBytesRead < nbytes) {
        nBytesRead += __osReadHost((u8*)dramAddr + nBytesRead, nbytes);
    }
}

s32 osBbReadHost(void* dramAddr, u32 nbytes) {
    if (__osReadHost(dramAddr, nbytes) < nbytes) {
        return -1;
    }
    return 0;
}

#else

void osReadHost(void* dramAddr, u32 nbytes) {
    char tstr[4];
    u32 sent = 0;

    if (!readHostInitialized) {
        osCreateMesgQueue(&readHostMesgQueue, readHostMesgBuf, ARRLEN(readHostMesgBuf));
        osSetEventMesg(OS_EVENT_RDB_READ_DONE, &readHostMesgQueue, NULL);
        readHostInitialized = TRUE;
    }

    __osRdb_Read_Data_Buf = dramAddr;
    __osRdb_Read_Data_Ct = nbytes;

    while (sent == 0) {
        sent += __osRdbSend(tstr, 1, RDB_TYPE_GtoH_READY_FOR_DATA);
    }

    osRecvMesg(&readHostMesgQueue, NULL, OS_MESG_BLOCK);
    return;
}
#endif

#endif
