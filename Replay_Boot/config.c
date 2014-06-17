/** @file config.c */

#include "config.h"
#include "messaging.h"
#include "menu.h"

extern hdfTYPE hdf[HD_MAX_NUM];  // in ..hdd.c

/** @brief extern link to sdcard i/o manager

  Having this here is "ugly", but ff_ioman declares this "prototype"
  in the header which is actually code and should be in the .c file.
  Won't fix for now to avoid issues with the fullfat stuff....

  Usually it should be part of the module header where it belongs to,
  not at all in this .h/.c file...
*/
extern FF_IOMAN *pIoman;

extern const char version[];  // temp

  // y0 - FPGA DRAM
  // y1 - Coder
  // y2 - FPGA aux/sys clk
  // y3 - Expansion Main
  // y4 - FPGA video
  // y5 - Expansion Small

  //  input clock = 27MHz.
  //  output = (27 * n / m) / p. example m=375 n=2048 p=12 output = 12.288MHz. VCO = 147.456 MHz
  //

const clockconfig_t init_clock_config_pal = {
  481, 4044,  // pll1 m,n = 227MHz
  46,  423,   // pll2 m,n = 248MHz
  225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
  { 1,  2,  4,  0,  0,  0, }, // p0,1,2,3,4,5
  { 2,  14, 5,  1,  1,  1, }, // d0,1,2,3,4,5
  { 1,  1,  1,  0,  1,  0  }  // y0,1,2,3,4,5
// 113  17  49   0   27  0
};

const clockconfig_t init_clock_config_ntsc ={
  33,  280,   // pll1 m,n = 229MHz
  27,  133,   // pll2 m,n = 133MHz
  225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
  { 1,  1,  4,  0,  0,  0, }, // p0,1,2,3,4,5
  { 2,  16, 5,  1,  1,  1, }, // d0,1,2,3,4,5
  { 1,  1,  1,  0,  1,  0  }  // y0,1,2,3,4,5
};
// 114  14  49   0   27  0

const clockconfig_t init_clock_config_hd74 ={
  33,  280,   // pll1 m,n = 229MHz
   2,   11,   // pll2 m,n = 148.5MHz
  225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
  { 1,  0,  4,  0,  2,  0, }, // p0,1,2,3,4,5
  { 2,  0,  5,  1,  2,  1, }, // d0,1,2,3,4,5
  { 1,  0,  1,  0,  1,  0  }  // y0,1,2,3,4,5
};
// 57  14  49   0   74.25  0

// VIDEO ENCODER CONFIGURATION
// ===========================
//
// Has to be set via configured FPGA
//

// fstep 50-80ps  65+/-15ps
// 27MHz    37.0ns 1/4 cycle ~ 9200 ps = 141 steps
// 74.25MHz 13.5ns 1/4 cycle ~ 3300 ps =  51 steps

const vidconfig_t   default_vid_config_sd = {
  0x00, // reg 0x1C CM   Clock Mode Register
  0x48, // reg 0x1D IC   Input Clock Register
  0xC0, // reg 0x1E GPIO
  0x80, // reg 0x1F IDF  Input Data Format
  0x00, // reg 0x20 CD   Connection Detect Register
  0x01, // reg 0x21 DC   DAC control
  0x00, // reg 0x23 HPF  Hot plug detection
  0x80, // reg 0x31 TCTL DVI Control Input Register
  0x08, // reg 0x33 TPCP DVI PLL Charge Pump
  0x16, // reg 0x34 TPD  DVI PLL Divider
  0x30, // reg 0x35 TPVT DVI PLL Supply Control
  0x60, // reg 0x36 TPF  DVI PLL Filter
  0x00, // reg 0x37 TCT  DVI Clock Test
  0x18, // reg 0x48 TSTP Test Pattern
  0xC0, // reg 0x49 PM   Power Management  /00 turn dvi off
  0x00  // reg 0x56 DSP  Sync Polarity
  };

const vidconfig_t   default_vid_config_hd = { // >65MHz
  0x00, // reg 0x1C CM   Clock Mode Register
  0x48, // reg 0x1D IC   Input Clock Register
  0xC0, // reg 0x1E GPIO
  0x80, // reg 0x1F IDF  Input Data Format
  0x00, // reg 0x20 CD   Connection Detect Register
  0x01, // reg 0x21 DC   DAC control
  0x00, // reg 0x23 HPF  Hot plug detection
  0x80, // reg 0x31 TCTL DVI Control Input Register
  0x06, // reg 0x33 TPCP DVI PLL Charge Pump
  0x26, // reg 0x34 TPD  DVI PLL Divider
  0x30, // reg 0x35 TPVT DVI PLL Supply Control
  0xA0, // reg 0x36 TPF  DVI PLL Filter
  0x00, // reg 0x37 TCT  DVI Clock Test
  0x18, // reg 0x48 TSTP Test Pattern
  0xC0, // reg 0x49 PM   Power Management  /00 turn dvi off
  0x00  // reg 0x56 DSP  Sync Polarity
  };

void CFG_fatal_error(uint8_t error)
{
  uint8_t i,j;
  ERROR("Fatal error: %02X ", error);
  for (j=0; j<5; j++) {
    for (i = 0; i < error; i++) {
      ACTLED_ON;
      Timer_Wait(250);
      ACTLED_OFF;
      Timer_Wait(250);
    }
    Timer_Wait(1000);
  }
}

/*{{{*/
/*void CFG_handle_fpga(void)*/
/*{*/
    /*uint8_t status;*/

    /*SPI_EnableFpga();*/
    /*SPI(0x10); // status update move to header*/
    /*SPI_DisableFpga();*/

    /*SPI_EnableFpga();*/
    /*SPI(0x00);*/
    /*status = SPI(0); // cmd request*/
    /*// just read first status word to save time*/
    /*SPI_DisableFpga();*/

    /*if (status & 0x08) { // FF request, move to header*/
      /*//DEBUG(1,"FDD:handle request");*/
      /*FDD_Handle();*/
    /*}*/
    /*if (status & 0x80) { // HD request, move to header*/
      /*DEBUG(1,"HDD:handle request %02X",status & 0xF0);*/
      /*HDD_Handle(status);*/
    /*}*/

    /*FDD_UpdateDriveStatus(); // TO GO - only if drive inserted/changed*/
/*}*/
/*}}}*/

// we copy the actual bootloader from flash to the RAM and call it
// (we assume there is no collision with the actual stack)
void CFG_call_bootloader(void)
{
  int i;

  volatile uint32_t *src = (volatile uint32_t *)0x200;
  volatile uint32_t *dest = (volatile uint32_t *)0x00200000;

  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L);
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);
  Timer_Wait(2);

  // copy bootloader to SRAM
  for(i = 0; i < 1024; i++) {
      *dest++ = *src++;
  }

  // launch bootloader in SRAM
  asm("ldr r3, = 0x00200000\n");
  asm("bx  r3\n");
}

void CFG_update_status(status_t *current_status)
{
  current_status->coder_fitted       = IO_Input_L(PIN_CODER_FITTED_L);
  current_status->card_detected      = IO_Input_L(PIN_CARD_DETECT);
  current_status->card_write_protect = IO_Input_H(PIN_WRITE_PROTECT);
}

void CFG_print_status(status_t *current_status)
{

  if (current_status->coder_fitted) {
    DEBUG(1,"CODER: Fitted.");
  } else {
    DEBUG(1,"CODER: Not fitted.");
  }

  if (current_status->card_detected) {
    DEBUG(1,"Memory card detected.");
    if (current_status->card_write_protect) {
      DEBUG(1,"Memory card is write protected.");
    } else {
      DEBUG(1,"Memory card is not write protected.");
    }
  } else {
    DEBUG(1,"No memory card detected.");
  }

}

