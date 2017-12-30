//*----------------------------------------------------------------------------
//*      ATMEL Microcontroller Software Support  -  ROUSSET  -
//*----------------------------------------------------------------------------
//* The software is delivered "AS IS" without warranty or condition of any
//* kind, either express, implied or statutory. This includes without
//* limitation any warranty or condition with respect to merchantability or
//* fitness for any particular purpose, or against the infringements of
//* intellectual property rights of others.
//*----------------------------------------------------------------------------
//* File Name           : cdc_enumerate.c
//* Object              : Handle CDC enumeration
//*
//* 1.0 Apr 20 200   : ODi Creation
//* 1.1 14/Sep/2004 JPP : Minor change
//* 1.1 15/12/2004  JPP : suppress warning
//*----------------------------------------------------------------------------

// 12. Apr. 2006: added modification found in the mikrocontroller.net gcc-Forum 
//                additional line marked with /* +++ */
// 1. Sept. 2006: fixed case: board.h -> Board.h

#if !defined(SAME70Q21)

#include "board.h"
#include "cdc_enumerate.h"

struct _AT91S_CDC pCDC;

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define AT91C_EP_IN_SIZE 0x40

const char devDescriptor[] = {
  /* Device descriptor */
  0x12,   // bLength
  0x01,   // bDescriptorType
  0x10,   // bcdUSBL
  0x01,   //
  0x02,   // bDeviceClass:    CDC class code
  0x00,   // bDeviceSubclass: CDC class sub code
  0x00,   // bDeviceProtocol: CDC Device protocol
  0x08,   // bMaxPacketSize0
  0xEB,   // idVendorL (Atmel)
  0x03,   //
  0xFF,   // idProductL (hopefully free...)
  0x60,   //
  0x10,   // bcdDeviceL
  0x01,   //
  0x00,   // iManufacturer    // 0x01
  0x00,   // iProduct
  0x00,   // SerialNumber
  0x01    // bNumConfigs
};

const char cfgDescriptor[] = {
  /* ============== CONFIGURATION 1 =========== */
  /* Configuration 1 descriptor */
  0x09,   // CbLength
  0x02,   // CbDescriptorType
  0x43,   // CwTotalLength 2 EP + Control
  0x00,
  0x02,   // CbNumInterfaces
  0x01,   // CbConfigurationValue
  0x00,   // CiConfiguration
  0xC0,   // CbmAttributes 0xA0
  0x00,   // CMaxPower

  /* Communication Class Interface Descriptor Requirement */
  0x09, // bLength
  0x04, // bDescriptorType
  0x00, // bInterfaceNumber
  0x00, // bAlternateSetting
  0x01, // bNumEndpoints
  0x02, // bInterfaceClass
  0x02, // bInterfaceSubclass
  0x00, // bInterfaceProtocol
  0x00, // iInterface

  /* Header Functional Descriptor */
  0x05, // bFunction Length
  0x24, // bDescriptor type: CS_INTERFACE
  0x00, // bDescriptor subtype: Header Func Desc
  0x10, // bcdCDC:1.1
  0x01,

  /* ACM Functional Descriptor */
  0x04, // bFunctionLength
  0x24, // bDescriptor Type: CS_INTERFACE
  0x02, // bDescriptor Subtype: ACM Func Desc
  0x00, // bmCapabilities

  /* Union Functional Descriptor */
  0x05, // bFunctionLength
  0x24, // bDescriptorType: CS_INTERFACE
  0x06, // bDescriptor Subtype: Union Func Desc
  0x00, // bMasterInterface: Communication Class Interface
  0x01, // bSlaveInterface0: Data Class Interface

  /* Call Management Functional Descriptor */
  0x05, // bFunctionLength
  0x24, // bDescriptor Type: CS_INTERFACE
  0x01, // bDescriptor Subtype: Call Management Func Desc
  0x00, // bmCapabilities: D1 + D0
  0x01, // bDataInterface: Data Class Interface 1

  /* Endpoint 1 descriptor */
  0x07,   // bLength
  0x05,   // bDescriptorType
  0x83,   // bEndpointAddress, Endpoint 03 - IN
  0x03,   // bmAttributes      INT
  0x08,   // wMaxPacketSize
  0x00,
  0xFF,   // bInterval

  /* Data Class Interface Descriptor Requirement */
  0x09, // bLength
  0x04, // bDescriptorType
  0x01, // bInterfaceNumber
  0x00, // bAlternateSetting
  0x02, // bNumEndpoints
  0x0A, // bInterfaceClass
  0x00, // bInterfaceSubclass
  0x00, // bInterfaceProtocol
  0x00, // iInterface

  /* First alternate setting */
  /* Endpoint 1 descriptor */
  0x07,   // bLength
  0x05,   // bDescriptorType
  0x01,   // bEndpointAddress, Endpoint 01 - OUT
  0x02,   // bmAttributes      BULK
  AT91C_EP_OUT_SIZE,   // wMaxPacketSize
  0x00,
  0x00,   // bInterval

  /* Endpoint 2 descriptor */
  0x07,   // bLength
  0x05,   // bDescriptorType
  0x82,   // bEndpointAddress, Endpoint 02 - IN
  0x02,   // bmAttributes      BULK
  AT91C_EP_IN_SIZE,   // wMaxPacketSize
  0x00,
  0x00    // bInterval
};

