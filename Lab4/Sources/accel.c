/*! @file
 *
 *  @brief Routines to access the Accelerometer on the TWR-K70F120M.
 *
 *  This contains the functions for operating the Accelerometer.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-05-21
 */
/*!
**  @addtogroup Accel_module Accelerometer module documentation
**  @{}
*/


// Accelerometer functions
#include "accel.h"

// Inter-Integrated Circuit
#include "I2C.h"

// Median filter
#include "median.h"

//LEDs HAL
#include "LEDs.h"

// K70 module registers
#include "MK70F12.h"

// CPU and PE_types are needed for critical section variables and the defintion of NULL pointer
#include "CPU.h"
#include "PE_types.h"

// Accelerometer registers
#define ADDRESS_OUT_X_MSB 0x01 // address used to get the value of xyz out, Most significant bit

#define ADDRESS_INT_SOURCE 0x0C

static union
{
  uint8_t byte;			/*!< The INT_SOURCE bits accessed as a byte. */
  struct
  {
    uint8_t SRC_DRDY   : 1;	/*!< Data ready interrupt status. */
    uint8_t               : 1;
    uint8_t SRC_FF_MT  : 1;	/*!< Freefall/motion interrupt status. */
    uint8_t SRC_PULSE  : 1;	/*!< Pulse detection interrupt status. */
    uint8_t SRC_LNDPRT : 1;	/*!< Orientation interrupt status. */
    uint8_t SRC_TRANS  : 1;	/*!< Transient interrupt status. */
    uint8_t SRC_FIFO   : 1;	/*!< FIFO interrupt status. */
    uint8_t SRC_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt status. */
  } bits;			/*!< The INT_SOURCE bits accessed individually. */
} INT_SOURCE_Union;

#define INT_SOURCE     		INT_SOURCE_Union.byte
#define INT_SOURCE_SRC_DRDY	INT_SOURCE_Union.bits.SRC_DRDY
#define INT_SOURCE_SRC_FF_MT	CTRL_REG4_Union.bits.SRC_FF_MT
#define INT_SOURCE_SRC_PULSE	CTRL_REG4_Union.bits.SRC_PULSE
#define INT_SOURCE_SRC_LNDPRT	CTRL_REG4_Union.bits.SRC_LNDPRT
#define INT_SOURCE_SRC_TRANS	CTRL_REG4_Union.bits.SRC_TRANS
#define INT_SOURCE_SRC_FIFO	CTRL_REG4_Union.bits.SRC_FIFO
#define INT_SOURCE_SRC_ASLP	CTRL_REG4_Union.bits.SRC_ASLP

#define ADDRESS_CTRL_REG1 0x2A

typedef enum
{
  DATE_RATE_800_HZ,
  DATE_RATE_400_HZ,
  DATE_RATE_200_HZ,
  DATE_RATE_100_HZ,
  DATE_RATE_50_HZ,
  DATE_RATE_12_5_HZ,
  DATE_RATE_6_25_HZ,
  DATE_RATE_1_56_HZ
} TOutputDataRate;

typedef enum
{
  SLEEP_MODE_RATE_50_HZ,
  SLEEP_MODE_RATE_12_5_HZ,
  SLEEP_MODE_RATE_6_25_HZ,
  SLEEP_MODE_RATE_1_56_HZ
} TSLEEPModeRate;

static union
{
  uint8_t byte;			/*!< The CTRL_REG1 bits accessed as a byte. */
  struct
  {
    uint8_t ACTIVE    : 1;	/*!< Mode selection. */
    uint8_t F_READ    : 1;	/*!< Fast read mode. */
    uint8_t LNOISE    : 1;	/*!< Reduced noise mode. */
    uint8_t DR        : 3;	/*!< Data rate selection. */
    uint8_t ASLP_RATE : 2;	/*!< Auto-WAKE sample frequency. */
  } bits;			/*!< The CTRL_REG1 bits accessed individually. */
} CTRL_REG1_Union;