void CFG_card_start(status_t *current_status)
{
  if (current_status->card_detected) {
    if ( current_status->card_init_ok && current_status->fs_mounted_ok)
      // assume all good
      return;

    if (Card_Init()) { // true is non zero
      DEBUG(1,"Card Init OK.");
      current_status->card_init_ok = TRUE;
    }
    else {
      ERROR("!! Card Init FAIL.");
      current_status->card_init_ok = FALSE;
      CFG_fatal_error(1);  // 1 flash if we failed to init the memory card
    }

    if(FF_MountPartition(pIoman, 0)) {
      DEBUG(1,"!! Could not mount partition.");
      current_status->fs_mounted_ok = FALSE;
      CFG_fatal_error(2); // 2 flash, invalid partition
    } else {
      current_status->fs_mounted_ok = TRUE;
      DEBUG(1,"Partition mounted ok.");
    }

    switch(pIoman->pPartition->Type) {
      case FF_T_FAT32:
        MSG_info("SDCARD: FAT32 formatted"); break;
        //DEBUG(1,"FAT32 Formatted Drive"); break;
      case FF_T_FAT16:
        MSG_info("SDCARD: FAT16 formatted"); break;
        //DEBUG(1,"FAT16 Formatted Drive"); break;
      case FF_T_FAT12:
       MSG_info("SDCARD: FAT12 formatted"); break;
       //DEBUG(1,"FAT12 Formatted Drive"); break;
    }
    //DEBUG(1,"");
    //DEBUG(1,"Block Size: %d",     pIoman->pPartition->BlkSize);
    //DEBUG(1,"Cluster Size: %d kB", (pIoman->pPartition->BlkSize *
    //        pIoman->pPartition->SectorsPerCluster) / 1024);
    //DEBUG(1,"Volume Size: %lu MB", FF_GetVolumeSize(pIoman));
    //DEBUG(1,"");

    MSG_info("SDCARD: %dB/%dkB/%luMB",
                              pIoman->pPartition->BlkSize,
                              (pIoman->pPartition->BlkSize *
                               pIoman->pPartition->SectorsPerCluster) >> 10,
                              FF_GetVolumeSize(pIoman)
            );

  }  else { // no card, maybe removed
    current_status->card_init_ok = FALSE;
    current_status->fs_mounted_ok = FALSE;
    // set SPI clock to 24MHz, perhaps the debugger is being used
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (2 << 8);
  }
}

void CFG_set_coder(coder_t standard)
{
  // Y1 -> 10 - LT_PN  - 18pf
  // Y2 -> 9 - LT_N - 9pf
  // high adds cap
  switch (standard) {
    case CODER_DISABLE :
      DEBUG(2,"CODER: Disabled.");
      AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H | PIN_CODER_CE;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1      | PIN_DRXD_Y2;
      break;

    case CODER_PAL:
      DEBUG(2,"CODER: PAL Y trap.");
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H;
      AT91C_BASE_PIOA->PIO_SODR  = PIN_DTXD_Y1;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_DRXD_Y2;
      break;

    case CODER_NTSC:
      DEBUG(2,"CODER: NTSC Y trap.");
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_NTSC_H;
      AT91C_BASE_PIOA->PIO_SODR  = PIN_DTXD_Y1;
      AT91C_BASE_PIOA->PIO_SODR  = PIN_DRXD_Y2;
      break;

    case CODER_PAL_NOTRAP:
      DEBUG(2,"CODER: PAL no trap.");
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1 | PIN_DRXD_Y2;
      break;

    case CODER_NTSC_NOTRAP:
      DEBUG(2,"CODER: NTSC no trap.");
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
      AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_NTSC_H;
      AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1 | PIN_DRXD_Y2;
      break;

    default :
      ERROR("CODER: Unknown standard.");
  }
}

uint8_t CFG_upload_rom(char *filename, uint32_t base, uint32_t size,
                       uint8_t verify, uint8_t format)
{
  FF_FILE *fSource = NULL;
  fSource = FF_Open(pIoman, filename, FF_MODE_READ, NULL);
  uint8_t rc=1;
  uint32_t offset=0;
  uint16_t filebase=0;

  if (fSource) {
    if (format==1) {
      FF_Seek(fSource, 0, FF_SEEK_SET);
      uint32_t bytes_read = FF_Read(fSource, 2, 1, (uint8_t *)&filebase);
      if (bytes_read == 0) filebase=0;
      else offset=2;
    }
    if (!size) { // auto-size
      size=FF_BytesLeft(fSource);
    }
    DEBUG(1, "%s @0x%X (0x%X),S:%d",filename, base+filebase, offset, size);
    FPGA_FileToMem(fSource, base+filebase, size, offset);
    if (verify) {
      rc=FPGA_FileToMemVerify(fSource, base+filebase, size, offset);
    } else {
      rc=0;
    }
    FF_Close(fSource);
  } else {
    ERROR("Could not open %s", filename);
    return 1;
  }

  return rc;
}

uint8_t CFG_download_rom(char *filename, uint32_t base, uint32_t size)
{
  FF_FILE *fSource = NULL;
  uint8_t rc=1;
  uint32_t offset=0;
  uint16_t filebase=0;

  //DEBUG(1,"CFG_download_rom(\"%s\",%x,%x)",filename,base,size);

  fSource = FF_Open(pIoman, filename,
                    FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE, NULL);

  //DEBUG(1,"FF_Open(%x,\"%s\",%x,%x) --> %x",pIoman, filename,
  //                  FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE, NULL, fSource);

  if (fSource) {
    if (!size) { // auto-size does not work here!
      return 1;
    }
    DEBUG(1, "%s @0x%X (0x%X),S:%d",filename, base+filebase, offset, size);
    rc=FPGA_MemToFile(fSource, base+filebase, size, offset);
    FF_Close(fSource);
  } else {
    WARNING("Could not open %s", filename);
    return 1;
  }

  return rc;
}

uint32_t CFG_get_free_mem(void)
{
  uint8_t  stack;
  uint32_t total;
  void    *heap;
  /* Allocating less than 1kB memory (e.g. only 4 byte)
     could lead to wrong results !!!  */
  heap = malloc(1024);
  total =  (uint32_t)&stack - (uint32_t)heap;
  free(heap);

  DEBUG(3,"===========================");
  DEBUG(3," Free memory %ld bytes",total);
  DEBUG(3,"===========================");
  return (total);
}

uint8_t CFG_configure_fpga(char *filename)
{
  FF_FILE *fSource = NULL;

  fSource = FF_Open(pIoman, filename, FF_MODE_READ, NULL);
  if(fSource) {
    uint8_t retryCounter = 0;
    uint8_t configStatus = 0;

    DEBUG(1,"FPGA configuration: %s found",filename);
    do {
      configStatus = FPGA_Config(fSource);
      FF_Seek(fSource, 0, FF_SEEK_SET);
      retryCounter ++;
    } while (!configStatus && retryCounter<3);

    if (!configStatus) {
      ERROR("!! Failed to configure FPGA.");
      return 4;
    }
    FF_Close(fSource);
  }
  else {
    ERROR("!! %s not found.",filename);
    return 3;
  }
  return 0;
}

void CFG_vid_timing_SD(framerate_t standard)
{
  if (standard == F50HZ)
    Configure_ClockGen(&init_clock_config_pal);
  else
    Configure_ClockGen(&init_clock_config_ntsc);

  // we should turn y1 clock off if coder disabled

  Configure_VidBuf(1, 0, 0, 3); // 9MHZ
  Configure_VidBuf(2, 0, 0, 3);
  Configure_VidBuf(3, 0, 0, 3);
}