/* USB standard request code */
#define STD_GET_STATUS_ZERO           0x0080
#define STD_GET_STATUS_INTERFACE      0x0081
#define STD_GET_STATUS_ENDPOINT       0x0082

#define STD_CLEAR_FEATURE_ZERO        0x0100
#define STD_CLEAR_FEATURE_INTERFACE   0x0101
#define STD_CLEAR_FEATURE_ENDPOINT    0x0102

#define STD_SET_FEATURE_ZERO          0x0300
#define STD_SET_FEATURE_INTERFACE     0x0301
#define STD_SET_FEATURE_ENDPOINT      0x0302

#define STD_SET_ADDRESS               0x0500
#define STD_GET_DESCRIPTOR            0x0680
#define STD_SET_DESCRIPTOR            0x0700
#define STD_GET_CONFIGURATION         0x0880
#define STD_SET_CONFIGURATION         0x0900
#define STD_GET_INTERFACE             0x0A81
#define STD_SET_INTERFACE             0x0B01
#define STD_SYNCH_FRAME               0x0C82

/* CDC Class Specific Request Code */
#define GET_LINE_CODING               0x21A1
#define SET_LINE_CODING               0x2021
#define SET_CONTROL_LINE_STATE        0x2221


typedef struct {
  unsigned int dwDTERRate;
  char bCharFormat;
  char bParityType;
  char bDataBits;
} AT91S_CDC_LINE_CODING, *AT91PS_CDC_LINE_CODING;

AT91S_CDC_LINE_CODING line = {
  115200, // baudrate
  0,      // 1 Stop Bit
  0,      // None Parity
  8};     // 8 Data bits

/// mt uint currentReceiveBank = AT91C_UDP_RX_DATA_BK0;

static uchar UDP_IsConfigured(AT91PS_CDC pCdc);
static uint UDP_Read(AT91PS_CDC pCdc, char *pData, uint length);
static uint UDP_Write(AT91PS_CDC pCdc, const char *pData, uint length);
static void CDC_Enumerate(AT91PS_CDC pCdc);


//*----------------------------------------------------------------------------
//* \fn    CDC_Open
//* \brief
//*----------------------------------------------------------------------------
AT91PS_CDC CDC_Open(AT91PS_CDC pCdc, AT91PS_UDP pUdp)
{
  pCdc->pUdp = pUdp;
  pCdc->currentConfiguration = 0;
  pCdc->currentConnection    = 0;
  pCdc->currentRcvBank       = AT91C_UDP_RX_DATA_BK0;
  pCdc->IsConfigured = UDP_IsConfigured;
  pCdc->Write        = UDP_Write;
  pCdc->Read         = UDP_Read;
  return pCdc;
}

