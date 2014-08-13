//
#include "card.h"
#include "hardware.h"
#include "messaging.h"

/*variables*/
uint8_t  crc;
uint32_t timeout;
uint8_t  response;
uint8_t  cardType;

// internal functions
uint8_t MMC_Command(uint8_t cmd, uint32_t arg);
uint8_t MMC_Command12(void);
void    MMC_CRC(uint8_t c);

uint8_t Card_Init(void)
{
  uint8_t n;
  uint8_t ocr[4];

  AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (120<<8) | (2<<24); //init clock 100-400 kHz
  SPI_DisableCard();

  for(n=0; n < 10; n++)                    /*Set SDcard in SPI-Mode, Reset*/
      SPI(0xFF);                           /*10 * 8bits = 80 clockpulses*/

  Timer_Wait(20);           // 20ms delay
  SPI_EnableCard();

  cardType = CARDTYPE_NONE;

  if (MMC_Command(CMD0,0) == 0x01) {       /*CMD0: Reset all cards to IDLE state*/
    timeout = Timer_Get(1000);      // 1000 ms timeout

    if (MMC_Command(CMD8, 0x1AA) == 0x01) { // check if the card can operate with 2.7-3.6V power
      // SDHC card
      for (n = 0; n < 4; n++)
        ocr[n] = SPI(0xFF); // get the rest of R7 response

      if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
      // the card can work at 2.7-3.6V
        DEBUG(1,"SPI:Card_Init SDHC card detected");
        while (!Timer_Check(timeout)) {
          // now we must wait until CMD41 returns 0 (or timeout elapses)
          if (MMC_Command(CMD55, 0) == 0x01) {
            // CMD55 must precede any ACMD command
            if (MMC_Command(CMD41, 1 << 30) == 0x00) { // ACMD41 with HCS bit
              // initialization completed
              if (MMC_Command(CMD58, 0) == 0x00) {
                // check CCS (Card Capacity Status) bit in the OCR
                for (n = 0; n < 4; n++)
                  ocr[n] = SPI(0xFF);
                // if CCS set then the card is SDHC compatible
                cardType = (ocr[0] & 0x40) ? CARDTYPE_SDHC : CARDTYPE_SD;
              }
              else {
                WARNING("SPI:Card_Init CMD58 (READ_OCR) failed!");
              }
              SPI_DisableCard();
              AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (2 << 8); // 24 MHz SPI clock (max 25 MHz for SDHC card)
              return(cardType);
            }
          }
          else {
            ERROR("SPI:Card_Init CMD55 (APP_CMD) failed!");
            SPI_DisableCard();
            return(CARDTYPE_NONE);
          }
        } // end of timer loop
        ERROR("SPI:Card_Init SDHC card initialization timed out!");
        SPI_DisableCard();
        return(CARDTYPE_NONE);
      }
    } // end of CMD8

    // it's not an SDHC card
    if (MMC_Command(CMD55, 0) == 0x01) {
      // CMD55 accepted so it's an SD card (or Kingston 128 MB MMC)
      if (MMC_Command(CMD41, 0) <= 0x01) {
        // SD card detected - wait for the end of initialization
        DEBUG(1,"SPI:Card_Init SD card detected");
        while (!Timer_Check(timeout)) {
          // now we must wait until CMD41 returns 0 (or timeout elapses)
          if (MMC_Command(CMD55, 0) == 0x01) {
            // CMD55 must precede any ACMD command

            if (MMC_Command(CMD41, 0) == 0x00) {
              // initialization completed
              if (MMC_Command(CMD16, 512) != 0x00) { //set block length
                WARNING("SPI:Card_Init CMD16 (SET_BLOCKLEN) failed!");
              }

              SPI_DisableCard();

              AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (2 << 8); // 24 MHz SPI clock (max 25 MHz for SD card)
              cardType = CARDTYPE_SD;
              return(cardType);
            }
          } else {
            ERROR("SPI:Card_Init CMD55 (APP_CMD) failed!");
            SPI_DisableCard();
            return(CARDTYPE_NONE);
          }
        } // end of timer loop
        ERROR("SPI:Card_Init SD card initialization timed out!");
        SPI_DisableCard();
        return(CARDTYPE_NONE);
      }
    }

    // it's not an SD card
    DEBUG(1,"SPI:Card_Init MMC card detected");
    while (!Timer_Check(timeout)) {
      // now we must wait until CMD1 returns 0 (or timeout elapses)
      if (MMC_Command(CMD1, 0) == 0x00) {
        // initialization completed

        if (MMC_Command(CMD16, 512) != 0x00) { // set block length
          WARNING("SPI:Card_Init CMD16 (SET_BLOCKLEN) failed!");
        }

        SPI_DisableCard();
        AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (3 << 8); // 16 MHz SPI clock (max 20 MHz for MMC card)
        cardType = CARDTYPE_MMC;
        return(cardType);
      }
    }
    ERROR("SPI:Card_Init MMC card initialization timed out!");
    SPI_DisableCard();
    return(CARDTYPE_NONE);
  }

  SPI_DisableCard();
  ERROR("SPI:Card_Init No memory card detected!");
  return(CARDTYPE_NONE);
}