void CFG_vid_timing_HD27(framerate_t standard) // for 480P/576P
{
  // PAL/NTSC base clock, but scan doubled. Coder disabled

  if (standard == F50HZ)
    Configure_ClockGen(&init_clock_config_pal);
  else
    Configure_ClockGen(&init_clock_config_ntsc);

  Configure_VidBuf(1, 0, 1, 3); // 16MHz
  Configure_VidBuf(2, 0, 1, 3);
  Configure_VidBuf(3, 0, 1, 3);

}

void CFG_vid_timing_HD74(framerate_t standard) // for 720P etc
{
  // just using 74.24 not /1.001

  Configure_ClockGen(&init_clock_config_hd74); // 60 HZ setup on base clock for now
  Configure_VidBuf(1, 0, 3, 3);
  Configure_VidBuf(2, 0, 3, 3);
  Configure_VidBuf(3, 0, 3, 3);
}

void CFG_set_CH7301_SD(void)
{
  Configure_CH7301(&default_vid_config_sd);
}

void CFG_set_CH7301_HD(void)
{
  Configure_CH7301(&default_vid_config_hd);
}


// ===========================================================================
// INI HANDLING: INITIAL FPGA SETUP
// ===========================================================================
/*
Example INI file (pre-setup part):
------------------------------------------------------------------------------
[SETUP]
bin = replay.bin

# sets initial clocking (PLL)
#       PAL / NTSC
CLOCK = PAL
# or direct setup (here: NTSC)
#          M1 N1 M2 N2 M3 N3      ps0 ps1 ps2 ps3 ps4 ps5 pd0 pd1 pd2 pd3 pd4 pd5 ys0 ys1 ys2 ys3 ys4 ys5
#CLOCK =   33,280,27,133,225,2048,1,1,4,0,0,0,2,16,5,1,1,1,1,1,1,0,1,0

# sets composite/svhs coder, if fitted
coder =  PAL_NOTRAP

# sets video filter for all 3 channels
# dc=250mv, 9MHZ
VFILTER = 0,0,3

# configures what will be available on FPGA after bin is loaded,
# if a line is ommited, the interface will be not used at all (= all zero)
#
# twi passed for further video configuration
#
#         vid
en_twi =   1

# spi for:
#     cfg to send/receive config words
#     osd for OSD video/key support
#
#         cfg osd
en_spi =   1,  1

# replay button configuration
#
#        off / menu / reset
button = menu

[UPLOAD]

# enables verification feature, checks back any ROM/DATA upload
VERIFY = 1

------------------------------------------------------------------------------
*/

uint8_t _CFG_pre_parse_handler(void* status, const ini_symbols_t section,
                               const ini_symbols_t name, const char* value)
{
  status_t *pStatus = (status_t*) status;

  if (section==INI_UNKNOWN) {
    // end of INI file
    DEBUG(3,"callback EOF");
  } else {
    if (name==INI_UNKNOWN) {
      // new [section] line
      DEBUG(3,"callback new section");
    }
    else {
      // new name=value line
      DEBUG(3,"callback [%d] <%d>=\"%s\" ",section, name,value);

      // -------------------------------------------------------
      if (section==INI_SETUP) {
                                       // =====================
        if (name==INI_BIN) {          // ===> SET BIN FILE
          strncpy(pStatus->bin_file,value,sizeof(pStatus->bin_file));
          DEBUG(2,"Will use %s for the FPGA.",pStatus->bin_file);
        }
                                       // =====================
        if (name==INI_CLOCK) {        // ===> PLL/CLOCKING CONFIGURATION
          strcpy(pStatus->clock_bak,value);
          if (MATCH(value,"PAL")) {
            Configure_ClockGen(&init_clock_config_pal);
          }
          else if (MATCH(value,"NTSC")) {
              Configure_ClockGen(&init_clock_config_ntsc);
            }
            else if (MATCH(value,"HD74")) {
                Configure_ClockGen(&init_clock_config_hd74);
              }
              else {
                ini_list_t valueList[32];
                uint16_t entries = ParseList(value,valueList,32);
                if (entries==24) {
                  // temporary map to structure required for setup routine
                  clockconfig_t clkcfg = {
                    valueList[0].intval, valueList[1].intval,       // pll1 m,n
                    valueList[2].intval, valueList[3].intval,       // pll2 m,n
                    valueList[4].intval, valueList[5].intval,       // pll3 m,n
                    { valueList[6].intval, valueList[7].intval,
                      valueList[8].intval, valueList[9].intval,
                      valueList[10].intval,valueList[11].intval, },//p0,1,2,3,4,5
                    { valueList[12].intval,valueList[13].intval,
                      valueList[14].intval,valueList[15].intval,
                      valueList[16].intval,valueList[17].intval, },//d0,1,2,3,4,5
                    { valueList[18].intval,valueList[19].intval,
                      valueList[20].intval,valueList[21].intval,
                      valueList[22].intval,valueList[23].intval  } //y0,1,2,3,4,5
                  };
                  Configure_ClockGen(&clkcfg);
                }
                else return 1;
              }
        }
                                       // =====================
        if (name==INI_CODER) {        // ===> CODER CONFIGURATION (OPTIONAL)
          coder_t codcfg;
          strcpy(pStatus->coder_bak,value);
          if (MATCH(value,"DISABLE")) {
            codcfg=CODER_DISABLE;
          }
          else if (MATCH(value,"PAL")) {
              codcfg=CODER_PAL;
            }
            else if (MATCH(value,"NTSC")) {
                codcfg=CODER_NTSC;
              }
              else if (MATCH(value,"PAL_NOTRAP")) {
                  codcfg=CODER_PAL_NOTRAP;
                }
                else if (MATCH(value,"NTSC_NOTRAP")) {
                    codcfg=CODER_NTSC_NOTRAP;
                  }
                  else return 1;
          if (pStatus->coder_fitted) {
            CFG_set_coder(codcfg);
          } else {
            DEBUG(1,"No coder fitted to update!");
          }
        }
                                       // =====================
        if (name==INI_VFILTER) {      // ===> VIDEO FILTER CONFIGURATION
          ini_list_t valueList[8];
          strcpy(pStatus->vfilter_bak,value);
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==3) {
            Configure_VidBuf(1, valueList[0].intval, valueList[1].intval, valueList[2].intval);
            Configure_VidBuf(2, valueList[0].intval, valueList[1].intval, valueList[2].intval);
            Configure_VidBuf(3, valueList[0].intval, valueList[1].intval, valueList[2].intval);
          }
          else return 1;
        }
                                       // =====================
        if (name==INI_PHASE) {       // ===> DRAM PHASE CONFIG
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            pStatus->dram_phase=valueList[0].intval;
            DEBUG(1,"DRAM phase: %d",pStatus->dram_phase);
          }
          else return 1;
        }
                                       // =====================
        if (name==INI_EN_TWI) {       // ===> ALLOW TWI POST-CONFIGURATIONS
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            pStatus->twi_enabled=valueList[0].intval;
            DEBUG(1,"TWI passthrough: %s",pStatus->twi_enabled?"ENABLED":"-");
          }
          else return 1;
        }
                                       // =====================
        if (name==INI_EN_SPI) {       // ===> ALLOW SPI POST-CONFIGURATIONS/ACCESS
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==2) {
            pStatus->spi_fpga_enabled=valueList[0].intval;
            pStatus->spi_osd_enabled=valueList[1].intval;
            DEBUG(1,"SPI control: %s %s ",pStatus->spi_fpga_enabled?"FPGA":"-"
                                ,pStatus->spi_osd_enabled?"OSD":"-");
          }
          else return 1;
        }
                                       // =====================
        if (name==INI_SPI_CLK) {       // ===> SPI CLOCK SETUP
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            // we allow only reducing clock to avoid issues with sdcard settings
            if (valueList[0].intval>
                ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8)) {
              pStatus->spiclk_old = ((AT91C_BASE_SPI->SPI_CSR[0] &
                                       AT91C_SPI_SCBR) >> 8);
              AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL |
                                                     (valueList[0].intval<<8);
              uint32_t spiFreq = BOARD_MCK /
                       ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8) /
                       1000000;
              DEBUG(1,"New SPI clock: %d MHz",spiFreq);
              pStatus->spiclk_bak = valueList[0].intval;
            }
          }
          else return 1;
        }
                                       // =====================
        if (name==INI_BUTTON) {       // ===> BUTTON CONFIGURATION
          if (MATCH(value,"OFF"))        pStatus->button=BUTTON_OFF;
          else if (MATCH(value,"RESET")) pStatus->button=BUTTON_RESET;
          else if (MATCH(value,"MENU"))  pStatus->button=BUTTON_MENU;
          else return 1;
          DEBUG(1,"Button control: %s",value);
        }
      }
      // -------------------------------------------------------
      if (section==INI_UPLOAD) {
                                       // =====================
        if (name==INI_VERIFY) {       // ===> ENABLES VERIFICATIONS OF UPLOADS
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            pStatus->verify_dl=valueList[0].intval;
            DEBUG(1,"UPLOAD verification: %s",pStatus->verify_dl?"ENABLED":"DISABLED");
          }
          else return 1;
        }
      }
    }
  }
  return 0;
}