//*----------------------------------------------------------------------------
//* \fn    UDP_IsConfigured
//* \brief Test if the device is configured and handle enumeration
//*----------------------------------------------------------------------------
static uchar UDP_IsConfigured(AT91PS_CDC pCdc)
{
  AT91PS_UDP pUDP = pCdc->pUdp;
  AT91_REG isr = pUDP->UDP_ISR;

  if (isr & AT91C_UDP_ENDBUSRES) {
    pUDP->UDP_ICR = AT91C_UDP_ENDBUSRES;
    // reset all endpoints
    pUDP->UDP_RSTEP  = (unsigned int)-1;
    pUDP->UDP_RSTEP  = 0;
    // Enable the function
    pUDP->UDP_FADDR = AT91C_UDP_FEN;
    // Configure endpoint 0
    pUDP->UDP_CSR[0] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL);
    pCdc->currentConfiguration = 0; /* +++ */
  }
  else if (isr & AT91C_UDP_EPINT0) {
    pUDP->UDP_ICR = AT91C_UDP_EPINT0;
    CDC_Enumerate(pCdc);
  }
  return pCdc->currentConfiguration;
}

//*----------------------------------------------------------------------------
//* \fn    UDP_Read
//* \brief Read available data from Endpoint OUT
//*----------------------------------------------------------------------------
static uint UDP_Read(AT91PS_CDC pCdc, char *pData, uint length)
{
  AT91PS_UDP pUdp = pCdc->pUdp;
  uint packetSize, nbBytesRcv = 0, currentReceiveBank = pCdc->currentRcvBank;

  while (length) {

    if ( !UDP_IsConfigured(pCdc) )
      break;

    if ( pUdp->UDP_CSR[AT91C_EP_OUT] & currentReceiveBank ) {
      packetSize = MIN(pUdp->UDP_CSR[AT91C_EP_OUT] >> 16, length);
      length -= packetSize;
      if (packetSize < AT91C_EP_OUT_SIZE)
        length = 0;

      while(packetSize--)
        pData[nbBytesRcv++] = pUdp->UDP_FDR[AT91C_EP_OUT];
      pUdp->UDP_CSR[AT91C_EP_OUT] &= ~(currentReceiveBank);
      if (currentReceiveBank == AT91C_UDP_RX_DATA_BK0)
        currentReceiveBank = AT91C_UDP_RX_DATA_BK1;
      else
        currentReceiveBank = AT91C_UDP_RX_DATA_BK0;
    }
    else
    {
      length = 0;
      return 0;
    }
  }

  pCdc->currentRcvBank = currentReceiveBank;
  return nbBytesRcv;

}

//*----------------------------------------------------------------------------
//* \fn    CDC_Write
//* \brief Send through endpoint 2
//*----------------------------------------------------------------------------
static uint UDP_Write(AT91PS_CDC pCdc, const char *pData, uint length)
{
  AT91PS_UDP pUdp = pCdc->pUdp;
  uint cpt = 0;

  // Send the first packet
  cpt = MIN(length, AT91C_EP_IN_SIZE);
  length -= cpt;
  while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *pData++;
  pUdp->UDP_CSR[AT91C_EP_IN] |= AT91C_UDP_TXPKTRDY;

  while (length) {
    // Fill the second bank
    cpt = MIN(length, AT91C_EP_IN_SIZE);
    length -= cpt;
    while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *pData++;
    // Wait for the the first bank to be sent
    while ( !(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP) )
      if ( !UDP_IsConfigured(pCdc) ) return length;
    pUdp->UDP_CSR[AT91C_EP_IN] &= ~(AT91C_UDP_TXCOMP);
    while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);
    pUdp->UDP_CSR[AT91C_EP_IN] |= AT91C_UDP_TXPKTRDY;
  }

  // Wait for the end of transfer
  while ( !(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP) )
    if ( !UDP_IsConfigured(pCdc) ) return length;

  pUdp->UDP_CSR[AT91C_EP_IN] &= ~(AT91C_UDP_TXCOMP);
  while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);

  return length;
}

//*----------------------------------------------------------------------------
//* \fn    USB_SendData
//* \brief Send Data through the control endpoint
//*----------------------------------------------------------------------------
unsigned int csrTab[100];
unsigned char csrIdx = 0;

