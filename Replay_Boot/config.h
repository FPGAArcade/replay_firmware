/** @file config.h */

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "fpga.h"
#include "osd.h"
#include "card.h"
#include "hardware.h"
#include "iniparser.h"
#include "twi.h"
#include "filesel.h"
#include "amiga_fdd.h"

/* ========================================================================== */

/** maximum length of a menu string to be used in the INI file,
    including limiter character - OSD width should be chosen  */
#define MAX_MENU_STRING (32+1)

/** maximum length of an item string to be used in the INI file
    including limiter character - usually half OSD width is fine  */
#define MAX_ITEM_STRING (16+1)

/** maximum length of an option string to be used in the INI file
    including limiter character - usually half OSD width is fine */
#define MAX_OPTION_STRING (16+1)

/* ========================================================================== */

/** @brief VALUE MATCHER

    Macro to simplify checking for a specific string in an other one.
*/
#define MATCH(v1,v2)  (stricmp(v1, v2) == 0)

/** @brief VALUE LENGTH MATCHER

    Macro to simplify checking for a specific string in an other one for
    given length of the second string (should be a static string).
*/
#define MATCHN(v1,v2)  (strnicmp(v1, v2, strlen(v2)) == 0)

/* ========================================================================== */

/** AD723 TV coder modes - TODO: where to put ? */
typedef enum {
  CODER_DISABLE,
  CODER_PAL,
  CODER_NTSC,
  CODER_PAL_NOTRAP,
  CODER_NTSC_NOTRAP
} coder_t;

typedef enum {
  F50HZ,
  F60HZ
} framerate_t;

/** Replay menu button modes - TODO: where to put ? */
typedef enum {
  BUTTON_OFF,
  BUTTON_MENU,
  BUTTON_RESET
} button_t;


/* ========================================================================== */

/** @brief Basic menu item options structure

    This double-linked structure contains an option string and its related
    value.
*/
typedef struct itemoption {

  /** link to previous option entry */
  struct itemoption *last;

  /** link to next option entry */
  struct itemoption *next;

  /** option name */
  char              option_name[MAX_OPTION_STRING];

  /** option value */
  uint32_t          conf_value;

} itemoption_t;

/** @brief Basic menu item structure

    This double-linked structure contains an item, its mask, the update
    behavior and a link to the list of options.
*/
typedef struct menuitem {

  /** link to previous item entry */
  struct menuitem   *last;

  /** link to next item entry */
  struct menuitem   *next;

  /** item name */
  char              item_name[MAX_ITEM_STRING];

  /** link to options list for this item */
  itemoption_t      *option_list;

  /** link to default option for options */
  itemoption_t      *selected_option;

  /** binary mask for this item */
  uint32_t          conf_mask;

  /** select if dynamic or static item */
  uint8_t           conf_dynamic;

  /** action of item (will be processed by the action handler) */
  char              action_name[MAX_ITEM_STRING];

  /** action value of item (will be processed by the action handler) */
  uint32_t          action_value;

} menuitem_t;

/** @brief Menu list

    This double-linked structure for several menu sheets.
*/
typedef struct menu {

  /** link to previous menu entry */
  struct menu       *last;

  /** link to next menu entry */
  struct menu       *next;

  /** title of this menu entry */
  char              menu_title[MAX_MENU_STRING];

  /** link to item list for this menu */
  menuitem_t        *item_list;

} menu_t;

/* ========================================================================== */

/** @brief ROM download backup structure
*/
typedef struct rom_list {
  /** link to next menu entry */
  struct rom_list   *next;

  /** value of this menu entry */
  char              rom_bak[MAX_LINE_LEN];

} rom_list_t;

/** @brief DATA download backup structure
*/
typedef struct data_list {
  /** link to next menu entry */
  struct data_list  *next;

  /** value of this menu entry */
  char              data_bak[MAX_LINE_LEN];

} data_list_t;

