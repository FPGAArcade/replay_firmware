/*

The flash process happens in 3 stages :

stage 1) using the "current" core we validate the flash data as much as possible, and create a crc for it
stage 2) after loading the embedded fallback core we upload the flash data directly to the video dram address
stage 3) using a completely standalone flash function we read from the video dram and write to the flash in 256 bytes chunks

when done we reboot the ARM

*/

#include <stdint.h>

#if defined(AT91SAM7S256)
#include "AT91SAM7S256.h"

#define BOARD_MCK               48054857

#define PIN_ACT_LED             AT91C_PIO_PA31
#define PIN_FPGA_CTRL0          AT91C_PIO_PA11
#define PIN_FPGA_PROG_L         AT91C_PIO_PA25

#define LED_OFF AT91C_BASE_PIOA->PIO_SODR = PIN_ACT_LED;
#define LED_ON  AT91C_BASE_PIOA->PIO_CODR = PIN_ACT_LED;

#define TIMER_INIT()        do { AT91C_BASE_PITC->PITC_PIMR = AT91C_PITC_PITEN | ((BOARD_MCK / 16 / 1000 - 1) & AT91C_PITC_PIV); } while(0)
#define TIMER_GET(offset)   ((AT91C_BASE_PITC->PITC_PIIR & AT91C_PITC_PICNT) + (offset << 20))
#define TIMER_CHECK(time)   ((time - (AT91C_BASE_PITC->PITC_PIIR & AT91C_PITC_PICNT)) > (1 << 31))
#define TIMER_WAIT(time)    do { unsigned int t = TIMER_GET(time); while (!TIMER_CHECK(t)); } while(0)

static __attribute__( ( section(".data") ) )  __attribute__ ((always_inline)) unsigned char rSPI(unsigned char outByte)
{
    volatile unsigned int t = AT91C_BASE_SPI->SPI_RDR;
    // for t not to generate a warning, add a dummy asm that 'reference' t
    asm volatile(""
                 : "=r" (t));

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TDRE));

    AT91C_BASE_SPI->SPI_TDR = outByte;

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF));

    return ((unsigned char)AT91C_BASE_SPI->SPI_RDR);
}

#define SPI_WAIT4XFEREND()  do { while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY)); } while(0)
#define SPI_ENABLEFILEIO()  do { AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL0; } while (0)
#define SPI_DISABLEFILEIO() do { SPI_WAIT4XFEREND(); AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL0; } while(0)

#define SPI_WAITSTAT(mask, wanted) do                   \
    {                                                       \
        uint8_t  stat;                                      \
        uint32_t timeout = TIMER_GET(100);                  \
        \
        do {                                                \
            SPI_ENABLEFILEIO();                             \
            rSPI(0x87);                                     \
            stat = rSPI(0);                                 \
            SPI_DISABLEFILEIO();                            \
            if (TIMER_CHECK(timeout)) {                     \
                break;                                      \
            }                                               \
        } while ((stat & (mask)) != (wanted));              \
    } while(0)


#define SPI_READBUFFERSINGLE(buffer, length) do         \
    {                                                       \
        AT91C_BASE_SPI->SPI_TPR  = (uint32_t) (buffer);     \
        AT91C_BASE_SPI->SPI_TCR  = (length);                \
        \
        AT91C_BASE_SPI->SPI_RPR  = (uint32_t) (buffer);     \
        AT91C_BASE_SPI->SPI_RCR  = (length);                \
        \
        AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN |        \
                                   AT91C_PDC_RXTEN;         \
        \
        uint32_t timeout = TIMER_GET(100);                  \
        while ((AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX)) != (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX) ) { \
            if (TIMER_CHECK(timeout)) {                     \
                break;                                      \
            }                                               \
        };                                                  \
        AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTDIS |       \
                                   AT91C_PDC_RXTDIS;        \
    } while(0)

#define SPI_WRITEBUFFERSINGLE(buffer, length) do        \
    {                                                       \
        AT91C_BASE_SPI->SPI_TPR  = (uint32_t) buffer;       \
        AT91C_BASE_SPI->SPI_TCR  = (length);                \
        AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN;         \
        \
        uint32_t timeout = TIMER_GET(100);                  \
        while ((AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX)) != (AT91C_SPI_ENDTX) ) { \
            if (TIMER_CHECK(timeout)) {                     \
                break;                                      \
            }                                               \
        };                                                  \
        AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTDIS |       \
                                   AT91C_PDC_RXTDIS;        \
    } while(0)