static void USB_SendData(AT91PS_UDP pUdp, const char *pData, uint length)
{
  uint cpt = 0;
  AT91_REG csr;

  do {
    cpt = MIN(length, 8);
    length -= cpt;

    while (cpt--)
      pUdp->UDP_FDR[0] = *pData++;

    if (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) {
      pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
      while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
    }

    pUdp->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY;
    do {
      csr = pUdp->UDP_CSR[0];

      // Data IN stage has been stopped by a status OUT
      if (csr & AT91C_UDP_RX_DATA_BK0) {
        pUdp->UDP_CSR[0] &= ~(AT91C_UDP_RX_DATA_BK0);
        return;
      }
    } while ( !(csr & AT91C_UDP_TXCOMP) );

  } while (length);

  if (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) {
    pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
    while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
  }
}

//*----------------------------------------------------------------------------
//* \fn    USB_SendZlp
//* \brief Send zero length packet through the control endpoint
//*----------------------------------------------------------------------------
void USB_SendZlp(AT91PS_UDP pUdp)
{
  pUdp->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY;
  while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) );
  pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
  while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
}

//*----------------------------------------------------------------------------
//* \fn    USB_SendStall
//* \brief Stall the control endpoint
//*----------------------------------------------------------------------------
void USB_SendStall(AT91PS_UDP pUdp)
{
  pUdp->UDP_CSR[0] |= AT91C_UDP_FORCESTALL;
  while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_ISOERROR) );
  pUdp->UDP_CSR[0] &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR);
  while (pUdp->UDP_CSR[0] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR));
}