FF_T_SINT32 Card_ReadM(FF_T_UINT8 *pBuffer, FF_T_UINT32 sector, FF_T_UINT32 numSectors, void *pParam)
{
  // if pReadBuffer is NULL then use direct to the FPGA transfer mode (FPGA2 asserted)
  // how to handle direct to FPGA mode?

  uint32_t sectorCount = numSectors;

  DEBUG(3,"SPI:Card_ReadM sector %lu %lu",sector, numSectors);
  SPI_EnableCard();

  if (cardType != CARDTYPE_SDHC) // SDHC cards are addressed in sectors not bytes
    sector = sector << 9;   // calculate byte address

  if (numSectors == 1) {
    // single sector
    if (MMC_Command(CMD17, sector)) {
      WARNING("SPI:Card_ReadM CMD17 - invalid response 0x%02X (lba=%lu)", response, sector);
      SPI_DisableCard();
      return(FF_ERR_DEVICE_DRIVER_FAILED);
    }
  } else {
    // multiple sector
    if (MMC_Command(CMD18, sector)) {
      WARNING("SPI:Card_ReadM CMD18 - invalid response 0x%02X (lba=%lu)", response, sector);
      SPI_DisableCard();
      return(FF_ERR_DEVICE_DRIVER_FAILED);
    }
  }

  while (sectorCount--) {

    timeout = Timer_Get(250);      // timeout
    while (SPI(0xFF) != 0xFE) {
      if (Timer_Check(timeout)) {
        WARNING("SPI:Card_ReadM - no data token! (lba=%lu)", sector);
        SPI_DisableCard();
        return(FF_ERR_DEVICE_DRIVER_FAILED);
      }
    }
    // set override
    AT91C_BASE_PIOA->PIO_SODR = PIN_CARD_MOSI;  // set GPIO output register
    AT91C_BASE_PIOA->PIO_OER  = PIN_CARD_MOSI;  // GPIO pin as output
    AT91C_BASE_PIOA->PIO_PER  = PIN_CARD_MOSI;  // enable GPIO function

    // use SPI PDC (DMA transfer)
    AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_TCR  = 512;
    AT91C_BASE_SPI->SPI_TNCR = 0;
    AT91C_BASE_SPI->SPI_RPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_RCR  = 512;
    AT91C_BASE_SPI->SPI_RNCR = 0;
    AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN; // start DMA transfer

    // wait for tranfer end
    timeout = Timer_Get(100);      // 100 ms timeout
    while ( (AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX)) != (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX) ) {
      if (Timer_Check(timeout)) {
        WARNING("SPI:Card_ReadM DMA Timeout! (lba=%lu)", sector);
        SPI_DisableCard();
        return(FF_ERR_DEVICE_DRIVER_FAILED);
      }
    };

    // disable
    AT91C_BASE_SPI ->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS; // disable transmitter and receiver*/
    AT91C_BASE_PIOA->PIO_PDR  = PIN_CARD_MOSI; // disable GPIO function*/

    pBuffer += 512; // point to next sector

    SPI(0xFF); // read CRC lo byte
    SPI(0xFF); // read CRC hi byte
    // ? check CRC
  }

  if (numSectors != 1) {
    MMC_Command12(); // stop multi block transmission
  }
  SPI_DisableCard();
  return (FF_ERR_NONE);
}