// placed before any other code to prevent accidentally calling external functions (which would be illegal)
__attribute__( ( section(".data") ) ) __attribute__ ((noreturn)) __attribute__ ((noinline)) void FlashAndReset(uint32_t base, uint32_t dram, uint32_t length)
{
    uint32_t line[256 / 4];
    uint32_t* sram = line;
    uint32_t* flash = (uint32_t*)0x0;

    // turn off all interrupts!
    AT91C_BASE_AIC->AIC_IDCR = AT91C_ALL_INT;

    TIMER_INIT();

    AT91C_BASE_MC->MC_FMR = AT91C_MC_FWS_1FWS | (48 << 16);

    for (uint32_t addr = dram; addr < dram + length; addr += sizeof(line), base += sizeof(line)) {

        const uint32_t size = sizeof(line);

        SPI_ENABLEFILEIO();
        rSPI(0x80); // set address
        rSPI((uint8_t)(addr));
        rSPI((uint8_t)(addr >> 8));
        rSPI((uint8_t)(addr >> 16));
        rSPI((uint8_t)(addr >> 24));
        SPI_DISABLEFILEIO();

        SPI_ENABLEFILEIO();
        rSPI(0x81); // set direction
        rSPI(0x80); // read
        SPI_DISABLEFILEIO();

        SPI_ENABLEFILEIO();
        rSPI(0x84); // do Read
        rSPI((uint8_t)( size - 1)     );
        rSPI((uint8_t)((size - 1) >> 8));
        SPI_DISABLEFILEIO();

        SPI_WAITSTAT(0x04, 0);

        SPI_ENABLEFILEIO();
        rSPI(0xA0);
        SPI_READBUFFERSINGLE(sram, size);
        SPI_DISABLEFILEIO();

        for (uint32_t i = 0; i < size / 4; i++) {
            flash[i] = sram[i];
        }

        TIMER_WAIT(10);

        while (!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));

        if ((base >> 8) & 1) {
            LED_ON;

        } else {
            LED_OFF;
        }

        AT91C_BASE_MC->MC_FCR = ((0x5a) << 24) | (base & AT91C_MC_PAGEN) | AT91C_MC_FCMD_START_PROG;

        TIMER_WAIT(10);

        while (!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));


        for (uint32_t i = 0; i < size / 4; i++) {
            sram[i] = 0;
        }

        SPI_ENABLEFILEIO();
        rSPI(0x80); // set address
        rSPI((uint8_t)(addr));
        rSPI((uint8_t)(addr >> 8));
        rSPI((uint8_t)(addr >> 16));
        rSPI((uint8_t)(addr >> 24));
        SPI_DISABLEFILEIO();

        SPI_ENABLEFILEIO();
        rSPI(0x81); // set direction
        rSPI(0x00); // write
        SPI_DISABLEFILEIO();

        SPI_ENABLEFILEIO();
        rSPI(0x84); // do Read
        rSPI((uint8_t)( size - 1)     );
        rSPI((uint8_t)((size - 1) >> 8));
        SPI_DISABLEFILEIO();

        SPI_WAITSTAT(0x02, 0);

        SPI_ENABLEFILEIO();
        rSPI(0xB0);
        SPI_WRITEBUFFERSINGLE(sram, size);
        SPI_DISABLEFILEIO();

        SPI_WAITSTAT(0x01, 0);
    }

    // set PROG low to reset FPGA (open drain)
    AT91C_BASE_PIOA->PIO_OER = PIN_FPGA_PROG_L;
    TIMER_WAIT(1);
    AT91C_BASE_PIOA->PIO_ODR = PIN_FPGA_PROG_L;
    TIMER_WAIT(2);

    // perform a ARM reset
    asm("ldr r3, = 0x00000000\n");
    asm("bx  r3\n");

    // we will never reach here
    while (1) {}
}

#else

void FlashAndReset(uint32_t base, uint32_t dram, uint32_t length)
{
    (void) base, (void) dram, (void) length;
    // noop!
}

#endif


#include "flash.h"
#include "messaging.h"
#include "fpga.h"
#include "hardware/io.h"
#include "hardware/timer.h"
#include "fileio.h"
#include "font8x8_basic.h"

extern FF_IOMAN* pIoman;

