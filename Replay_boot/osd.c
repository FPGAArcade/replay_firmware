#include "hardware.h"
#include "config.h"
#include "osd.h"

// nasty globals ...
uint8_t osd_vscroll = 0;
uint8_t osd_page = 0;

// conversion table of ps/2 scan codes to ASCII codes

const char keycode_table[128] =
{
      0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0, 'Q','1', 0,   0,  0, 'Z','S','A','W','2', 0,
      0, 'C','X','D','E','4','3', 0,   0, ' ','V','F','T','R','5', 0,
      0, 'N','B','H','G','Y','6', 0,   0,  0, 'M','J','U','7','8', 0,
      0, ',','K','I','O','0','9', 0,  '.', 0,  0, 'L', 0, 'P', 0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,   0, '1', 0, '4','7', 0,  0,  0,
     '0','.','2','5','6','8', 0,  0,   0,  0, '3', 0, '9', 0,  0,  0
};

// a ? e1:e2 e1 a/=0, e2 a==0
void OSD_Write(uint8_t row, const char *s, uint8_t invert)
{
  // clears until end of line
  OSD_WriteBase(row, 0 , s, invert, 0xF, 0, 1);
}

void OSD_WriteRC(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col )
{
  //
  OSD_WriteBase(row, col, s, invert, fg_col, bg_col, 0);
}

// write a null-terminated string <s> to the OSD buffer starting at line row, col
void OSD_WriteBase(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col, uint8_t clear )
{
    uint16_t i;
    uint8_t b;
    uint8_t attrib = (fg_col & 0xF) | ((bg_col & 0xF) << 4);

    if (invert)
      attrib = (fg_col & 0xF) | (0x4 << 4);
    #ifdef OSD_DEBUG
      printf("OsdWrite %s ", s);

      if (invert)
        printf("<< \r\n");
      else
        printf("   \r\n");
    #endif

    // select OSD SPI device
    SPI_EnableOsd();
    SPI(OSDCMD_WRITE | (row & 0x3F));
    SPI(col + osd_page * OSDLINELEN);

    i = 0;
    // send all characters in string to OSD
    while (1)
    {
        b = *s++;

        if (b == 0) // end of string
            break;

        else if (b == 0x0D || b == 0x0A) { // cariage return / linefeed, go to next line
          // increment line counter
          if (++row >= OSDNLINE)
              row = 0;
          SPI_DisableOsd();
          // send new line number to OSD
          SPI_EnableOsd();
          SPI(OSDCMD_WRITE | (row & 0x3F)); // col 0
          SPI(col + osd_page * OSDLINELEN);
        }
        else { // normal character
          SPI(b);
          SPI(attrib); // attrib
          i++;
        }
    }

    if (clear) {
      for (; i < OSDLINELEN; i++) { // clear end of line
         /*SPI(0x20 | (invert ? 0x80 : 0) );*/
         SPI(0x20);
         SPI(attrib);
      }
    }
    // deselect OSD SPI device
    SPI_DisableOsd();
}


void OSD_SetHOffset(uint8_t row, uint8_t col, uint8_t pix)
{
  SPI_EnableOsd();
  SPI(OSDCMD_WRITE | (row & 0x3F));
  // col not set for address
  SPI_DisableOsd();

  SPI_EnableOsd();
  SPI(OSDCMD_SETHOFF);
  SPI( ((col & 0x3F) << 2) | ((pix & 0xC)>>2) );
  SPI(pix & 0x3);
  SPI_DisableOsd();
}

void OSD_SetVOffset(uint8_t row)
{
  SPI_EnableOsd();
  SPI(OSDCMD_SETVOFF);
  SPI(row);
  SPI_DisableOsd();
}

uint8_t OSD_NextPage(void)
{
  return (osd_page + 1) % 2;
};

uint8_t OSD_GetPage(void)
{
  return osd_page;
};

void OSD_SetPage(uint8_t page)
{
  osd_page = page;
}

void OSD_SetDisplay(uint8_t page)
{
  uint8_t row = 0;
  OSD_WaitVBL();
  for (row = 0; row < OSDNLINE; row ++) {
    OSD_SetHOffset(row, page*OSDLINELEN, 0);
  }
}

void OSD_WriteScroll(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col, uint8_t clear )
{
  // do scroll
  osd_vscroll++;
  OSD_WaitVBL();
  OSD_SetVOffset(osd_vscroll);
  OSD_WriteBase( (row + osd_vscroll) & 0xF,col,s,invert,fg_col,bg_col,clear);
}

