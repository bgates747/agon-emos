/* PORT-008 / INTEG-019: one foreground SD lease, small retained RX slices.
 * File paths, FatFS, source capacity and file policy belong to the utility/P4.
 * Only ext.sdlink's admitted manual Legacy listener reaches these functions.
 * Never restore UART from a timeout or run inactive clocks against loop TX.
 */
#include "emos_z80_io.h"
#include "ff.h"
#include "emos.h"
#include "uart.h"
#include "emos_parallel.h"
#include "emos_parallel_handover.h"
#include "emos_keyboard.h"
#include "emos_parallel_upload.h"
extern BYTE emos_parallel_upload_bytes(BYTE *, UINT24);
extern void emos_parallel_io_write_control(UINT24);
static UINT32 uploadRemaining;
static BYTE uploadOwned, uploadStarted, uploadParked, uploadFailed;
BYTE emos_parallel_upload_owned(void) { return uploadOwned; }
void emos_parallel_upload_abandon(void) {
    if(uploadOwned) {
        /* Existing boot owner fences all shared pads and invalidates grant.
         * It never revives the uploadParked UART or claims a partial file succeeded. */
        emos_parallel_boot_block_end(0);
        uploadOwned=0;
    }
}
static BYTE advance(BYTE ending) {
    t_emosParallelHandover *h=emos_parallel_boot_handover();
    t_emosDeadline d;
    BYTE done, action;
    d.last=emos_keyboard_clock();d.elapsed=0;d.budget=(UINT24)262144;
    while(emos_keyboard_deadline_step(&d)) {
        done=(PD_DR & 0x10)?EPH_READY_HIGH:0;
        if(emos_key_faulted || h->failed)break;
        if(h->phase==EPH_DRAIN) {
            BYTE r=emos_keyboard_parallel_park();
            if(r==UART_POLL_EMPTY)continue;
            if(r!=UART_POLL_READY)break;
            uploadParked=1;done|=EPH_QUIET;
        }
        if(h->phase==EPH_ENTRY_RELEASE || h->phase==EPH_BLOCK_RELEASE) {
            if(!uploadParked || PC_DDR!=0xFF || PC_ALT1 || PC_ALT2)break;
            done|=EPH_RELEASED;
        }
        if(h->phase==EPH_LOCAL_ARM)done|=EPH_ARMED;
        if(h->phase==EPH_BLOCK) {
            if(!ending)return FR_OK;
            done|=EPH_DONE;
        }
        if(h->phase==EPH_RETURN_UART) {
            if(emos_keyboard_parallel_unpark()!=UART_POLL_READY)break;
            done|=EPH_UART_UP;
        }
        action=emos_parallel_handover_step(h,done);
        if(h->failed || action==EPH_FENCE)break;
        emos_parallel_io_write_control((h->validN?0x80:0)|(h->clockHigh?0x20:0));
        if(action==EPH_LIVE) {
            emos_parallel_boot_block_end(1);
            if(emos_keyboard_parallel_release()!=UART_POLL_READY)break;
            uploadOwned=0;return FR_OK;
        }
    }
    emos_parallel_upload_abandon();return FR_TIMEOUT;
}
BYTE emos_parallel_upload_begin(UINT32 length) {
    t_emosParallelHandover *h=emos_parallel_boot_handover();
    if(uploadOwned || !length || length>32UL*1024*1024)return FR_INVALID_PARAMETER;
    /* failed records the old payload, including boot. The recovered UART
     * phase plus actual serializer reservation admit a NEW begin(), which
     * resets that history. All active-payload fault checks remain below. */
    if(h->phase!=EPH_UART ||
        emos_keyboard_parallel_reserve()!=UART_POLL_READY)return EMOS_BUSY;
    if(!emos_parallel_handover_begin(h) || !emos_parallel_boot_block_begin()) {
        emos_parallel_boot_block_end(0);return EMOS_UNAVAILABLE;
    }
    uploadRemaining=length;uploadOwned=1;uploadStarted=uploadParked=uploadFailed=0;
    return advance(0);
}
BYTE emos_parallel_upload_read(BYTE *buffer, UINT24 length) {
    t_emosParallelHandover *h=emos_parallel_boot_handover();
    BYTE irq=emos_keyboard_lock(), status;
    emos_keyboard_unlock(irq);
    if(!buffer || !length || length>512 || length>uploadRemaining)return FR_INVALID_PARAMETER;
    if(!irq || !uploadOwned || uploadFailed || !uploadParked || h->failed || h->phase!=EPH_BLOCK ||
        UART1_IER || UART1_MCTL!=UART_MCTL_LOOP || PC_DDR!=0xFF || PC_ALT1 || PC_ALT2 ||
        (PD_DDR & 0xB0)!=0x10 || (PD_ALT1 & 0xB0) || (PD_ALT2 & 0xB0) ||
        (PD_DR & 0xB0)!=(uploadStarted?0x20:0xA0))return EMOS_UNAVAILABLE;
    if(!uploadStarted) {
        h->validN=0;emos_parallel_io_write_control(0x20);uploadStarted=1;
    }
    status=emos_parallel_upload_bytes(buffer,length);
    if(status){uploadFailed=1;return FR_TIMEOUT;}
    uploadRemaining-=length;return FR_OK;
}
BYTE emos_parallel_upload_end(BYTE abort) {
    if(!uploadOwned || abort>1 || (!abort && (uploadRemaining || uploadFailed)))return FR_INVALID_PARAMETER;
    /* End performs the normal reciprocal return even after a known file error.
     * P4 sees an exact or explicitly short VALID pulse; no partial activation.
     * Starting and ending an empty/aborted pulse is a failure, never data. */
    if(!uploadStarted) {
        emos_parallel_boot_handover()->validN=0;
        emos_parallel_io_write_control(0x20);uploadStarted=1;
    }
    /* Release VALID while CLOCK remains high BEFORE the ordinary return
     * lowers CLOCK. Changing both together can create an extra sampled byte. */
    emos_parallel_boot_handover()->validN=1;
    emos_parallel_io_write_control(0xA0);
    return advance(1);
}