static inline uint32_t HexVal(int c)
{
    c = tolower(c);

    if (c >= '0' && c <= '9') {
        return c - '0';

    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a') + 10;

    } else {
        ERROR("bad hex digit '%c'", c);
        return (uint32_t) - 1;
    }
}

static inline uint8_t HexByte(const char** strref)
{
    const char* s = *strref;
    uint8_t c = (HexVal(s[0]) << 4) | HexVal(s[1]);
    (*strref) += 2 * sizeof(uint8_t);
    return c;
}

static inline uint32_t AsciiToHex(const char** strref, int maxsize)
{
    const char* str = *strref;
    uint32_t value = 0;

    for (int i = 0; i < maxsize * 2; ++i) {
        uint8_t c = toupper(*str++);
        uint8_t isnum = isdigit(c);
        uint8_t ishex = isxdigit(c);

        if (c == '\0') {
            break;
        }

        if ( !isnum && !ishex ) {
            break;
        }

        c = isnum ? c - '0' : c - 'A' + 10;

        value = (value << 4) | c;
    }

    *strref = str;
    return value;
}

typedef uint8_t (*SRECHandler)(uint8_t type, uint32_t addr, uint32_t offset, uint8_t length, uint8_t value);

static uint8_t ParseSRecords(const char* filename, SRECHandler handler)
{
    char line[2 /* "Sx record type */ + 2 /* byte count */ + 0xff * 2 /* maximum value of byte count */ + 1 /*string termination*/];
    int linenum = -1;

    if (!handler) {
        return linenum;
    }

    FF_FILE* f = FF_Open(pIoman, filename, FF_MODE_READ, NULL);

    if (!f) {
        return linenum;
    }

    linenum++;

    while (!FF_isEOF(f) && FF_GetLine(f, line, sizeof(line))) {
        const char* s = line;
        uint8_t checksum = 0, c = 0;
        uint8_t S = *s++;
        uint8_t type = *s++ - '0';
        int len = HexByte(&s);

        uint32_t addr = 0;
        uint8_t retval = 0;

        linenum++;

        // is this really an S-record?
        if (S != 'S' && (3 <= len && len <= 0xff)) {
            continue;
        }

        switch (type) {
            case 0: { // header (16bit '0000' address)
                addr = AsciiToHex(&s, sizeof(uint16_t));
                checksum += len;
                checksum += (addr & 0xff) + (addr >> 8);
                len -= sizeof(uint16_t) /* addr */ + sizeof(uint8_t) /* checksum */;

                if (addr != 0) {
                    len = 0;
                    retval = 0xff;
                }

                break;
            }

            case 3: // data (32bit address)
            case 7: { // start address (32bit) termination
                addr = AsciiToHex(&s, sizeof(uint32_t));
                checksum += len;
                checksum += (addr & 0xff) + ((addr >> 8) & 0xff) + ((addr >> 16) & 0xff) + ((addr >> 24) & 0xff);
                len -= sizeof(uint32_t) /* addr */ + sizeof(uint8_t) /* checksum */;

                if (type == 7 && len != 0) {
                    len = 0;
                    retval = 0xff;
                }

                break;
            }

            default:
                break;
        }

        for (int i = 0; i < len; ++i) {
            uint8_t b = HexByte(&s);

            if ((retval = handler(type, addr, i, len, b)) != 0) {
                break;
            }

            checksum += b;
        }

        checksum = ~checksum;
        c = HexByte(&s);

        if (retval != 0 || checksum != c) {
            break;
        }

        if (type == 7) {
            if (handler(type, addr, 0, len, 0) == 0) {
                linenum = 0;
            }

            break;
        }
    }

    FF_Close(f);

    return linenum;
}