// clear OSD frame buffer
void OSD_Clear(void)
{
  uint8_t  row;
  uint16_t n;

  #ifdef OSD_DEBUG
    printf("OsdClear\r\n");
  #endif

  for (row = 0; row <OSDNLINE; row++) {
    SPI_EnableOsd();
    SPI(OSDCMD_WRITE | (row & 0x3F));
    SPI(osd_page * OSDLINELEN);

    // clear buffer
    for (n = 0; n <OSDLINELEN; n++) {
      SPI(0x20);
      SPI(0x0F);
    }
    SPI_DisableOsd();
  }

  OSD_SetVOffset(0);
  osd_vscroll = 0;
}

void OSD_WaitVBL(void)
{
    uint32_t pioa_old = 0;
    uint32_t pioa = 0;
    uint32_t timeout = Timer_Get(100);

    while ((~pioa ^ pioa_old) & PIN_CONF_DOUT) {
      pioa_old = pioa;
      pioa     = AT91C_BASE_PIOA->PIO_PDSR;
      if (Timer_Check(timeout)) {
        printf("OSDWaitVBL timeout\r\n");
        break;
      }
    }
}

void OSD_BootPrint(const char *pText)
{
  printf("BootPrint : %s\n\r",pText);
  OSD_WriteScroll(15, 0, pText, 0, 0xF, 0x0, 1);
}


// enable displaying of OSD
void OSD_Enable(unsigned char mode)
{
  SPI_EnableOsd();
  SPI(OSDCMD_ENABLE | (mode & 0xF));
  SPI_DisableOsd();
}

// disable displaying of OSD
void OSD_Disable(void)
{
  SPI_EnableOsd();
  SPI(OSDCMD_DISABLE);
  SPI_DisableOsd();
}

void OSD_Reset(unsigned char option)
{
  // soft reset or halt
  SPI_EnableOsd();
  SPI(OSDCMD_CTRL | option);
  SPI_DisableOsd();
}

void OSD_ConfigSendUserD(uint32_t configD)
{
  // Dynamic config
  SPI_EnableOsd();
  SPI(OSDCMD_CONFIG | 0x01); // dynamic
  SPI((uint8_t)(configD));
  SPI((uint8_t)(configD >> 8));
  SPI((uint8_t)(configD >> 16));
  SPI((uint8_t)(configD >> 24));
  SPI_DisableOsd();
}

void OSD_ConfigSendUserS(uint32_t configS)
{
  // Static config
  SPI_EnableOsd();
  SPI(OSDCMD_CONFIG | 0x00); // static
  SPI((uint8_t)(configS));
  SPI((uint8_t)(configS >> 8));
  SPI((uint8_t)(configS >> 16));
  SPI((uint8_t)(configS >> 24));
  SPI_DisableOsd();
}

void OSD_ConfigSendFileIO(uint32_t config)
{
  SPI_EnableOsd();
  SPI(OSDCMD_CONFIG | 0x02); // gileio
  SPI((uint8_t)(config));
  SPI_DisableOsd();
}

void OSD_ConfigSendCtrl(uint32_t config)
{
  printf("ram config 0x%04X\r\n",config);

  SPI_EnableOsd();
  SPI(OSDCMD_CONFIG | 0x03); // ctrl
  SPI((uint8_t)(config));
  SPI((uint8_t)(config >> 8));
  SPI_DisableOsd();
}

uint8_t OSD_ConfigReadSysconVer(void)
{
  uint8_t config;

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x01);
  config = SPI(0);
  SPI_DisableOsd();
  return config;
}

uint32_t OSD_ConfigReadVer(void)
{
  uint32_t config;

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x03); // high word
  config  = (SPI(0) & 0xFF) << 8;
  SPI_DisableOsd();

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x02); // low word
  config |= (SPI(0) & 0xFF);
  SPI_DisableOsd();

  return config;
}

uint32_t OSD_ConfigReadStatus(void)
{
  uint32_t config;

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x07); // high word
  config  = (SPI(0) & 0xFF) << 8;
  SPI_DisableOsd();

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x06); // low word
  config |= (SPI(0) & 0xFF);
  SPI_DisableOsd();

  return config;
}

uint32_t OSD_ConfigReadFileIO(void) // num disks etc
{
  uint32_t config;

  SPI_EnableOsd();
  SPI(OSDCMD_READSTAT | 0x04);
  config  = (SPI(0) & 0xFF);
  SPI_DisableOsd();
  return config;
}

