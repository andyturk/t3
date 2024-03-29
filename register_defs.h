#pragma once

#include <stdint.h>

#define __I  volatile
#define __O  volatile
#define __IO volatile

struct gpio_register_map {
  __I  uint32_t  RESERVED0[255];
  __IO uint32_t  DATA;               // GPIO Data
  __IO uint32_t  DIR;                // GPIO Direction
  __IO uint32_t  IS;                 // GPIO Interrupt Sense
  __IO uint32_t  IBE;                // GPIO Interrupt Both Edges
  __IO uint32_t  IEV;                // GPIO Interrupt Event
  __IO uint32_t  IM;                 // GPIO Interrupt Mask
  __IO uint32_t  RIS;                // GPIO Raw Interrupt Status
  __IO uint32_t  MIS;                // GPIO Masked Interrupt Status
  __O  uint32_t  ICR;                // GPIO Interrupt Clear
  __IO uint32_t  AFSEL;              // GPIO Alternate Function Select
  __I  uint32_t  RESERVED1[55];      
  __IO uint32_t  DR2R;               // GPIO 2-mA Drive Select
  __IO uint32_t  DR4R;               // GPIO 4-mA Drive Select
  __IO uint32_t  DR8R;               // GPIO 8-mA Drive Select
  __IO uint32_t  ODR;                // GPIO Open Drain Select
  __IO uint32_t  PUR;                // GPIO Pull-Up Select
  __IO uint32_t  PDR;                // GPIO Pull-Down Select
  __IO uint32_t  SLR;                // GPIO Slew Rate Control Select
  __IO uint32_t  DEN;                // GPIO Digital Enable
  __IO uint32_t  LOCK;               // GPIO Lock
  __I  uint32_t  CR;                 // GPIO Commit
  __IO uint32_t  AMSEL;              // GPIO Analog Mode Select
  __IO uint32_t  PCTL;               // GPIO Port Control
};

// UART Register Map
typedef struct {
  __IO uint32_t  DR;                 // UART Data
  union {
    __IO uint32_t  UART_ALT_ECR;     // UART Receive Status/Error Clear
    __IO uint32_t  RSR;              // UART Receive Status/Error Clear
  } ;
  __I  uint32_t  RESERVED0[4];
  __IO uint32_t  FR;                 // UART Flag
  __I  uint32_t  RESERVED1;
  __IO uint32_t  ILPR;               // UART IrDA Low-Power Register
  __IO uint32_t  IBRD;               // UART Integer Baud-Rate Divisor
  __IO uint32_t  FBRD;               // UART Fractional Baud-Rate Divisor
  __IO uint32_t  LCRH;               // UART Line Control
  __IO uint32_t  CTL;                // UART Control
  __IO uint32_t  IFLS;               // UART Interrupt FIFO Level Select
  __IO uint32_t  IM;                 // UART Interrupt Mask
  __IO uint32_t  RIS;                // UART Raw Interrupt Status
  __IO uint32_t  MIS;                // UART Masked Interrupt Status
  __O  uint32_t  ICR;                // UART Interrupt Clear
  __IO uint32_t  DMACTL;             // UART DMA Control
  __I  uint32_t  RESERVED2[17];
  __IO uint32_t  LCTL;               // UART LIN Control
  __IO uint32_t  LSS;                // UART LIN Snap Shot
  __IO uint32_t  LTIM;               // UART LIN Timer
} uart_register_map;

// UART Interrupt Mask (UARTIM, UARTRIS, UARTMIS, UARTICR)
typedef union {
  uint32_t w;
  struct {
    unsigned int RIIM       : 1; // Ring Indicator
    unsigned int CTSIM      : 1; // Clear To Send
    unsigned int DCDIM      : 1; // Data Carrier Detect
    unsigned int DSRIM      : 1; // Data Set Ready
    unsigned int RXIM       : 1; // Receive Interrupt
    unsigned int TXIM       : 1; // Transmit Interrupt
    unsigned int RTIM       : 1; // Receive Timeout
    unsigned int FEIM       : 1; // Framing Error
    unsigned int PEIM       : 1; // Parity Error
    unsigned int BEIM       : 1; // Break Error
    unsigned int OEIM       : 1; // Overrun Error
    unsigned int _reserved0 : 2;
    unsigned int LMSBIM     : 1; // LIN Mode Sync Break
    unsigned int LME1IM     : 1; // LIN Mode Edge 1
    unsigned int LME5IM     : 1; // LIN Mode Edge 5
    unsigned int _reserved1 : 16;
  } __attribute__ ((packed));
} uart_interrupt_mask;


/**
  * @brief Register map for ADC0 peripheral (ADC0)
  */

typedef struct {                                    /*!< ADC0 Structure                                                        */
  __IO uint32_t  ACTSS;                             /*!< ADC Active Sample Sequencer                                           */
  __IO uint32_t  RIS;                               /*!< ADC Raw Interrupt Status                                              */
  __IO uint32_t  IM;                                /*!< ADC Interrupt Mask                                                    */
  __IO uint32_t  ISC;                               /*!< ADC Interrupt Status and Clear                                        */
  __IO uint32_t  OSTAT;                             /*!< ADC Overflow Status                                                   */
  __IO uint32_t  EMUX;                              /*!< ADC Event Multiplexer Select                                          */
  __IO uint32_t  USTAT;                             /*!< ADC Underflow Status                                                  */
  __I  uint32_t  RESERVED0;
  __IO uint32_t  SSPRI;                             /*!< ADC Sample Sequencer Priority                                         */
  __IO uint32_t  SPC;                               /*!< ADC Sample Phase Control                                              */
  __IO uint32_t  PSSI;                              /*!< ADC Processor Sample Sequence Initiate                                */
  __I  uint32_t  RESERVED1;
  __IO uint32_t  SAC;                               /*!< ADC Sample Averaging Control                                          */
  __IO uint32_t  DCISC;                             /*!< ADC Digital Comparator Interrupt Status and Clear                     */
  __IO uint32_t  CTL;                               /*!< ADC Control                                                           */
  __I  uint32_t  RESERVED2;

  struct {
    __IO uint32_t  MUX;                            /*!< ADC Sample Sequence Input Multiplexer Select 0                        */
    __IO uint32_t  CTL;                            /*!< ADC Sample Sequence Control 0                                         */
    __IO uint32_t  FIFO;                           /*!< ADC Sample Sequence Result FIFO 0                                     */
    __IO uint32_t  FSTAT;                          /*!< ADC Sample Sequence FIFO 0 Status                                     */
    __IO uint32_t  OP;                             /*!< ADC Sample Sequence 0 Operation                                       */
    __IO uint32_t  DC;                             /*!< ADC Sample Sequence 0 Digital Comparator Select                       */
    __IO uint32_t  EMUX;                           // ???
    __I  uint32_t  RESERVED3[1];
  } __attribute__ ((packed)) SS[4];

  __I  uint32_t  RESERVED4[784];
  __IO uint32_t  DCRIC;                             /*!< ADC Digital Comparator Reset Initial Conditions                       */
  __I  uint32_t  RESERVED5[63];
  __IO uint32_t  DCCTL[8];                          /*!< ADC Digital Comparator Controls                                       */
  __I  uint32_t  RESERVED6[8];
  __IO uint32_t  DCCMP[8];                          /*!< ADC Digital Comparator Range                                          */
} adc_register_map;

