#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "machine.h"
#include "6809.h"

/*
 * my6809 machine (NitrOS-9 / multicomp09-compatible timer):
 *
 *   64K RAM (vectors in RAM)
 *   ACIA  $8018 (status) / $8019 (data)   -- my6809.d ACIABase
 *   TIMER $8010                           -- my6809.d TIMER
 *
  * TIMER (same protocol as multicomp09 $FFDD / NitrOS-9 mc09clock):
 *   At reset: disabled, IRQ deasserted, reads as 0.
 *   bit[1] R/W  -- enable
 *   bit[7] R / write-1-to-clear -- interrupt pending
 *
 *   ISR services with:  INC TIMER  (N set iff it was a timer IRQ)
 *     read $00 write $01  disabled, N=0
 *     read $02 write $03  enabled, no IRQ, N=0
 *     read $80 write $81  disabled, old IRQ cleared, N=1
 *     read $82 write $83  enabled, old IRQ cleared, N=1
 *
 * OS-9 expects TkPerSec=50 (20 ms).  If -I is omitted we default to
 *   cycles_per_tick = cpu_khz * 20  (50 Hz at the configured MHz).
 */
 
extern unsigned int cycles_per_tick;
extern unsigned int cpu_khz;
 
/* --- ACIA state --- */
static U8 acia_data = 0;
static int acia_rx_ready = 0;

/* --- Timer state --- */
static U8 my_timer = 0x00;
 
/* Offsets within I/O page $8000 (CPU addr = $8000 + offset) */
#define MY6809_TIMER_OFF   0x10   /* $8010 TIMER */
#define MY6809_ACIA_STAT   0x18   /* $8018 */
#define MY6809_ACIA_DATA   0x19   /* $8019 */

#define MY_TIMER_EN        0x02
#define MY_TIMER_IRQ       0x80
 
 
static U8
my6809_io_read (struct hw_device *dev, unsigned long addr)
{
    (void)dev;
    addr &= 0x7f;
 
    switch (addr)
    {
    case MY6809_TIMER_OFF:
        return my_timer;
 
    case MY6809_ACIA_STAT:
        {
            U8 status = 0x02; /* TDRE */
 
            if (!acia_rx_ready)
            {
                int flags = fcntl (0, F_GETFL, 0);
                fcntl (0, F_SETFL, flags | O_NONBLOCK);
                int ch = getchar ();
                fcntl (0, F_SETFL, flags);
                if (ch != EOF)
                {
                    acia_data = (U8)ch;
                    if (acia_data == 10)
                        acia_data = 13;
                    acia_rx_ready = 1;
                }
            }
            if (acia_rx_ready)
                status |= 0x01; /* RDRF */
            return status;
        }
 
    case MY6809_ACIA_DATA:
        acia_rx_ready = 0;
        return acia_data;
 
    default:
        return 0xff;
    }
}
 
 
static void
my6809_io_write (struct hw_device *dev, unsigned long addr, U8 val)
{
    (void)dev;
    addr &= 0x7f;
 
    switch (addr)
    {
    case MY6809_TIMER_OFF:
        /*
         * Multicomp09 / OS-9 semantics (see nitros9/defs/mc09.d):
         *  - only bit1 of the written value is stored as enable
         *  - writing 1 to bit7 clears pending IRQ and release_irq
         * INC TIMER does RMW: e.g. $82 -> write $83 -> stored $02, N=1 from INC
         */
        my_timer = (U8)((my_timer & (U8)~MY_TIMER_EN) | (val & MY_TIMER_EN));
        if (val & MY_TIMER_IRQ)
        {
            release_irq (0);
            my_timer &= (U8)~MY_TIMER_IRQ;
        }
        break;
 
    case MY6809_ACIA_DATA:
        putchar (val);
        fflush (stdout);
        break;
        
    case MY6809_ACIA_STAT:
        /* control register ignored (polled ACIA) */
        break;
 
    default:
        break;
    }
} 


/* Called every cycles_per_tick CPU cycles (main loop -I path). */
void
my6809_tick (void)
{
    if (my_timer & MY_TIMER_EN)
    {
        my_timer |= MY_TIMER_IRQ;
        request_irq (0);
    }
}


void
my6809_init (const char *boot_rom_file)
{
    struct hw_device *io;
 
    (void)boot_rom_file;

    device_define (ram_create (MAX_CPU_ADDR), 0,
                   0x0000, MAX_CPU_ADDR, MAP_READWRITE);
    
    /*
     * I/O page $8000-$807F: TIMER $8010, ACIA $8018/$8019.
     * bus_map rounds to 128-byte pages -- must map the whole page.
    */
    io = console_create ();
    io->class_ptr->read  = my6809_io_read;
    io->class_ptr->write = my6809_io_write;
    device_define (io, 0, 0x8000, BUS_MAP_SIZE, MAP_READWRITE);
 
    my_timer = 0x00;

    /*
     * Default 50 Hz for OS-9 (TkPerSec=50) if user did not pass -I.
     * cycles_per_tick = cpu_khz * 20  ->  20 ms period.
     */
    if (cycles_per_tick == 0)
    {
        unsigned int khz = cpu_khz ? cpu_khz : 1000;
        cycles_per_tick = khz * 20;
    }
 
    printf ("my6809: 64K RAM + ACIA $8018/$8019 + TIMER $8010 (OS-9/mc09 protocol)\n");
    printf ("my6809: 50Hz tick every %u cycles (-I overrides); en=$02, ISR: INC $8010\n",
            cycles_per_tick);
}


struct machine my6809_machine =
{
    .name = "my6809",
    .fault = fault,
    .init = my6809_init,
    .periodic = 0,
    .tick = my6809_tick,
};