uint8_t CFG_pre_init(status_t *currentStatus, const char *iniFile)
{
  FF_FILE *fIni = NULL;

  DEBUG(2,"--- PRE-SETUP INI RUN ---");

  if (currentStatus->fs_mounted_ok) {

    DEBUG(1,"Looking for %s (pre-init)",iniFile);

    fIni = FF_Open(pIoman, iniFile, FF_MODE_READ, NULL);

    if(fIni) {

      uint8_t status = ParseIni(fIni, _CFG_pre_parse_handler, currentStatus);
      FF_Close(fIni);

      if (status !=0 ) {
        ERROR("Execution stopped at INI line %d",status);
        return 1;
      }

    }
    else {
      MSG_error("INI file not found");
      //ERROR("Ini file not found.");
      return 1;

    }

  }

  DEBUG(2,"PRE-INI processing done");
  return 0;
}

// ===========================================================================
// INI HANDLING: POST FPGA SETUP
// ===========================================================================
/*
Example INI file (post-setup part):
------------------------------------------------------------------------------
[SETUP]

# VGA/DVI settings, "EN_TWI" must be set to one !
#           0x1c 0x1d 0x1e 0x1f 0x20 0x21 0x23 0x31 0x33 0x34 0x35 0x36 0x37 0x48 0x49 0x56
video =     0x00,0x48,0xC0,0x80,0x00,0x01,0x00,0x80,0x08,0x16,0x30,0x60,0x00,0x18,0xC0,0x00

[UPLOAD]

# first to load (files relative to this ini file)
#
# name = file1.rom
# size = 0x4000
# addr = 0x12345678
ROM = file1.rom,0x4000,0x12345678

# start after previous ROM (or zero, if first entry), length given
#
# name = file2.rom
# size = 0x2000
# addr = 0x12345678+0x4000
ROM = file2.rom,0x2000

# arbitrary data upload (up to 16 byte), always to a given address
# (does not change ROM loader address and is thus idependent to use)
#
# list of data (bytes), address, number of bytes (= brief check of validity)
DATA = 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x98765432,8

# 2x32bit static+dynamic configuration bits, "CFG" in EN_SPI must be set to one!
#        31                             0,31                             0
config = 10101000111001101010100011100110,10101000111001101010100011100110

[MENU]

# menu entry
title = "Video Settings"

#         text            mask          static/dynamic (or just d/s)
item = "Filter",     0x00003000, dynamic

#              text       value           none/default
option = "H only", 0x00001000, default
option = "V only", 0x00002000
option = "H+V",    0x00003000

#         text      mask  shift    static/dynamic
item = "Filter",     3,   12,     dynamic

#              text       value   none/default
option = "H only",   1,        default
option = "V only",   2
option = "H+V",      3

------------------------------------------------------------------------------
*/