FF_T_SINT32 Card_WriteM(FF_T_UINT8 *pBuffer, FF_T_UINT32 sector, FF_T_UINT32 numSectors, void *pParam)
{
  DEBUG(3,"SPI:Card_WriteM sector %lu num %lu",sector, numSectors);

  uint32_t sectorCount = numSectors;
  uint32_t i;
  SPI_EnableCard();

  // to do : optimise for multi-block write

  if (cardType != CARDTYPE_SDHC) // SDHC cards are addressed in sectors not bytes
    sector = sector << 9;   // calculate byte address

  while (sectorCount--) {
    if (MMC_Command(CMD24, sector)) {
      WARNING("SPI:Card_WriteM CMD24 - invalid response 0x%02X (lba=%lu)", response, sector);
      SPI_DisableCard();
      return(FF_ERR_DEVICE_DRIVER_FAILED);
    }

    SPI(0xFF); // one byte gap
    SPI(0xFE); // send Data Token

    // send sector bytes TO DO -- DMA
    for (i = 0; i < 512; i++)
      SPI(*(pBuffer++));

    // calc CRC????
    SPI(0xFF); // send CRC lo byte
    SPI(0xFF); // send CRC hi byte

    response = SPI(0xFF); // read packet response
    // Status codes
    // 010 = Data accepted
    // 101 = Data rejected due to CRC error
    // 110 = Data rejected due to write error
    response &= 0x1F;
    if (response != 0x05)
    {
      WARNING("SPI:Card_WriteM - invalid status 0x%02X (lba=%lu)", response, sector);
      SPI_DisableCard();
      return(FF_ERR_DEVICE_DRIVER_FAILED);
    }

    timeout = Timer_Get(500);      // timeout
    while (SPI(0xFF) == 0x00) {
      if (Timer_Check(timeout)) {
        WARNING("SPI:Card_WriteM - busy write timeout! (lba=%lu)", sector);
        SPI_DisableCard();
        return(FF_ERR_DEVICE_DRIVER_FAILED);
      }
    }
    // sector loop
  }
  SPI_DisableCard();
  return (FF_ERR_NONE);
}

uint8_t MMC_Command(uint8_t cmd, uint32_t arg)
{
  uint8_t c;
  /*flush SPI-bus*/
  uint8_t attempts = 100;
  do {
    response = SPI(0xFF); // get response
  } while (response != 0xFF && attempts--);
  // this gives a minimum of 8 clocks between response and command (nRC)
  crc = 0;
  SPI(cmd); MMC_CRC(cmd);
  c = (uint8_t)(arg >> 24); SPI(c); MMC_CRC(c);
  c = (uint8_t)(arg >> 16); SPI(c); MMC_CRC(c);
  c = (uint8_t)(arg >>  8); SPI(c); MMC_CRC(c);
  c = (uint8_t)(arg      ); SPI(c); MMC_CRC(c);

  crc = crc << 1; // *shift all bits 1 position to the left, to free position 0
  crc ++;     // set LSB to '1'
  SPI(crc);

  attempts = 100;
  do {
    response = SPI(0xFF); // get response
  } while (response == 0xFF && attempts--);

  DEBUG(3,"response %02X",response);
  return response;
}

uint8_t MMC_Command12(void)
{
  SPI(CMD12);
  SPI(0x00);
  SPI(0x00);
  SPI(0x00);
  SPI(0x00);
  SPI(0x00); // dummy CRC7
  SPI(0xFF); // skip stuff byte

  unsigned char Ncr = 100;  // Ncr = 0..8 (SD) / 1..8 (MMC)
  do
      response = SPI(0xFF); // get response
  while (response == 0xFF && Ncr--);

  timeout = Timer_Get(10);      // 10 ms timeout
  while (SPI(0xFF) == 0x00) {// wait until the card is not busy
    if (Timer_Check(timeout)) {
      WARNING("SPI:Card_CMD12 (STOP) timeout!");
      SPI_DisableCard();
      return(0);
    }
  }
  return response;
}

void MMC_CRC(uint8_t c)
{
  uint8_t i;

  for (i=0; i<8; i++) {
    crc = crc << 1;
    if (  c & 0x80) crc ^= 0x09;
    if (crc & 0x80) crc ^= 0x09;
    c = c << 1;
  }
}