#define CTRL_REG1     		    CTRL_REG1_Union.byte
#define CTRL_REG1_ACTIVE	    CTRL_REG1_Union.bits.ACTIVE
#define CTRL_REG1_F_READ  	  CTRL_REG1_Union.bits.F_READ
#define CTRL_REG1_LNOISE  	  CTRL_REG1_Union.bits.LNOISE
#define CTRL_REG1_DR	    	  CTRL_REG1_Union.bits.DR
#define CTRL_REG1_ASLP_RATE	  CTRL_REG1_Union.bits.ASLP_RATE

#define ADDRESS_CTRL_REG2 0x2B

#define ADDRESS_CTRL_REG3 0x2C

static union
{
  uint8_t byte;			/*!< The CTRL_REG3 bits accessed as a byte. */
  struct
  {
    uint8_t PP_OD       : 1;	/*!< Push-pull/open drain selection. */
    uint8_t IPOL        : 1;	/*!< Interrupt polarity. */
    uint8_t WAKE_FF_MT  : 1;	/*!< Freefall/motion function in SLEEP mode. */
    uint8_t WAKE_PULSE  : 1;	/*!< Pulse function in SLEEP mode. */
    uint8_t WAKE_LNDPRT : 1;	/*!< Orientation function in SLEEP mode. */
    uint8_t WAKE_TRANS  : 1;	/*!< Transient function in SLEEP mode. */
    uint8_t FIFO_GATE   : 1;	/*!< FIFO gate bypass. */
  } bits;			/*!< The CTRL_REG3 bits accessed individually. */
} CTRL_REG3_Union;

#define CTRL_REG3     		    CTRL_REG3_Union.byte
#define CTRL_REG3_PP_OD		    CTRL_REG3_Union.bits.PP_OD
#define CTRL_REG3_IPOL		    CTRL_REG3_Union.bits.IPOL
#define CTRL_REG3_WAKE_FF_MT	CTRL_REG3_Union.bits.WAKE_FF_MT
#define CTRL_REG3_WAKE_PULSE	CTRL_REG3_Union.bits.WAKE_PULSE
#define CTRL_REG3_WAKE_LNDPRT	CTRL_REG3_Union.bits.WAKE_LNDPRT
#define CTRL_REG3_WAKE_TRANS	CTRL_REG3_Union.bits.WAKE_TRANS
#define CTRL_REG3_FIFO_GATE	  CTRL_REG3_Union.bits.FIFO_GATE

#define ADDRESS_CTRL_REG4 0x2D