uint8_t _CFG_parse_handler(void* status, const ini_symbols_t section,
                           const ini_symbols_t name, const char* value)
{
  status_t *pStatus = (status_t*) status;

  if (section==INI_START) {
    // start of INI file
    DEBUG(3,"callback SOF");
  }

  if (section==INI_UNKNOWN) {
    // end of INI file
    DEBUG(3,"callback EOF");
  } else {
    if (name==INI_UNKNOWN) {
      // new [section] line
      DEBUG(3,"callback new section");
    }
    else {
      // new name=value line
      DEBUG(3,"callback [%d] <%d>=\"%s\" ",section, name,value);

      // -------------------------------------------------------
      if (section==INI_SETUP) {
                                        // =====================
        if (name==INI_VIDEO) {          // ===> FINAL VIDEO CONFIGURATION
          strcpy(pStatus->video_bak,value);
          if (pStatus->twi_enabled) {
            ini_list_t valueList[32];
            uint16_t entries = ParseList(value,valueList,32);
            if (entries==16) {
              const vidconfig_t vid_config = {
                valueList[ 0].intval,valueList[ 1].intval,
                valueList[ 2].intval,valueList[ 3].intval,
                valueList[ 4].intval,valueList[ 5].intval,
                valueList[ 6].intval,valueList[ 7].intval,
                valueList[ 8].intval,valueList[ 9].intval,
                valueList[10].intval,valueList[11].intval,
                valueList[12].intval,valueList[13].intval,
                valueList[14].intval,valueList[15].intval,
              };
              Configure_CH7301(&vid_config);
            }
            else return 1;
            DEBUG(2,"VIDEO: %s",value);
          } else {
            MSG_error("VIDEO config but TWI disabled");
            //ERROR("VIDEO not allowed, ignored");
          }
        }
                                        // =====================
        if (name==INI_CONFIG) {        // ===> SET CONFIGURATION BITS
          if (pStatus->spi_fpga_enabled) {
            ini_list_t valueList[8];
            uint16_t entries = ParseList(value,valueList,8);
            if (entries==2) {
              pStatus->config_s = valueList[0].intval;
              pStatus->config_d = valueList[1].intval;
            }
            else return 1;
            DEBUG(2,"CONFIG: static=0x%08X, dynamic=0x%08X",
                    pStatus->config_s,pStatus->config_d);
          } else {
            MSG_error("FPGA config but SPI disabled");
            //ERROR("CONFIG not allowed, ignored");
          }
        }
                                        // =====================
        if (name==INI_INFO) {          // ===> SHOW INFO LINE
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            // we do this to get rid of ""
            if (pStatus->info_bak) {
              pStatus->info_bak_last->next=malloc(sizeof(info_list_t));
              pStatus->info_bak_last=pStatus->info_bak_last->next;
            } else {
              pStatus->info_bak=malloc(sizeof(info_list_t));
              pStatus->info_bak_last=pStatus->info_bak;
            }
            pStatus->info_bak_last->next=NULL;
            strcpy(pStatus->info_bak_last->info_bak,valueList[0].strval);

            MSG_info(valueList[0].strval);
          }
        }
      }
      // -------------------------------------------------------
      if (section==INI_UPLOAD) {
                                       // =====================
        if (name==INI_ROM) {          // ===> UPLOAD A ROM FILE
                                       // (name is relative to INI path)
          if (pStatus->rom_bak) {
            pStatus->rom_bak_last->next=malloc(sizeof(rom_list_t));
            pStatus->rom_bak_last=pStatus->rom_bak_last->next;
          } else {
            pStatus->rom_bak=malloc(sizeof(rom_list_t));
            pStatus->rom_bak_last=pStatus->rom_bak;
          }
          pStatus->rom_bak_last->next=NULL;
          strcpy(pStatus->rom_bak_last->rom_bak,value);
          if (pStatus->spi_fpga_enabled) {
            ini_list_t valueList[8];
            uint16_t entries = ParseList(value,valueList,8);
            if ((entries==4) && (strlen(valueList[0].strval))) {
              char fullname[FF_MAX_PATH];
              // set address properly
              pStatus->last_rom_adr = valueList[2].intval+valueList[1].intval;
              // prepare filename
              sprintf(fullname,"%s%s",pStatus->ini_dir,valueList[0].strval);
              DEBUG(2,"ROM upload @ 0x%08lX (%ld byte)",
                      valueList[2].intval,valueList[1].intval);
              if (CFG_upload_rom(fullname, valueList[2].intval,valueList[1].intval,
                                 pStatus->verify_dl,valueList[3].intval)) {
                MSG_error("ROM upload to FPGA failed");
                return 1;
              } else {
                return 0;
              }
            }
            else if ((entries==3) && (strlen(valueList[0].strval))) {
              char fullname[FF_MAX_PATH];
              // set address properly
              pStatus->last_rom_adr = valueList[2].intval+valueList[1].intval;
              // prepare filename
              sprintf(fullname,"%s%s",pStatus->ini_dir,valueList[0].strval);
              DEBUG(2,"ROM upload @ 0x%08lX (%ld byte)",
                      valueList[2].intval,valueList[1].intval);
              if (CFG_upload_rom(fullname, valueList[2].intval,valueList[1].intval,
                                 pStatus->verify_dl,0)) {
                MSG_error("ROM upload to FPGA failed");
                return 1;
              } else {
                return 0;
              }
            }
            else if ((entries==2) && (strlen(valueList[0].strval))) {
              char fullname[FF_MAX_PATH];
              uint32_t adr = pStatus->last_rom_adr;
              // increment address properly
              pStatus->last_rom_adr = adr + valueList[1].intval;
              // prepare filename
              sprintf(fullname,"%s%s",pStatus->ini_dir,valueList[0].strval);
              DEBUG(2,"ROM upload @ 0x%08lX (%ld byte)",
                      adr,valueList[1].intval);
              if (CFG_upload_rom(fullname, adr, valueList[1].intval,
                                 pStatus->verify_dl, 0)) {
                MSG_error("ROM upload to FPGA failed");
                return 1;
              } else {
                return 0;
              }
            }
            else return 1;
          } else {
            MSG_error("ROM upload, but SPI disabled");
            //ERROR("ROM not allowed, ignored");
          }
        }
                                       // =====================
        if (name==INI_DATA) {          // ===> UPLOAD A DATA SET
                                       //      (name is relative to INI path)
          if (pStatus->data_bak) {
            pStatus->data_bak_last->next=malloc(sizeof(data_list_t));
            pStatus->data_bak_last=pStatus->data_bak_last->next;
          } else {
            pStatus->data_bak=malloc(sizeof(data_list_t));
            pStatus->data_bak_last=pStatus->data_bak;
          }
          pStatus->data_bak_last->next=NULL;
          strcpy(pStatus->data_bak_last->data_bak,value);
          if (pStatus->spi_fpga_enabled) {
            ini_list_t valueList[32];
            uint16_t entries = ParseList(value,valueList,32);
            // check for maximum of 16 entries
            // (plus address and dataset length), check both lengths
            if ((entries<=18) && ((entries-2)==valueList[entries-1].intval)) {
              uint8_t buf[16];
              for(int i=0;i<(entries-2);buf[i]=valueList[i].intval,i++);
              DEBUG(2,"Data upload @ 0x%08lX (%ld byte)",
                      valueList[entries-2].intval,entries-2);
              if (FPGA_BufToMem(buf, valueList[entries-2].intval,
                                     valueList[entries-1].intval)) {
                MSG_error("DATA upload to FPGA failed");
                return 1;
              }
              if (pStatus->verify_dl) {
                // required to readback data and check it again...
                uint8_t tmpbuf[16];
                FPGA_MemToBuf(tmpbuf, valueList[entries-2].intval,
                                      valueList[entries-1].intval);
                for(int i=0;i<(entries-2);i++) {
                  if (tmpbuf[i]!=buf[i]) {
                    MSG_error("DATA verification failed");
                    //ERROR("Data upload failed.");
                    return 1;
                  }
                }
              }
              else {
                return FPGA_BufToMem(buf, valueList[entries-2].intval,
                                          valueList[entries-1].intval);
              }
            }
            else return 1;
          } else {
            MSG_error("DATA upload, but SPI disabled");
            //ERROR("DATA not allowed, ignored");
          }
        }
                                      // =====================
        if (name==INI_LAUNCH) {       // ===> Get data from FPGA and execute
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==3) {
            uint32_t start   =valueList[0].intval;
            uint32_t length  =valueList[1].intval;
            uint32_t checksum=valueList[2].intval;
            DEBUG(2,"LAUNCH FROM MEMORY @ 0x%lx, l:%d (s:0x%x),",start,length,checksum);
            FPGA_ExecMem(start,(uint16_t)length,checksum);
          }
          else return 1;
        }
      }
      // -------------------------------------------------------
      if (section==INI_MENU) {
                                        // =====================
        if (name==INI_TITLE) {          // ===> set new menu title
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          if (entries==1) {
            DEBUG(2,"TITLE: %s ",value);
            DEBUG(3,"T1: %lx %lx %lx ",pStatus->menu_act,
                    pStatus->menu_act->next,pStatus->menu_act->item_list);

            // we filled this menu branch already with items
            if (pStatus->menu_top) {   // add further entry
              // prepare next level and set pointers correctly
              pStatus->menu_act->next = malloc(sizeof(menu_t));
              // link back
              pStatus->menu_act->next->last = pStatus->menu_act;
              // step in linked list
              pStatus->menu_act = pStatus->menu_act->next;
            } else {                   // first top entry
              // prepare top level
              pStatus->menu_act = malloc(sizeof(menu_t));
              pStatus->menu_act->last=NULL;
              // set top level
              pStatus->menu_top = pStatus->menu_act;
            }
            pStatus->menu_act->next=NULL;
            pStatus->menu_act->item_list = malloc(sizeof(menuitem_t));
            pStatus->menu_item_act = pStatus->menu_act->item_list;
            pStatus->menu_item_act->item_name[0]=0;
            pStatus->menu_item_act->next=NULL;
            pStatus->menu_item_act->last=NULL;
            pStatus->menu_item_act->option_list=NULL;
            pStatus->menu_item_act->selected_option=NULL;
            pStatus->menu_item_act->action_name[0]=0;
            // store title to actual branch
            strncpy(pStatus->menu_act->menu_title,
                    valueList[0].strval,MAX_MENU_STRING);

            DEBUG(3,"T2: %lx %lx %lx ",pStatus->menu_act,
                    pStatus->menu_act->next,pStatus->menu_act->item_list);
          } else {
            return 1;
          }
        }
                                        // =====================
        if (name==INI_ITEM) {           // ===> add a new menu item
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,32);
          if ((entries==2) || (entries==3) || (entries==4)) {
            DEBUG(2,"ITEM: %s ",value);
            DEBUG(3,"I1: %lx %lx %lx ",pStatus->menu_item_act,
                    pStatus->menu_item_act->next,
                    pStatus->menu_item_act->option_list);
            // item string is set, so this is a new item
            // (happens with all but the seed item entry of a menu)
            if (pStatus->menu_item_act->item_name[0]) {
              pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
              // link back
              pStatus->menu_item_act->next->last = pStatus->menu_item_act;
              // step in linked list
              pStatus->menu_item_act = pStatus->menu_item_act->next;
            }
            // basic settings
            pStatus->menu_item_act->next=NULL;
            pStatus->menu_item_act->option_list=NULL;
            pStatus->menu_item_act->selected_option=NULL;
            pStatus->menu_item_act->action_name[0]=0;
            strncpy(pStatus->menu_item_act->item_name,
                    valueList[0].strval,MAX_MENU_STRING);
            // special item with own handler, options handled there or ignored
            if (entries==2) {
              strncpy(pStatus->menu_item_act->action_name,valueList[1].strval,
                      MAX_ITEM_STRING);
              pStatus->menu_item_act->action_value=0;
            }
            // absolute mask item or action handler with value
            if (entries==3) {
              pStatus->menu_item_act->conf_mask = valueList[1].intval;
              if (MATCH(valueList[2].strval,"DYNAMIC") ||
                  MATCH(valueList[2].strval,"DYN") ||
                  MATCH(valueList[2].strval,"D")) {
                pStatus->menu_item_act->conf_dynamic=1;
              }
              else if (MATCH(valueList[2].strval,"STATIC") ||
                       MATCH(valueList[2].strval,"STAT") ||
                       MATCH(valueList[2].strval,"S")) {
                pStatus->menu_item_act->conf_dynamic=0;
              } else {
                strncpy(pStatus->menu_item_act->action_name,
                        valueList[1].strval, MAX_ITEM_STRING);
                pStatus->menu_item_act->action_value=valueList[2].intval;
              }
            }
            // value/shift mask item
            if (entries==4) {
              pStatus->menu_item_act->conf_mask = valueList[1].intval <<
                                                   valueList[2].intval;
              if (MATCH(valueList[3].strval,"DYNAMIC") ||
                  MATCH(valueList[3].strval,"DYN") ||
                  MATCH(valueList[3].strval,"D")) {
                pStatus->menu_item_act->conf_dynamic=1;
              }
              else if (MATCH(valueList[3].strval,"STATIC") ||
                       MATCH(valueList[3].strval,"STAT") ||
                       MATCH(valueList[3].strval,"S")) {
                pStatus->menu_item_act->conf_dynamic=0;
              } else {
                pStatus->menu_item_act->conf_dynamic=0;
                MSG_error("Menu items must be static or dynamic");
                //ERROR("Illegal item type, only static or dynamic allowed");
                return 1;
              }
            }
            pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));
            pStatus->item_opt_act = pStatus->menu_item_act->option_list;
            pStatus->item_opt_act->next=NULL;
            pStatus->item_opt_act->last=NULL;
            pStatus->item_opt_act->option_name[0]=0;
            DEBUG(3,"I2: %lx %lx %lx ",pStatus->menu_item_act,
                    pStatus->menu_item_act->next,
                    pStatus->menu_item_act->option_list);
          } else {
            return 1;
          }
        }
                                        // =====================
        if (name==INI_OPTION) {         // ===> add an option to an item
          ini_list_t valueList[8];
          uint8_t nocheck=0;
          uint16_t entries = ParseList(value,valueList,32);
          if ((entries==2)||(entries==3)) {
            DEBUG(2,"OPTION: %s ",value);
            DEBUG(3,"O1: %lx %lx",pStatus->item_opt_act,
                                      pStatus->item_opt_act->next);
            // option string is set, so we add a new one
            // (happens with all but the seed option entry of a menu item)
            if (pStatus->item_opt_act->option_name[0]) {
              pStatus->item_opt_act->next = malloc(sizeof(menuitem_t));
              // link back
              pStatus->item_opt_act->next->last = pStatus->item_opt_act;
              // step in linked list
              pStatus->item_opt_act = pStatus->item_opt_act->next;
              pStatus->item_opt_act->next=NULL;
            }
            pStatus->item_opt_act->conf_value=valueList[1].intval;
            if (entries==3) {
              if (MATCH(valueList[2].strval,"FLAGS") ||
                  MATCH(valueList[2].strval,"F")) {
                nocheck=1;
              } else {
                if (MATCH(valueList[2].strval,"DEFAULT") ||
                    MATCH(valueList[2].strval,"DEF") ||
                    MATCH(valueList[2].strval,"D")) {
                  pStatus->menu_item_act->selected_option=pStatus->item_opt_act;
                } else {
                  MSG_error("bad option type - use 'default' or none");
                  //ERROR("Illegal option type (\"default\" or none)");
                  return 1;
                }
              }
            }
            if ((pStatus->menu_item_act->conf_mask &
                 pStatus->item_opt_act->conf_value) !=
                 pStatus->item_opt_act->conf_value) {
              if (!nocheck) {
                MSG_error("item mask does not fit to value");
                return 1;
              }
            }
            strncpy(pStatus->item_opt_act->option_name,
                    valueList[0].strval,MAX_OPTION_STRING);
            DEBUG(3,"O2: %lx %lx",pStatus->item_opt_act,
                  pStatus->item_opt_act->next);
          } else {
            return 1;
          }
        }
      }
      // -------------------------------------------------------
      if (section==INI_FILES) {
        if (name==INI_HDD) {
          ini_list_t valueList[8];
          uint16_t entries = ParseList(value,valueList,8);
          uint8_t  unit = 0;

          DEBUG(1,"hdd entries %d",entries);
          if ((entries==1) || (entries==2)) {
            if (entries==2)
              unit = valueList[1].intval;

            if (unit > 1) {
              DEBUG(1,"Illegal HDD unit number")
            } else {
              if (strlen(valueList[0].strval)) {
                char fullname[FF_MAX_PATH];
                // prepare filename
                sprintf(fullname,"%s%s",pStatus->ini_dir,valueList[0].strval);
                DEBUG(1,"HDD file %s", fullname);
                HDD_OpenHardfile(fullname, unit); // MASTER ONLY FOR NOW
              };
            };
          }
        }
      }
      // -------------------------------------------------------
    }
  }
  return 0;
}