/** @brief INFO download backup structure
*/
typedef struct info_list {
  /** link to next menu entry */
  struct info_list  *next;

  /** value of this menu entry */
  char              info_bak[MAX_LINE_LEN];

} info_list_t;

/* ========================================================================== */

/** @brief Basic replay status structure

    This structure contains the configuration and HW state
    of on-board components (TV coder, SD-CARD), some general
    settings for INI file handling and FPGA configuration items.
    Furthermore it configures the on-board button behaviour.
*/
typedef struct {

  /* ======== input pins ========== */

  /** set to 1 if AD723 is fitted on board - updated by update_status() */
  uint8_t      coder_fitted;

  /** set to 1 if SD-card is in the slot  - updated by update_status() */
  uint8_t      card_detected;

  /** set to 1 if SD-card is write protected - updated by update_status() */
  uint8_t      card_write_protect;

  /* ======== replay state ========== */

  /** set to 1 if SD-card is successfully initialized - set by card_start() */
  uint8_t      card_init_ok;

  /** set to 1 if SD-card is successfully mounted - set by card_start()*/
  uint8_t      fs_mounted_ok;

  /** set to 1 if FPGA got successfully configured - set in main() loop */
  uint8_t      fpga_load_ok;

  /* ======== general INI environment ======== */

  /** current directory used for INI/CNF/BIN and starting point for browsing */
  char         ini_dir[FF_MAX_PATH];

  /** local initialization filename "*.INI" */
  char         ini_file[FF_MAX_FILENAME];

  /** local configuration filename "*.CNF" - implementation TODO */
  char         conf_file[FF_MAX_FILENAME];

  /* ======== FPGA initialization ======== */

  /** local FPGA configuration filename "*.BIN", file may be compressed */
  char         bin_file[FF_MAX_FILENAME];

  /** set to 1 if TWI is routed through FPGA - set by ini_pre_read() */
  uint8_t      twi_enabled;

  /** set to 1 if SPI for OSD is available on FPGA - set by ini_pre_read() */
  uint8_t      spi_osd_enabled;

  /** set to 1 if SPI for config is available on FPGA - set by ini_pre_read() */
  uint8_t      spi_fpga_enabled;

  /** set to 1 if ROM/DATA uploads shall be verified - set by ini_post_read() */
  uint8_t      verify_dl;

  /** next address where to load a ROM content - set by ini_post_read() */
  uint32_t     last_rom_adr;

  /** set DRAM phase config - set by ini_post_read() */
  int8_t      dram_phase;

  /* ======== CONFIGURATION ======== */

  /** "static" core configuration, set on (re-)initialisation only */
  uint32_t     config_s;

  /** dynamic core configuration, set "on the fly" when changed in menu */
  uint32_t     config_d;

  /* ======== MENU handling ======== */

  /** defines the on-board button function - set by ini_pre_read() */
  button_t     button;

  /** set to 1 if menu is visible - set by handle_ui() */
  uint8_t      show_menu;

  /** set to 1 if file browsing is active - set by handle_ui() */
  uint8_t      file_browser;

  /** set to 1 if status screen is visible - set by handle_ui() */
  uint8_t      show_status;

  /** set to 1 if warning pop-up is visible - set by handle_ui() */
  uint8_t      popup_menu;

  /** set to 1 if update is pending and must be processed by handle_ui() */
  uint8_t      update;

  /** set to a line number if scrolling is required within handle_ui() */
  uint8_t      scroll_pos;

  /** set to 1 if pop-up is requested for reboot (otherwise only FPGA reset) */
  uint8_t      do_reboot;

  /* ======== INI menu entry stuff ======== */

  /** ink to top of menu tree (fixed, does not vary) */
  menu_t       *menu_top;

  /** link to a double-linked list of menus for ini_post_read(), handle_ui() */
  menu_t       *menu_act;

  /** link to a double-linked list of items ini_post_read(), handle_ui() */
  menuitem_t   *menu_item_act;

  /** link to a double-linked list of options ini_post_read(), handle_ui() */
  itemoption_t *item_opt_act;

  /* ======== OSD menu stuff ======== */

  /** link to first menu entry shown on OSD - set by ini_post_read() */
  menu_t       *menu_first;

  /** link to last menu entry shown on OSD - set by ini_post_read() */
  menu_t       *menu_last;

  /** link to first item entry shown on OSD - set by ini_post_read() */
  menuitem_t   *item_first;

  /** link to last item entry shown on OSD - set by ini_post_read() */
  menuitem_t   *item_last;

  /* ======== warning pop-up handling ======== */

  /** pop-up OSD "warning" message */
  char         popup_msg[MAX_MENU_STRING];

  /** pop-up OSD selection type: "yes/no", ... */
  uint8_t      selections;

  /** which pop-up choice is selected (1/0 => "yes/no") - set by handle_ui() */
  uint8_t      selected;

  /* ======== status page ======== */

  /** 3 status lines always shown on replay OSD */
  char         status[3][MAX_MENU_STRING];

  /** latest 8 info lines shown on replay OSD */
  char         info[8][MAX_MENU_STRING];

  /** instead of scrolling in array, we scroll moving the start index
       of the info array*/
  uint8_t      info_start_idx;

  /* ======== file browser stuff menu ======== */

  /** structure handling directory browsing */
  tDirScan     *dir_scan;

  /** the actual browser directory path */
  char         act_dir[FF_MAX_PATH];

  /* ======== used for config backup only ======== */

  /** clock line backup */
  char         clock_bak[MAX_LINE_LEN];
  /** coder line backup */
  char         coder_bak[MAX_LINE_LEN];
  /** vfilter line backup */
  char         vfilter_bak[MAX_LINE_LEN];
  /** video line backup */
  char         video_bak[MAX_LINE_LEN];
  /** rom lines backup */
  rom_list_t   *rom_bak;
  /** rom lines backup position */
  rom_list_t   *rom_bak_last;
  /** data lines backup */
  data_list_t  *data_bak;
  /** data lines backup position*/
  data_list_t  *data_bak_last;
  /** info lines backup */
  info_list_t  *info_bak;
  /** info lines backup position*/
  info_list_t  *info_bak_last;
  /** SPI clock backup */
  int32_t      spiclk_bak;
  /** SPI clock last value */
  int32_t      spiclk_old;
} status_t;