static union
{
  uint8_t byte;			/*!< The CTRL_REG4 bits accessed as a byte. */
  struct
  {
    uint8_t INT_EN_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t               : 1;
    uint8_t INT_EN_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_EN_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_EN_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_EN_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_EN_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_EN_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG4 bits accessed individually. */
} CTRL_REG4_Union;

#define CTRL_REG4            		CTRL_REG4_Union.byte
#define CTRL_REG4_INT_EN_DRDY	  CTRL_REG4_Union.bits.INT_EN_DRDY
#define CTRL_REG4_INT_EN_FF_MT	CTRL_REG4_Union.bits.INT_EN_FF_MT
#define CTRL_REG4_INT_EN_PULSE	CTRL_REG4_Union.bits.INT_EN_PULSE
#define CTRL_REG4_INT_EN_LNDPRT	CTRL_REG4_Union.bits.INT_EN_LNDPRT
#define CTRL_REG4_INT_EN_TRANS	CTRL_REG4_Union.bits.INT_EN_TRANS
#define CTRL_REG4_INT_EN_FIFO	  CTRL_REG4_Union.bits.INT_EN_FIFO
#define CTRL_REG4_INT_EN_ASLP	  CTRL_REG4_Union.bits.INT_EN_ASLP

#define ADDRESS_CTRL_REG5 0x2E

static union
{
  uint8_t byte;			/*!< The CTRL_REG5 bits accessed as a byte. */
  struct
  {
    uint8_t INT_CFG_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t                : 1;
    uint8_t INT_CFG_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_CFG_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_CFG_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_CFG_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_CFG_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_CFG_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG5 bits accessed individually. */
} CTRL_REG5_Union;

#define CTRL_REG5     		      	CTRL_REG5_Union.byte
#define CTRL_REG5_INT_CFG_DRDY		CTRL_REG5_Union.bits.INT_CFG_DRDY
#define CTRL_REG5_INT_CFG_FF_MT		CTRL_REG5_Union.bits.INT_CFG_FF_MT
#define CTRL_REG5_INT_CFG_PULSE		CTRL_REG5_Union.bits.INT_CFG_PULSE
#define CTRL_REG5_INT_CFG_LNDPRT	CTRL_REG5_Union.bits.INT_CFG_LNDPRT
#define CTRL_REG5_INT_CFG_TRANS		CTRL_REG5_Union.bits.INT_CFG_TRANS
#define CTRL_REG5_INT_CFG_FIFO		CTRL_REG5_Union.bits.INT_CFG_FIFO
#define CTRL_REG5_INT_CFG_ASLP		CTRL_REG5_Union.bits.INT_CFG_ASLP

const uint32_t ACC_BAUD_RATE = 100000; /*!< The accelerometer is to use a baud rate that is as close to 100 kbps as possible*/
const uint8_t ACC_ADDRESS_SA00 =0x1C; /*!< When SA0 = 0, this is the accelerometer's address */
const uint8_t ACC_ADDRESS_SA01 =0x1D; /*!< When SA0 = 1, this is the accelerometer's address */

static void *DataAccArguments;
static void (*DataAccCallback)(void* DataAccArguments);

static void *ReadAccArguments;
static void (*ReadAccCallback)(void* ReadAccArguments);


bool Accel_Init(const TAccelSetup* const accelSetup)
{
  DataAccArguments = accelSetup->dataReadyCallbackArguments;
  DataAccCallback = accelSetup->dataReadyCallbackFunction;

  ReadAccArguments = accelSetup->readCompleteCallbackArguments;
  ReadAccCallback = accelSetup->readCompleteCallbackFunction;

  TI2CModule initI2C =
  {
    ACC_ADDRESS_SA01, /*!<SA0 is enabled to 1 - in I2C*/
    ACC_BAUD_RATE, /*!< Baud rate has to be as close as possible to 100kps*/
    ReadAccCallback, /*!< Callback function when reading values*/
    ReadAccArguments /*!<Arguments for the callback function */
  };

  // init I2C
  I2C_Init(&initI2C, accelSetup->moduleClk);
  I2C_SelectSlaveDevice(initI2C.primarySlaveAddress);

  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; /*!< Enable pin routing to port B for interrrupt. Port E for I2CSCL and SDA. Found in schematics.pdf */

  PORTB_PCR4 |= PORT_PCR_MUX(1) | PORT_PCR_ISF_MASK; //GPIO, INT1, IRQ1. from hint use INT1 and clearing interrupt flag at init

  Accel_SetMode(ACCEL_POLL); /*!<Protocol Mode to Poll by default */

  /*!< IRQ = 88, 88%32 = 24, PORTB interrupt*/
  NVICICPR2 = (1<< 24);
  NVICISER2= (1<<24);

  return true;
}


void Accel_ReadXYZ(uint8_t data[3])
{
  /*!< Different way to read data, whether it is poll or interrupt*/
  static uint8_t oldXYZ[3];
  if (CurrentMode == ACCEL_POLL)
  {
    I2C_PollRead(ADDRESS_OUT_X_MSB, TempArray, 3); /* Polling method to read data*/
  }
  else
  {
    I2C_IntRead(ADDRESS_OUT_X_MSB, TempArray, 3); /*Interrupt method to read data*/
  }

  if ((TempArray[0] == oldXYZ[0]) &&(TempArray[1] == oldXYZ[1]) && (TempArray[2] == oldXYZ[2]) && (CurrentMode == ACCEL_POLL) && oldXYZ) /*!< If the new XYZ values equals to the old value and the mode is poll*/
  {
    data = NULL; /*!< data equals to null and in main: If data is null = do not send the new values */
  }
  else
  {
    ShiftArray(); /*!< get rid of the oldest value and assign the newest value in the array*/
    /*!< make the array data equal to the new median values */
    data[0] = Median_Filter3(XYZstruct[0].axes.x, XYZstruct[1].axes.x, XYZstruct[2].axes.x);
    data[1] = Median_Filter3(XYZstruct[0].axes.y, XYZstruct[1].axes.y, XYZstruct[2].axes.y);
    data[2] = Median_Filter3(XYZstruct[0].axes.z, XYZstruct[1].axes.z, XYZstruct[2].axes.z);


    /*!< Make the static variable oldXYZ equal to the new value to compare them with the incoming new one and see if it has changed */
    oldXYZ[0] = Median_Filter3(XYZstruct[0].axes.x, XYZstruct[1].axes.x, XYZstruct[2].axes.x);
    oldXYZ[1] = Median_Filter3(XYZstruct[0].axes.y, XYZstruct[1].axes.y, XYZstruct[2].axes.y);
    oldXYZ[2] = Median_Filter3(XYZstruct[0].axes.z, XYZstruct[1].axes.z, XYZstruct[2].axes.z);
  }


}


void Accel_SetMode(const TAccelMode mode)
{
  CTRL_REG1_ACTIVE = 0; /* Deactivate the accelerometer before changing mode */
  I2C_Write(ADDRESS_CTRL_REG1,CTRL_REG1); /* Writing to the accelerometer to deactivate it */
  CTRL_REG1_ASLP_RATE = SLEEP_MODE_RATE_1_56_HZ;
  CTRL_REG1_F_READ = 1;
  if (mode == ACCEL_POLL)
  {
    CurrentMode = ACCEL_POLL;
/*    CTRL_REG1_DR = DATE_RATE_1_56_HZ; !<Set the output data rate to 1Hz for asynchronous*/
    CTRL_REG4_INT_EN_DRDY = 0; /*!< Deactivate the data-ready interrupt when set to polling mode*/
    PORTB_PCR7 &= ~PORT_PCR_IRQC_MASK; /*!< Disable the interrupt port to use polling*/
    I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);
  }
  else if (mode == ACCEL_INT)
  {
    CurrentMode = ACCEL_INT;
    CTRL_REG1_DR = DATE_RATE_1_56_HZ; /*!<Set the output data rate to 1.56Hz for synchronous mode */
    CTRL_REG5_INT_CFG_DRDY = 1; // Interrupt is routed to INT1 pin
    CTRL_REG4_INT_EN_DRDY = 1; //Activate the data-ready interrupt when set to interrupt mode
    PORTB_PCR7 = PORT_PCR_IRQC(0x0A); /*! Enable the interrupt port with falling edge*/
    I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4); // Write the changes to the accelerometer for REG4
    I2C_Write(ADDRESS_CTRL_REG5, CTRL_REG5);  // Write the changes to the accelerometer for REG5
  }