uint8_t CFG_init(status_t *currentStatus, const char *iniFile)
{
//  uint32_t i = 0;
//  uint16_t key = 0;
//  char s[OSDMAXLEN]; // watch size

  // POST FPGA BOOT CONFIG
  DEBUG(2,"--- POST-SETUP INI RUN ---");

  // reset ROM address, in case it is initially not stated in INI
  currentStatus->last_rom_adr=0;

  if (OSD_ConfigReadSysconVer() != 0xA5) {
    DEBUG(1,"!! Comms failure. FPGA Syscon not detected !!");
    return 1; // can do no more here
  }

  uint32_t config_ver    = OSD_ConfigReadVer();
  uint32_t config_stat   = OSD_ConfigReadStatus();
  uint32_t config_fileio = OSD_ConfigReadFileIO();

  if (!currentStatus->dram_phase) {
    OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
  } else {
    if (abs(currentStatus->dram_phase)<21) {
      INFO("DRAM phase fix: %d -> %d",kDRAM_PHASE,kDRAM_PHASE + currentStatus->dram_phase);
      OSD_ConfigSendCtrl((kDRAM_SEL << 8) | (kDRAM_PHASE + currentStatus->dram_phase)); // phase from INI
    } else {
      WARNING("DRAM phase value bad, ignored!");
    }
  }

  // reset core and halt it for now
  OSD_Reset(OSDCMD_CTRL_RES|OSDCMD_CTRL_HALT);
  Timer_Wait(100);

  // initialize and check DDR RAM
  if (config_ver & 0x8000) {
    FPGA_DramTrain();
  }

  uint32_t init_mem = CFG_get_free_mem();
  DEBUG(1,"Initial free MEM: %ld bytes", init_mem);

  if (currentStatus->fs_mounted_ok) {
    FF_FILE *fIni = NULL;
    DEBUG(1,"Looking for %s (post-init)",iniFile);

    fIni = FF_Open(pIoman, iniFile, FF_MODE_READ, NULL);
    if(fIni) {
      int32_t status = ParseIni(fIni, _CFG_parse_handler, currentStatus);
      FF_Close(fIni);
      if (status !=0 ) {
        ERROR("Execution stopped at INI line %d",status);
        return status;
      }
    }
    else {
      MSG_error("INI file not found");
      //ERROR("Ini file not found.");
      return 1;
    }
  }

  // POLL FPGA and see how many FD/HD supported
  uint32_t fileio_cfg = OSD_ConfigReadFileIO();

  currentStatus->fd_supported =  fileio_cfg       & 0x0F;
  currentStatus->hd_supported = (fileio_cfg >> 4) & 0x0F;

  DEBUG(1,"FD supported : "BYTETOBINARYPATTERN4", HD supported : "BYTETOBINARYPATTERN4, 
    BYTETOBINARY4(currentStatus->fd_supported), 
    BYTETOBINARY4(currentStatus->hd_supported) );


  // TEMP config to 1 floppy, 1 hard disk
  /*uint8_t hd_mask = 0;*/
  /*if (hdf[0].status & HD_INSERTED) hd_mask |= 0x10;*/
  /*if (hdf[1].status & HD_INSERTED) hd_mask |= 0x20;*/
  /*DEBUG(1,"HD MASK %02X",hd_mask);*/
  /*OSD_ConfigSendFileIO(hd_mask | 0x01);*/
  // 0x22 write config (fileio)       W0       number of disks 2..0 FD 7..4 HD note FD is a number 0-4 HD is a MASK

  init_mem -= CFG_get_free_mem();
  DEBUG(1,"Final free MEM:   %ld bytes", CFG_get_free_mem());
  if (init_mem) DEBUG(0,"Menu requires %ld bytes", init_mem);

  sprintf(currentStatus->status[0], "ARM |FW:%s (%ldkB free)", version,
                                      CFG_get_free_mem()>>10);
  sprintf(currentStatus->status[1], "FPGA|FW:%04X STAT:%04x IO:%04X",
                                      config_ver, config_stat, config_fileio);

  // TODO: ini path may exceed length and crash system here!
  sprintf(currentStatus->status[2], "INI |%s", iniFile);

  // check all config bits
  _MENU_update_bits(currentStatus);

  // reset core and remove halt
  OSD_Reset(OSDCMD_CTRL_RES);
  Timer_Wait(100);

  DEBUG(2,"POST-INI processing done");

  // temp
  if (config_ver == 0x80FF ) {
    DEBUG(2,"Executing production test.");
    FPGA_ProdTest();
  }

  return 0;
}

