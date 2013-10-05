#include "hardware.h"
#include "config.h"
#include "osd.h"
#include "messaging.h"

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
      DEBUG(1,"OsdWrite %s ", s);

      if (invert)
        DEBUG(1,"<< ");
      else
        DEBUG(1,"   ");
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
    DEBUG(1,"OsdClear");
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
        WARNING("OSDWaitVBL timeout");
        break;
      }
    }
}

//--> Replaced by structure in message.* module
//void OSD_BootPrint(const char *pText)
//{
//  DEBUG(1,"BootPrint : %s",pText);
//  OSD_WriteScroll(15, 0, pText, 0, 0xF, 0x0, 1);
//}


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
  DEBUG(1,"ram config 0x%04X",config);

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

    static uint32_t button_delay;
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
        button_delay = Timer_Get(BUTTONDELAY);
      }
    } else if (Timer_Check(button_delay)) {
        button_pressed = 0;
    }

    if (x & STF_NEWKEY) {
        SPI_EnableOsd();
        SPI(OSDCMD_READKBD);
        x = SPI(0);
        SPI_DisableOsd();

        /*DEBUG(1,"new key %X",x);*/

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

    if (!key_code) {
      static uint8_t  esc_received = 0;
      static uint32_t esc_delay = 0;

      if (USART_Peekc()==0x1b) {
        if (!esc_received) {
          // lets take some time to check for further characters
          esc_received = 1;
          esc_delay = Timer_Get(SERIALDELAY);
        } else if (Timer_Check(esc_delay)) {
          // timeout, let's see what we got...
          const uint8_t UP_KEYSEQ[]    = {0x1b, 0x5b, 0x41};
          const uint8_t DOWN_KEYSEQ[]  = {0x1b, 0x5b, 0x42};
          const uint8_t RIGHT_KEYSEQ[] = {0x1b, 0x5b, 0x43};
          const uint8_t LEFT_KEYSEQ[]  = {0x1b, 0x5b, 0x44};
          //--
          const uint8_t POS1_KEYSEQ[]  = {0x1b, 0x5b, 0x31, 0x7e};
          const uint8_t INS_KEYSEQ[]   = {0x1b, 0x5b, 0x32, 0x7e};
          const uint8_t DEL_KEYSEQ[]   = {0x1b, 0x5b, 0x33, 0x7e};
          const uint8_t END_KEYSEQ[]   = {0x1b, 0x5b, 0x34, 0x7e};
          const uint8_t PGUP_KEYSEQ[]  = {0x1b, 0x5b, 0x35, 0x7e};
          const uint8_t PGDN_KEYSEQ[]  = {0x1b, 0x5b, 0x36, 0x7e};
          //--
          const uint8_t F1_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x31, 0x7e};
          const uint8_t F2_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x32, 0x7e};
          const uint8_t F3_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x33, 0x7e};
          const uint8_t F4_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x34, 0x7e};
          const uint8_t F5_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x35, 0x7e};
          const uint8_t F6_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x37, 0x7e};
          const uint8_t F7_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x38, 0x7e};
          const uint8_t F8_KEYSEQ[]    = {0x1b, 0x5b, 0x31, 0x39, 0x7e};
          const uint8_t F9_KEYSEQ[]    = {0x1b, 0x5b, 0x32, 0x30, 0x7e};
          const uint8_t F10_KEYSEQ[]   = {0x1b, 0x5b, 0x32, 0x31, 0x7e};
          const uint8_t F11_KEYSEQ[]   = {0x1b, 0x5b, 0x32, 0x33, 0x7e};
          const uint8_t F12_KEYSEQ[]   = {0x1b, 0x5b, 0x32, 0x34, 0x7e};

          esc_received = 0;

          if ((!key_code) && USART_GetBuf(LEFT_KEYSEQ,sizeof(LEFT_KEYSEQ))) key_code=KEY_LEFT;
          if ((!key_code) && USART_GetBuf(RIGHT_KEYSEQ,sizeof(RIGHT_KEYSEQ))) key_code=KEY_RIGHT;
          if ((!key_code) && USART_GetBuf(UP_KEYSEQ,sizeof(UP_KEYSEQ))) key_code=KEY_UP;
          if ((!key_code) && USART_GetBuf(DOWN_KEYSEQ,sizeof(DOWN_KEYSEQ))) key_code=KEY_DOWN;
          if ((!key_code) && USART_GetBuf(PGUP_KEYSEQ,sizeof(PGUP_KEYSEQ))) key_code=KEY_PGUP;
          if ((!key_code) && USART_GetBuf(PGDN_KEYSEQ,sizeof(PGDN_KEYSEQ))) key_code=KEY_PGDN;

          if ((!key_code) && USART_GetBuf(POS1_KEYSEQ,sizeof(POS1_KEYSEQ))) key_code=KEY_ESC; // ignored
          if ((!key_code) && USART_GetBuf(END_KEYSEQ,sizeof(END_KEYSEQ))) key_code=KEY_ESC; // ignored
          if ((!key_code) && USART_GetBuf(INS_KEYSEQ,sizeof(INS_KEYSEQ))) key_code=KEY_ESC; // ignored
          if ((!key_code) && USART_GetBuf(DEL_KEYSEQ,sizeof(DEL_KEYSEQ))) key_code=KEY_ESC; // ignored
          
          if ((!key_code) && USART_GetBuf(F1_KEYSEQ,sizeof(F1_KEYSEQ))) key_code=KEY_F1;
          if ((!key_code) && USART_GetBuf(F2_KEYSEQ,sizeof(F2_KEYSEQ))) key_code=KEY_F2;
          if ((!key_code) && USART_GetBuf(F3_KEYSEQ,sizeof(F3_KEYSEQ))) key_code=KEY_F3;
          if ((!key_code) && USART_GetBuf(F4_KEYSEQ,sizeof(F4_KEYSEQ))) key_code=KEY_F4;
          if ((!key_code) && USART_GetBuf(F5_KEYSEQ,sizeof(F5_KEYSEQ))) key_code=KEY_F5;
          if ((!key_code) && USART_GetBuf(F6_KEYSEQ,sizeof(F6_KEYSEQ))) key_code=KEY_F6;
          if ((!key_code) && USART_GetBuf(F7_KEYSEQ,sizeof(F7_KEYSEQ))) key_code=KEY_F7;
          if ((!key_code) && USART_GetBuf(F8_KEYSEQ,sizeof(F8_KEYSEQ))) key_code=KEY_F8;
          if ((!key_code) && USART_GetBuf(F9_KEYSEQ,sizeof(F9_KEYSEQ))) key_code=KEY_F9;
          if ((!key_code) && USART_GetBuf(F10_KEYSEQ,sizeof(F10_KEYSEQ))) key_code=KEY_F10;
          if ((!key_code) && USART_GetBuf(F11_KEYSEQ,sizeof(F11_KEYSEQ))) key_code=KEY_ESC; // ignored
          if ((!key_code) && USART_GetBuf(F12_KEYSEQ,sizeof(F12_KEYSEQ))) key_code=KEY_MENU;
          if (!key_code) {
            key_code=KEY_ESC;
            USART_Getc(); // take ESC key
            // Use the lines below temporarly to check not implemented sequences
            // (then also comment out above lines!)
            //while (USART_CharAvail()>0) {
            //  printf("%02x ",USART_Getc());
            //} 
            //printf("\n\r");
          }
        }
      } else {
        uint8_t c=USART_Getc();          // take (non-sequence) single char input
        if (c==0x0d) key_code=KEY_ENTER;
      }
    }

    return key_code;
}