  CTRL_REG1_ACTIVE = 1; /* Activate the accelerometer after changing mode */
  I2C_Write(ADDRESS_CTRL_REG1,CTRL_REG1); /* Writing to the accelerometer to activate it */
}


void __attribute__ ((interrupt)) AccelDataReady_ISR(void)
{
  PORTB_PCR7 |= PORT_PCR_ISF_MASK; // Clear the interrupt
  if (DataAccCallback)
  {
    /*!< Hint 3 in Lab3 Notes - Callback is run every ISR (green LED toggle every second)*/
    (*DataAccCallback)(DataAccArguments); /*! When data has been read by the I2C ISR, the data is ready to be output to the PC (Packet_Put)*/
  }
}


static void ShiftArray(void)
{
  /*!< Shifting the elements in the array - oldest data is lost and newest data is written to the array*/
  XYZstruct[0].axes.x = XYZstruct[1].axes.x;
  XYZstruct[1].axes.x = XYZstruct[2].axes.x;
  XYZstruct[2].axes.x = TempArray[0];

  XYZstruct[0].axes.y = XYZstruct[1].axes.y;
  XYZstruct[1].axes.y = XYZstruct[2].axes.y;
  XYZstruct[2].axes.y = TempArray[1];

  XYZstruct[0].axes.z = XYZstruct[1].axes.z;
  XYZstruct[1].axes.z = XYZstruct[2].axes.z;
  XYZstruct[2].axes.z = TempArray[2];
}
/*!
 * @}
*/