//*----------------------------------------------------------------------------
//* \fn    CDC_Enumerate
//* \brief This function is a callback invoked when a SETUP packet is received
//*----------------------------------------------------------------------------
static void CDC_Enumerate(AT91PS_CDC pCdc)
{
  AT91PS_UDP pUDP = pCdc->pUdp;
  uchar bmRequestType, bRequest;
  ushort wValue, wIndex, wLength, wStatus;


  if ( !(pUDP->UDP_CSR[0] & AT91C_UDP_RXSETUP) )
    return;

  bmRequestType = pUDP->UDP_FDR[0];
  bRequest      = pUDP->UDP_FDR[0];
  wValue        = (pUDP->UDP_FDR[0] & 0xFF);
  wValue       |= (pUDP->UDP_FDR[0] << 8);
  wIndex        = (pUDP->UDP_FDR[0] & 0xFF);
  wIndex       |= (pUDP->UDP_FDR[0] << 8);
  wLength       = (pUDP->UDP_FDR[0] & 0xFF);
  wLength      |= (pUDP->UDP_FDR[0] << 8);

  if (bmRequestType & 0x80) {
    pUDP->UDP_CSR[0] |= AT91C_UDP_DIR;
    while ( !(pUDP->UDP_CSR[0] & AT91C_UDP_DIR) );
  }
  pUDP->UDP_CSR[0] &= ~AT91C_UDP_RXSETUP;
  while ( (pUDP->UDP_CSR[0]  & AT91C_UDP_RXSETUP)  );

  // Handle supported standard device request Cf Table 9-3 in USB specification Rev 1.1
  switch ((bRequest << 8) | bmRequestType) {
  case STD_GET_DESCRIPTOR:
    if (wValue == 0x100)       // Return Device Descriptor
      USB_SendData(pUDP, devDescriptor, MIN(sizeof(devDescriptor), wLength));
    else if (wValue == 0x200)  // Return Configuration Descriptor
      USB_SendData(pUDP, cfgDescriptor, MIN(sizeof(cfgDescriptor), wLength));
    else
      USB_SendStall(pUDP);
    break;
  case STD_SET_ADDRESS:
    USB_SendZlp(pUDP);
    pUDP->UDP_FADDR = (AT91C_UDP_FEN | wValue);
    pUDP->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_FADDEN : 0;
    break;
  case STD_SET_CONFIGURATION:
    pCdc->currentConfiguration = wValue;
    USB_SendZlp(pUDP);
    pUDP->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_CONFG : AT91C_UDP_FADDEN;
    pUDP->UDP_CSR[1] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT) : 0;
    pUDP->UDP_CSR[2] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)  : 0;
    pUDP->UDP_CSR[3] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN)   : 0;
    break;
  case STD_GET_CONFIGURATION:
    USB_SendData(pUDP, (char *) &(pCdc->currentConfiguration), sizeof(pCdc->currentConfiguration));
    break;
  case STD_GET_STATUS_ZERO:
    wStatus = 0;
    USB_SendData(pUDP, (char *) &wStatus, sizeof(wStatus));
    break;
  case STD_GET_STATUS_INTERFACE:
    wStatus = 0;
    USB_SendData(pUDP, (char *) &wStatus, sizeof(wStatus));
    break;
  case STD_GET_STATUS_ENDPOINT:
    wStatus = 0;
    wIndex &= 0x0F;
    if ((pUDP->UDP_GLBSTATE & AT91C_UDP_CONFG) && (wIndex <= 3)) {
      wStatus = (pUDP->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
      USB_SendData(pUDP, (char *) &wStatus, sizeof(wStatus));
    }
    else if ((pUDP->UDP_GLBSTATE & AT91C_UDP_FADDEN) && (wIndex == 0)) {
      wStatus = (pUDP->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
      USB_SendData(pUDP, (char *) &wStatus, sizeof(wStatus));
    }
    else
      USB_SendStall(pUDP);
    break;
  case STD_SET_FEATURE_ZERO:
    USB_SendStall(pUDP);
      break;
  case STD_SET_FEATURE_INTERFACE:
    USB_SendZlp(pUDP);
    break;
  case STD_SET_FEATURE_ENDPOINT:
    wIndex &= 0x0F;
    if ((wValue == 0) && wIndex && (wIndex <= 3)) {
      pUDP->UDP_CSR[wIndex] = 0;
      USB_SendZlp(pUDP);
    }
    else
      USB_SendStall(pUDP);
    break;
  case STD_CLEAR_FEATURE_ZERO:
    USB_SendStall(pUDP);
      break;
  case STD_CLEAR_FEATURE_INTERFACE:
    USB_SendZlp(pUDP);
    break;
  case STD_CLEAR_FEATURE_ENDPOINT:
    wIndex &= 0x0F;
    if ((wValue == 0) && wIndex && (wIndex <= 3)) {
      if (wIndex == 1)
        pUDP->UDP_CSR[1] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT);
      else if (wIndex == 2)
        pUDP->UDP_CSR[2] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN);
      else if (wIndex == 3)
        pUDP->UDP_CSR[3] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN);
      USB_SendZlp(pUDP);
    }
    else
      USB_SendStall(pUDP);
    break;

  // handle CDC class requests
  case SET_LINE_CODING:
    while ( !(pUDP->UDP_CSR[0] & AT91C_UDP_RX_DATA_BK0) );
    pUDP->UDP_CSR[0] &= ~(AT91C_UDP_RX_DATA_BK0);
    USB_SendZlp(pUDP);
    break;
  case GET_LINE_CODING:
    USB_SendData(pUDP, (char *) &line, MIN(sizeof(line), wLength));
    break;
  case SET_CONTROL_LINE_STATE:
    pCdc->currentConnection = wValue;
    USB_SendZlp(pUDP);
    break;
  default:
    USB_SendStall(pUDP);
      break;
  }
}

//*----------------------------------------------------------------------------
//* \fn    USB_Open
//* \brief This function Open the USB device
//*----------------------------------------------------------------------------
void USB_Open(void)
{
    // Set the PLL USB Divider
    AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;

    // Specific Chip USB Initialisation
    // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);

    // Enable UDP PullUp (USB_DP_PUP) : enable & Clear of the corresponding PIO
    // Set in PIO mode and Configure in Output
    AT91C_BASE_PIOA->PIO_PER = AT91C_PIO_PA16; // Set in PIO mode
    AT91C_BASE_PIOA->PIO_OER = AT91C_PIO_PA16; // Configure in Output

    // Clear for set the Pull-up resistor
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PIO_PA16;

    // CDC Open by structure initialization
    CDC_Open(&pCDC, AT91C_BASE_UDP);
}

#endif // !defined(SAME70Q21)