static unsigned int feed_crc32(uint8_t* data, uint32_t length)
{
    static uint32_t crc;

    if (data == 0 && length == 0xffffffff) {
        crc = length;
        length = 0;
    }

    for (uint32_t i = 0; i < length; ++i) {
        uint32_t byte = *data++;
        crc = crc ^ byte;

        for (int j = 7; j >= 0; j--) {    // Do eight times.
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}


static uint32_t srecCurrentAddr;
static uint8_t srecLineBuffer[FLASH_PAGE_SIZE];

static uint32_t dramBaseAddr;

static uint32_t s_FlashAddress;
static uint32_t s_FlashSize;
static uint32_t s_FlashCRC32;

static void DumpToConsole(void)
{
    uint32_t valid_bytes = ((srecCurrentAddr - 1) & (FLASH_PAGE_SIZE - 1)) + 1;
    //    DumpBufferOffset(srecLineBuffer, valid_bytes, srecCurrentAddr-valid_bytes);
    s_FlashSize += valid_bytes;
    feed_crc32(srecLineBuffer, valid_bytes);
}

static uint8_t VerifyHandler(uint8_t type, uint32_t base, uint32_t offset, uint8_t length, uint8_t value)
{
    uint32_t addr = base + offset;

    //    DEBUG(1, "VerifyHandler(%02d, 0x%08x, 0x%04x, 0x%02x, 0x%02x = %c) ; srecCurrentAddr = 0x%08x ; addr = 0x%08x", type, base, offset, length, value, isgraph(value) ? value : '?', srecCurrentAddr, addr);

    if (type == 0) {            // header

        srecLineBuffer[offset] = value;

        if (offset == length - 1) {
            srecLineBuffer[offset + 1] = '\0';
            INFO("S0 : <%04x> %s", base, srecLineBuffer);
            s_FlashSize = 0;
            feed_crc32(0, 0xffffffff);
        }

        return 0;

    } else if (type == 7) {     // termination
        // flush
        if ((srecCurrentAddr & (FLASH_PAGE_SIZE - 1)) != 0) {
            DumpToConsole();
            memset(srecLineBuffer, 0x00, sizeof(srecLineBuffer));
        }

        s_FlashAddress = base;
        s_FlashCRC32 = feed_crc32(0, 0);
        INFO("S7 : Address = $%08x", s_FlashAddress);
        INFO("S7 : Length  = %d", s_FlashSize);
        INFO("S7 : CRC32   = $%08x", s_FlashCRC32);
        return 0;
    }

    if (addr < srecCurrentAddr) {
        // error - unexpected address lower than
        return 0xff;
    }

    while (srecCurrentAddr <= addr) {

        uint8_t b = (addr == srecCurrentAddr) ? value : 0xff;

        srecLineBuffer[srecCurrentAddr & (FLASH_PAGE_SIZE - 1)] = b;
        srecCurrentAddr++;

        if ((srecCurrentAddr & (FLASH_PAGE_SIZE - 1)) == 0) {
            // we have completed a full page
            DumpToConsole();
            memset(srecLineBuffer, 0x00, sizeof(srecLineBuffer));
        }
    }

    return 0;
}


uint8_t FLASH_VerifySRecords(const char* filename, uint32_t* crc_file, uint32_t* crc_flash)
{
    srecCurrentAddr = 0x102000L;        // we expect the flash SREC to start at $102000 - otherwise error.
    uint8_t retval = ParseSRecords(filename, VerifyHandler);

    if (retval != 0) {
        return 0;
    }

    uint8_t* p = (uint8_t*)0x102000;
    feed_crc32(0, 0xffffffff);
    uint32_t current_crc = feed_crc32(p, s_FlashSize);

    INFO("Current CRC32 = $%08x", current_crc);

    if (crc_file) {
        *crc_file = s_FlashCRC32;
    }

    if (crc_flash) {
        *crc_flash = current_crc;
    }

    return 1;
}

static void UploadToDRAM()
{
    uint32_t valid_bytes = ((srecCurrentAddr - 1) & (FLASH_PAGE_SIZE - 1)) + 1;

    if (FileIO_MCh_BufToMem(srecLineBuffer, dramBaseAddr, valid_bytes) != 0) {
        WARNING("DRAM write failed!");
    }

    dramBaseAddr += valid_bytes;
}

static uint8_t UploadHandler(uint8_t type, uint32_t base, uint32_t offset, uint8_t length, uint8_t value)
{
    uint32_t addr = base + offset;

    if (type == 0) {            // header

        return 0;

    } else if (type == 7) {     // termination
        // flush
        if ((srecCurrentAddr & (FLASH_PAGE_SIZE - 1)) != 0) {
            UploadToDRAM();
            memset(srecLineBuffer, 0x00, sizeof(srecLineBuffer));
        }

        return 0;
    }

    if (addr < srecCurrentAddr) {
        // error - unexpected address lower than
        return 0xff;
    }

    while (srecCurrentAddr <= addr) {

        uint8_t b = (addr == srecCurrentAddr) ? value : 0xff;

        srecLineBuffer[srecCurrentAddr & (FLASH_PAGE_SIZE - 1)] = b;
        srecCurrentAddr++;

        if ((srecCurrentAddr & (FLASH_PAGE_SIZE - 1)) == 0) {
            // we have completed a full page
            UploadToDRAM();
            memset(srecLineBuffer, 0x00, sizeof(srecLineBuffer));
        }
    }

    return 0;
}

static uint8_t VerifyDRAMContents(uint32_t address, uint32_t size, uint32_t crc32)
{
    uint8_t line[256];

    feed_crc32(0, 0xffffffff);

    while (size) {
        uint32_t len = size >= sizeof(line) ? sizeof(line) : size;

        FileIO_MCh_MemToBuf(line, address, len);
        address += len;
        size -= len;
        feed_crc32(line, len);

    }

    DEBUG(1, "crc32 %08x vs %08x", crc32, feed_crc32(0, 0));

    return 0;
}


static void RenderChar(uint32_t addr, uint8_t c)
{
    const uint8_t* map = font8x8_basic[c];
    const uint32_t chunk = 512;
    uint8_t tile[512 * 12]; // must be atleast 720*8, but in 512 chunks because of FileIO_MCh_MemToBuf

    if (c > 128) {
        return;
    }

    for (int i = 0; i < sizeof(tile); i += chunk) {
        FileIO_MCh_MemToBuf(&tile[i], addr + i, chunk);
    }

    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            const uint8_t bit = map[v] & (1 << u);
            tile[u + 720 * v] = bit ? 0xff : 0x00;
        }
    }

    for (int i = 0; i < sizeof(tile); i += chunk) {
        FileIO_MCh_BufToMem(&tile[i], addr + i, chunk);
    }
}