uint8_t OSD_ConvASCII(uint16_t keycode)
{
    if (keycode & 0x180)
        return 0;
    // note, this converts from ps2 keycode to ASCII
    return keycode_table[keycode & 0x7F];
}

uint16_t OSD_GetKeyCode(void)
{
    static uint16_t key_flags = 0;
    static uint16_t old_key_code = 0;
    uint16_t key_code = 0;
    uint8_t  x;

    static uint32_t delay;
    static uint8_t  button_pressed = 0;

    SPI_EnableOsd();
    SPI(OSDCMD_READSTAT);
    x = SPI(0);
    SPI_DisableOsd();

    // add front menu button
    if (IO_Input_L(PIN_MENU_BUTTON)) {
      if (!button_pressed) {
        key_code = KEY_MENU;
        button_pressed = 1;
        delay = Timer_Get(BUTTONDELAY);
      }
    } else if (Timer_Check(delay)) {
        button_pressed = 0;
    }

    if (x & STF_NEWKEY) {
        SPI_EnableOsd();
        SPI(OSDCMD_READKBD);
        x = SPI(0);
        SPI_DisableOsd();

        /*printf("new key %X\r\n",x);*/

        if (x == ATKB_RELEASE)
            key_flags |= KF_RELEASED;
        else if ((x & 0xFE) == ATKB_EXTEND) // extended codes are preceeded by 0xE0 or 0xE1
            key_flags |= KF_EXTENDED;
        else
        {
             key_code = key_flags | x;
             if (key_code == old_key_code)
                key_code |= KF_REPEATED;
             else
                 old_key_code = key_code;
             key_flags = 0;
        }
    }

    x=USART_Getc();
    if (x) {
      if (x==0x0d) key_code=KEY_ENTER;
      if (x=='1')  key_code=KEY_F1;   // NOT good - TODO: better mapping

      if (x=='4')  key_code=KEY_F4;
      if (x=='5')  key_code=KEY_F5;
      if (x=='6')  key_code=KEY_F6;
      if (x=='7')  key_code=KEY_F7;
      if (x=='8')  key_code=KEY_F8;
      if (x=='9')  key_code=KEY_F9;

      if (x==0x1b) {
        Timer_Wait(1); // NOT good - TODO: do not swallow chars of sequence
        x=USART_Getc();
        if (x==0)    key_code=KEY_ESC;
        if (x==0x41) key_code=KEY_UP;
        if (x==0x42) key_code=KEY_DOWN;
        if (x==0x43) key_code=KEY_RIGHT;
        if (x==0x44) key_code=KEY_LEFT;

        if (x==0x7e) key_code=KEY_MENU; // NOT good - TODO: better mapping
      }
    }
/*
    static uint8_t  tty_esc = 0;
    static uint32_t tty_timeout = 0;
    static int      tty_idx = 0;
    static uint8_t  tty_buf[5] = {0,0,0,0,0};

    if (tty_esc) {
      if (x) {
        // got another character, extend timeout for another ms
        tty_buf[tty_idx++]=x;
        tty_timeout=Timer_Get(1);
      }
      // we have a max. 5 char sequence or we got a timeout
      if (Timer_Check(tty_timeout) || (tty_idx==5)) {
        for(int i=0;i<tty_idx;i++) printf("%x ",tty_buf[i]);
        printf(" %d\r\n",tty_idx);
        if (tty_idx==0) key_code=KEY_ESC;
        if ((tty_idx==2) && (tty_buf[0]==0x5b)) {
          if (tty_buf[1]==0x41) key_code=KEY_UP;
          if (tty_buf[1]==0x42) key_code=KEY_DOWN;
          if (tty_buf[1]==0x43) key_code=KEY_RIGHT;
          if (tty_buf[1]==0x44) key_code=KEY_LEFT;
        }
        if ((tty_idx==4) && (tty_buf[0]==0x5b)) { // ignore the middle ones...
          if (tty_buf[3]==0x7e) key_code=KEY_MENU; // NOT good - TODO: better mapping
        }
        tty_idx=0;
        tty_esc=0;
      }
    }
    else {
      if (x==0x0d) key_code=KEY_ENTER;
      if (x=='1')  key_code=KEY_F1;   // NOT good - TODO: better mapping
      if (x==0x1b) {
        // got an ESC, remember and set timeout to 1 ms
        tty_esc=1;
        tty_timeout=Timer_Get(1);
      }
    }
*/

    return key_code;
}