/* ========================================================================== */
/** @brief BLINK ERROR STATUS ON LED

    Blink the given count, make a pause and blink again (5x).

    @param error blink count to signal error
*/
void CFG_fatal_error(uint8_t error);

/* ========================================================================== */
/** @brief HDD/FDD HANDLER FOR FPGA

    Polls SPI status and updates FDD/HDD handling stuff.
*/
void CFG_handle_fpga(void);

/* ========================================================================== */
/** @brief INIT ARM BOOTLOADER

    Copies bootloader from FLASH to RAM and starts it.
    Will never return directly, thus it also sets the FPGA
    back in the setup mode to ensure a reload after the reset.
*/
void CFG_call_bootloader(void);

/* ========================================================================== */

/** @brief UPDATE REPLAY FLAGS

    Updates HW flags on replay board in status structure.
    Checks if a composite decoder is available on the board and if
    a SD-card is inserted and/or write protected.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_update_status(status_t *currentStatus);

/** @brief DEBUG-PRINT REPLAY FLAGS

    Shows setting of HW flags on debug channel.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_print_status(status_t *currentStatus);

/* ========================================================================== */

/** @brief SET UP INSERTED SD CARD

    Tries to initialise and mount a SD-card.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_card_start(status_t *currentStatus);

/* ========================================================================== */

/** @brief ROM DATA UPLOADER

    Takes a file and sends it to the FPGA as ROM data.

    @param filename (absolute) filename to the binary file
    @param base base address where to store the data on the FPGA
    @param size size of the datafile (when 0, do auto-sizing)
    @param verify if set to 1, verify uploaded content again

    @return 0 when transmission was successful, others indicate a failure
*/
uint8_t CFG_upload_rom(char *filename, uint32_t base,
                       uint32_t size, uint8_t verify);