static void RenderText(uint32_t x, uint32_t y, const char* str)
{
    const uint32_t DRAMbase = 0x00400000;
    uint32_t addr = DRAMbase + (x + 720 * y);

    for (char c; (c = *str); str++) {
        RenderChar(addr, c);
        addr += 8;
    }
}


// TODO - keep video mode settings in the OSD!
uint8_t FLASH_RebootAndFlash(const char* filename)
{
    const uint32_t DRAMbase = 0x00400000;
    const uint32_t DRAMupload = DRAMbase + (576 - 480 + 480 / 5) * 720;

    OSD_Disable();

    // make sure FPGA is held in reset
    IO_DriveLow_OD(PIN_FPGA_RST_L);
    // set PROG low to reset FPGA (open drain)
    IO_DriveLow_OD(PIN_FPGA_PROG_L);
    Timer_Wait(1);
    IO_DriveHigh_OD(PIN_FPGA_PROG_L);
    Timer_Wait(2);

    CFG_vid_timing_HD27(F60HZ);
    CFG_set_coder(CODER_DISABLE);

    if (FPGA_Default()) {
        return 0;
    }

    IO_DriveHigh_OD(PIN_FPGA_RST_L);
    Timer_Wait(200);

    OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
    OSD_Reset(OSDCMD_CTRL_RES | OSDCMD_CTRL_HALT);
    Timer_Wait(100);
    FPGA_DramTrain();
    CFG_set_CH7301_SD();

    // dynamic/static setup bits
    OSD_ConfigSendUserS(0x00000000);
    OSD_ConfigSendUserD(0x00000002); // 60HZ progressive + background image
    OSD_Reset(OSDCMD_CTRL_RES);

    memset(srecLineBuffer, 0x00, sizeof(srecLineBuffer));

    for (uint32_t dram = DRAMbase; dram < DRAMbase + (1024 * 1024); dram += sizeof(srecLineBuffer)) {
        if (FileIO_MCh_BufToMem(srecLineBuffer, dram, sizeof(srecLineBuffer)) != 0) {
            WARNING("DRAM write failed!");
        }
    }

    RenderText(30, 30 + (576 - 480), "Flashing... Please wait!");

    srecCurrentAddr = 0x102000L;        // we expect the flash SREC to start at $102000 - otherwise error.
    dramBaseAddr = DRAMupload;
    uint8_t ret = ParseSRecords(filename, UploadHandler);

    if (ret != 0) {
        WARNING("SREC error at line %d", ret);
        return 0;
    }

    VerifyDRAMContents(DRAMupload, s_FlashSize, s_FlashCRC32);

    DEBUG(1, "LETS GO!");

    FlashAndReset(0x102000L, DRAMupload, s_FlashSize);        // will never return!

    DEBUG(1, "WHAT?!");

    return 1;
}