void CFG_add_default(status_t *currentStatus)
{
  status_t *pStatus = (status_t*) currentStatus;

  // add default menu entry
  if (pStatus->menu_top) {   // add further entry
    // prepare next level and set pointers correctly
    pStatus->menu_act->next = malloc(sizeof(menu_t));
    // link back
    pStatus->menu_act->next->last = pStatus->menu_act;
    // step in linked list
    pStatus->menu_act = pStatus->menu_act->next;
  } else {                   // first top entry
    // prepare top level
    pStatus->menu_act = malloc(sizeof(menu_t));
    pStatus->menu_act->last=NULL;
    // set top level
    pStatus->menu_top = pStatus->menu_act;
  }
  pStatus->menu_act->next=NULL;
  strcpy(pStatus->menu_act->menu_title,"Replay Menu");
  pStatus->menu_act->item_list = malloc(sizeof(menuitem_t));
  pStatus->menu_item_act = pStatus->menu_act->item_list;
  strcpy(pStatus->menu_item_act->item_name,"Load Target");
  pStatus->menu_item_act->next= malloc(sizeof(menuitem_t));
  pStatus->menu_item_act->next->last = pStatus->menu_item_act;
  pStatus->menu_item_act->last=NULL;
  pStatus->menu_item_act->option_list=NULL;
  pStatus->menu_item_act->selected_option=NULL;
  pStatus->menu_item_act->conf_dynamic=0;
  pStatus->menu_item_act->conf_mask=0;
  strcpy(pStatus->menu_item_act->action_name,"iniselect");
  pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));
  pStatus->item_opt_act = pStatus->menu_item_act->option_list;
  pStatus->item_opt_act->next=NULL;
  pStatus->item_opt_act->last=NULL;
  pStatus->item_opt_act->option_name[0]=0;

  pStatus->menu_item_act = pStatus->menu_item_act->next;
  strcpy(pStatus->menu_item_act->item_name,"Reset Target");
  pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
  pStatus->menu_item_act->next->last=pStatus->menu_item_act;
  pStatus->menu_item_act->option_list=NULL;
  pStatus->menu_item_act->selected_option=NULL;
  pStatus->menu_item_act->conf_dynamic=0;
  pStatus->menu_item_act->conf_mask=0;
  strcpy(pStatus->menu_item_act->action_name,"reset");
  pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));
  pStatus->item_opt_act = pStatus->menu_item_act->option_list;
  pStatus->item_opt_act->next=NULL;
  pStatus->item_opt_act->last=NULL;
  pStatus->item_opt_act->option_name[0]=0;

  pStatus->menu_item_act = pStatus->menu_item_act->next;
  strcpy(pStatus->menu_item_act->item_name,"Config Backup");
  pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
  pStatus->menu_item_act->next->last=pStatus->menu_item_act;
  pStatus->menu_item_act->option_list=NULL;
  pStatus->menu_item_act->selected_option=NULL;
  pStatus->menu_item_act->conf_dynamic=0;
  pStatus->menu_item_act->conf_mask=0;
  strcpy(pStatus->menu_item_act->action_name,"backup");
  pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));
  pStatus->item_opt_act = pStatus->menu_item_act->option_list;
  pStatus->item_opt_act->next=NULL;
  pStatus->item_opt_act->last=NULL;
  pStatus->item_opt_act->option_name[0]=0;

  pStatus->menu_item_act = pStatus->menu_item_act->next;
  strcpy(pStatus->menu_item_act->item_name,"Reboot Board");
  pStatus->menu_item_act->next=NULL;
  pStatus->menu_item_act->option_list=NULL;
  pStatus->menu_item_act->selected_option=NULL;
  pStatus->menu_item_act->conf_dynamic=0;
  pStatus->menu_item_act->conf_mask=0;
  strcpy(pStatus->menu_item_act->action_name,"reboot");
  pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));
  pStatus->item_opt_act = pStatus->menu_item_act->option_list;
  pStatus->item_opt_act->next=NULL;
  pStatus->item_opt_act->last=NULL;
  pStatus->item_opt_act->option_name[0]=0;
}