/* ========================================================================== */

/** @brief CONFIGURE (OPTIONAL) AD723 TV CODER

    Controls some HW lines to the coder to configure it for the used
    TV standard.

    @param standard TV standard to set
*/
void CFG_set_coder(coder_t standard);

/** @brief CONFIGURE FPGA FROM FILE

    Takes a "BIN" file and sends it to the FPGA to configure it.

    @param filename (absolute) filename to the FPGA configuration file
    @return 0 when FPGA got configured, others indicate a failure
*/
uint8_t CFG_configure_fpga(char *filename);

/** @brief DETERMINE FREE RAM

    Determines the space between top-of-stack and bottom-of-heap by allocating
    1kByte memory.

    @return free memory in bytes
*/

void CFG_vid_timing_SD(framerate_t standard);
void CFG_vid_timing_HD27(framerate_t standard);
void CFG_vid_timing_HD74(framerate_t standard);
void CFG_set_CH7301_SD(void);
void CFG_set_CH7301_HD(void);

uint32_t CFG_get_free_mem(void);

/** @brief INITIAL INI READER PARSE HANDLER

    Called by INI parser to execute file entries.

    @param status pointer to the replay board status dataset
    @param section section token (INI_START for SOF or INI_UNKNOWN for EOF)
    @param name name token (INI_UNKNOWN for change of section)
    @param value value of given name entry

    @return 0 on success, others indicate a failure
*/
uint8_t _CFG_pre_parse_handler(void *status, const ini_symbols_t section,
                               const ini_symbols_t name, const char *value);

/** @brief INITIAL INI READER

    Initial INI parse to set up system clocking and some TWI components,
    as well as determining the FPGA configuration file to use and
    what functions the FPGA can be used for later configurations.

    @param currentStatus pointer to the replay board status dataset
    @param iniFile (absolute) filename of the INI file to parse
    @return 0 on success, others indicate a failure
*/
uint8_t CFG_pre_init(status_t *currentStatus, const char *iniFile);

/** @brief FINAL INI READER PARSE HANDLER

    Called by INI parser to execute file entries.

    @param status pointer to the replay board status dataset
    @param section section token (INI_START for SOF or INI_UNKNOWN for EOF)
    @param name name token (INI_UNKNOWN for change of section)
    @param value value of given name entry
    @return 0 on success, others indicate a failure
*/
uint8_t _CFG_parse_handler(void *status, const ini_symbols_t section,
                           const ini_symbols_t name, const char *value);

/** @brief FINAL INI READER

    Final INI parse to set up components requiring functionality of the
    FPGA.
    Furthermore it set up additional ROM filelists etc. to send to the FPGA.

    TODO: config details

    @param currentStatus pointer to the replay board status dataset
    @param iniFile (absolute) filename of the INI file to parse
    @return 0 on success, others indicate a failure
*/
uint8_t CFG_init(status_t *currentStatus, const char *iniFile);

/** @brief ADD DEFAULT MENU

    Adds some common menu entries which should be always available.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_add_default(status_t *currentStatus);

/** @brief FREE DYNAMIC MENU MEMORY

    Free memory allocated for menu system.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_free_menu(status_t *currentStatus);

/** @brief FREE BACKUP MEMORY

    Free memory allocated for baclup.

    @param currentStatus pointer to the replay board status dataset
*/
void CFG_free_bak(status_t *currentStatus);

/** @brief INI SAVER

    Saves actual settings to an INI file.

    @param currentStatus pointer to the replay board status dataset
    @param iniDir directory where to put the INI file
    @param iniFile filename in which the INI settings are written
    @return 0 on success, others indicate a failure
*/
void CFG_save_all(status_t *currentStatus, const char *iniDir, 
                  const char *iniFile);

#endif