void CFG_free_menu(status_t *currentStatus)
{
  DEBUG(2,"--- FREE MENU (and backup) SPACE ---");

  currentStatus->menu_act = currentStatus->menu_top;
  while (currentStatus->menu_act) {
    void *p;
    DEBUG(3,"T:%08lx >%08lx <%08lx ><%08lx",currentStatus->menu_act,
            currentStatus->menu_act->next,currentStatus->menu_act->last,
            currentStatus->menu_act->next->last);
    currentStatus->menu_item_act = currentStatus->menu_act->item_list;
    while (currentStatus->menu_item_act) {
      DEBUG(3,"I:%08lx >%08lx <%08lx ><%08lx",currentStatus->menu_item_act,
              currentStatus->menu_item_act->next,
              currentStatus->menu_item_act->last,
              currentStatus->menu_item_act->next->last);
      currentStatus->item_opt_act = currentStatus->menu_item_act->option_list;
      while (currentStatus->item_opt_act) {
        DEBUG(3,"O:%08lx >%08lx <%08lx ><%08lx",
                currentStatus->item_opt_act,currentStatus->item_opt_act->next,
                currentStatus->item_opt_act->last,
                currentStatus->item_opt_act->next->last);
        p=currentStatus->item_opt_act;
        currentStatus->item_opt_act = currentStatus->item_opt_act->next;
        free(p);
      }
      p=currentStatus->menu_item_act;
      currentStatus->menu_item_act = currentStatus->menu_item_act->next;
      free(p);
    }
    p=currentStatus->menu_act;
    currentStatus->menu_act = currentStatus->menu_act->next;
    free(p);
  }
  currentStatus->menu_top = NULL;
  currentStatus->menu_act = NULL;
  currentStatus->menu_item_act = NULL;
  currentStatus->item_opt_act = NULL;
}

void CFG_free_bak(status_t *currentStatus)
{
  currentStatus->rom_bak_last=currentStatus->rom_bak;
  while (currentStatus->rom_bak_last) {
    rom_list_t *p = currentStatus->rom_bak_last->next;
    free(currentStatus->rom_bak_last);
    currentStatus->rom_bak_last=p;
  }
  currentStatus->rom_bak=NULL;
  currentStatus->rom_bak_last=NULL;

  currentStatus->data_bak_last=currentStatus->data_bak;
  while (currentStatus->data_bak_last) {
    data_list_t *p = currentStatus->data_bak_last->next;
    free(currentStatus->data_bak_last);
    currentStatus->data_bak_last=p;
  }
  currentStatus->data_bak=NULL;
  currentStatus->data_bak_last=NULL;

  currentStatus->info_bak_last=currentStatus->info_bak;
  while (currentStatus->info_bak_last) {
    info_list_t *p = currentStatus->info_bak_last->next;
    free(currentStatus->info_bak_last);
    currentStatus->info_bak_last=p;
  }
  currentStatus->info_bak=NULL;
  currentStatus->info_bak_last=NULL;

  currentStatus->clock_bak[0]=0;
  currentStatus->coder_bak[0]=0;
  currentStatus->vfilter_bak[0]=0;
  currentStatus->video_bak[0]=0;

  currentStatus->spiclk_bak=0;
  if (currentStatus->spiclk_old) {
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL |
                                  (currentStatus->spiclk_old<<8);
  }
  currentStatus->spiclk_old=0;
}

void _CFG_write_str(FF_FILE *fIni, char *str)
{
  uint32_t len = strlen(str);
  FF_Write (fIni, len, 1, (uint8_t *)str);
}

void _CFG_write_strln(FF_FILE *fIni, char *str)
{
  _CFG_write_str(fIni,str);
  _CFG_write_str(fIni,"\r\n");
}

void CFG_save_all(status_t *currentStatus, const char *iniDir,
                  const char *iniFile)
{
  FF_FILE *fIni = NULL;
  char full_filename[FF_MAX_PATH];
  sprintf(full_filename,"%s%s",iniDir,iniFile);

  if (currentStatus->fs_mounted_ok) {
    DEBUG(1,"Writing %s",full_filename);

    fIni = FF_Open(pIoman, full_filename,
                   FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE, NULL);

    if(fIni) {
      char s[128];

      // SETUP section
      _CFG_write_strln(fIni,"[SETUP]");
      sprintf(s,"bin = %s",currentStatus->bin_file);
      _CFG_write_strln(fIni,s);

      sprintf(s,"clock = %s",currentStatus->clock_bak);
      _CFG_write_strln(fIni,s);

      sprintf(s,"coder = %s",currentStatus->coder_bak);
      _CFG_write_strln(fIni,s);

      sprintf(s,"vfilter = %s",currentStatus->vfilter_bak);
      _CFG_write_strln(fIni,s);

      sprintf(s,"en_twi = %d",currentStatus->twi_enabled);
      _CFG_write_strln(fIni,s);

      sprintf(s,"en_spi = %d,%d",currentStatus->spi_fpga_enabled,
                                 currentStatus->spi_osd_enabled);
      _CFG_write_strln(fIni,s);

      if (currentStatus->spiclk_bak) {
        sprintf(s,"spi_clk = %ld",currentStatus->spiclk_bak);
        _CFG_write_strln(fIni,s);
      }

      sprintf(s,"button = %s",currentStatus->button==BUTTON_RESET?"reset": (
                              currentStatus->button==BUTTON_OFF?"off":"menu"));
      _CFG_write_strln(fIni,s);

      sprintf(s,"video = %s",currentStatus->video_bak);
      _CFG_write_strln(fIni,s);

      sprintf(s,"config = 0x%08lx,0x%08lx",currentStatus->config_s
                                      ,currentStatus->config_d);
      _CFG_write_strln(fIni,s);

      currentStatus->info_bak_last=currentStatus->info_bak;
      while (currentStatus->info_bak_last) {
        sprintf(s,"info = %s",currentStatus->info_bak_last->info_bak);
        _CFG_write_strln(fIni,s);
        currentStatus->info_bak_last=currentStatus->info_bak_last->next;
      }

      // UPLOAD section
      _CFG_write_strln(fIni,"");
      _CFG_write_strln(fIni,"[UPLOAD]");

      sprintf(s,"verify = 0");
      _CFG_write_strln(fIni,s);

      currentStatus->rom_bak_last=currentStatus->rom_bak;
      while (currentStatus->rom_bak_last) {
        sprintf(s,"rom = %s",currentStatus->rom_bak_last->rom_bak);
        _CFG_write_strln(fIni,s);
        currentStatus->rom_bak_last=currentStatus->rom_bak_last->next;
      }

      currentStatus->data_bak_last=currentStatus->data_bak;
      while (currentStatus->data_bak_last) {
        sprintf(s,"data = %s",currentStatus->data_bak_last->data_bak);
        _CFG_write_strln(fIni,s);
        currentStatus->data_bak_last=currentStatus->data_bak_last->next;
      }

      // MENU section
      _CFG_write_strln(fIni,"");
      if (currentStatus->menu_top) {
        menu_t *menu_act = currentStatus->menu_top;
        menuitem_t *item_act;
        itemoption_t *option_act;

        _CFG_write_strln(fIni,"[MENU]");
        while (menu_act && menu_act->item_list) {
          if (menu_act && (!MATCH(menu_act->menu_title,"Replay Menu"))) {
            sprintf(s,"title = \"%s\"",menu_act->menu_title);
            _CFG_write_strln(fIni,s);

            item_act = menu_act->item_list;
            while (item_act && item_act->option_list) {
              sprintf(s,"item = \"%s\",0x%08lx,%s",item_act->item_name,
                        item_act->conf_mask,
                        item_act->conf_dynamic?"dynamic":"static");
              _CFG_write_strln(fIni,s);

              option_act = item_act->option_list;
              while (option_act && option_act->option_name[0]) {
                sprintf(s,"option = \"%s\",0x%08lx%s",option_act->option_name,
                          option_act->conf_value,
                          option_act==item_act->selected_option?",default":"");
                _CFG_write_strln(fIni,s);

                option_act = option_act->next;
              }
              item_act = item_act->next;
            }
          }
          menu_act = menu_act->next;
        }
      }

      FF_Close(fIni);
    }
  }

}
