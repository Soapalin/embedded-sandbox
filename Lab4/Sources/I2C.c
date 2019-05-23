/*! @file
 *
 *  @brief Routines to access the I2C on the TWR-K70F120M.
 *
 *  This contains the functions for operating the I2C.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-05-21
 */
/*!
**  @addtogroup i2C_module I2C module documentation
**  @{
*/

#include "Cpu.h"
#include "MK70F12.h"
#include "I2C.h"
#include "PE_Types.h"

/****************************************************************************************************************
 * TypeDefs declaration
 ***************************************************************************************************************/
typedef enum
{
  START_CONDITION,
  DEVICE_ADDRESS_READ,
  REGISTER_ADDRESS,
  REPEATED_START_CONDITION,
  DEVICE_ADDRESS_WRITE,
  READ_BYTE,
  WRITE_BYTE,
  NAK,
  STOP_CONDITION
}TState;

typedef enum
{
  COMPLETE,
  LAST_BYTE,
  TX_LAST_BYTE
}TISRSTATE;

/****************************************************************************************************************
 * Private function declaration
 ***************************************************************************************************************/
static void* ReadCompleteUserArgumentsGlobal; /*!< Arguments for Callback function */
static void (*ReadCompleteCallbackGlobal)(void *); /*!< Callback function */
static bool WaitForAcknowledgment(void); /*!< Determines whether data acknowledged */
static void WaitForIdle(void); /*!< Waits until I2C is not busy */


/****************************************************************************************************************
 * Global variables, Enumerations Declaration and Structures
 ***************************************************************************************************************/
static const uint8_t I2C_D_READ = 0x01;
static const uint8_t I2C_D_WRITE = 0xFE;
static uint8_t SlaveAddress;
static uint8_t RegisterAddress;
static uint8_t *RxData;
static uint8_t NumberOfBytes;

TState State;
TState ISRState;
TISRSTATE ISRData;

const static uint8_t Multiplier[] = { 1, 2, 4 };
const static uint16_t SclDivider[] =
{
  20, 22, 24, 26, 28, 30,
  34, 40, 28, 32, 36, 40,
  44, 48, 56, 68, 48, 56,
  64, 72, 80, 88, 104,128,
  80,  96,  112, 128, 144,
  160, 192, 240, 160, 192,
  224, 256, 288, 320, 384,
  480, 320, 384, 448, 512,
  576, 640, 768, 960, 640,
  768,  896,  1024,  1152,
  1280, 1536, 1920,  1280,
  1536, 1792, 2048,  2304,
  2560,     3072,    3840
};

bool I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk) {
  uint8_t i, j, mult, scl, irq; /*!< Initiate variables used to determine most suitable multipler and SCL Divider*/
  uint32_t testingBaudRate; /*!< variable to hold current testing baudrate value*/
  uint32_t baudRateError = 100000; /*!< BaudRate should be closest to 100kps as possible (ref. Lab 4 Notes)*/

  if ((aI2CModule->baudRate <= 0)|| (moduleClk <= 0)) { /*!< Check to see if BaudRate is an invalid value */
      return false; /*!< If invalid return false */
  }

  for (i=0; i <(sizeof(SclDivider)/sizeof(uint16_t))-1; i++) /*!< Sequentiality go through SCL Divider values from globally declared struct */
  {
    for (j=0; j< (sizeof(Multiplier)/sizeof(uint8_t))-1; j++) /*!< Sequentiality go through Multiplierr values from globally declared struct */
    {
      if ((testingBaudRate-aI2CModule ->baudRate) < baudRateError) /*!< Check if the baudRate is close to 100kbps */
      {
	testingBaudRate = moduleClk / Multiplier[j] * SclDivider[i]; /*!< calculate current testing baudRate */
	baudRateError = (testingBaudRate - aI2CModule->baudRate); /*!< calculate difference between testingbaudrate and passed in baudrate */
	mult = j; /*!< Store Current value of j into variable to retain as the best possible Multiplier, only replaced if better one found */
	scl = i; /*!< Store Current value of i into variable to retain as the best possible SCL Divider, only replaced if better one found */
       }
    }
  }

  ReadCompleteUserArgumentsGlobal = aI2CModule->readCompleteCallbackArguments; /*!< Set stored User arguments into globally(private) function */
  ReadCompleteCallbackGlobal = aI2CModule->readCompleteCallbackFunction;  /*!< Set stored User arguments into globally(private) function */

  SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK; /*!< Enables Clock Gate for Inter-Integrated Circuit (IIC/I2C) (pg. 345-346) */
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; /*!< Enable Pin Routing Port E to ensure it is accessible */

  /*!< The Open Drain mode on SDA, SCL pins has to be enabled by setting PORTx_PCRn[ODE] bits. (pg. 184) */
  PORTE_PCR18 = PORT_PCR_MUX(4) | PORT_PCR_ODE_MASK; /*!< PCR18 ALT 4 -> I2C0_SDA (pg. 280) */
  PORTE_PCR19 = PORT_PCR_MUX(4) | PORT_PCR_ODE_MASK; /*!< PCR19 ALT 4 -> I2C0_SCL (pg. 280) */

  I2C_SelectSlaveDevice(aI2CModule->primarySlaveAddress); /*!< Assign Slave Device */
  I2C0_F = I2C_F_MULT(Multiplier[mult]) | I2C_F_ICR(scl); // Set baud rate
  I2C0_S = I2C_S_IICIF_MASK; /*!< Clear interrupts */

  NVICICPR0 = (1 << 24); /*!< Clear pending interrupts */
  NVICISER0 = (1 << 24); /*!< Enable interrupts */

  I2C0_C1 |= I2C_C1_IICEN_MASK; //Enable I2C (pg. 1887-1888)

  return true;
}

void I2C_SelectSlaveDevice(const uint8_t slaveAddress) {
  SlaveAddress = slaveAddress; /*!< Set slaveAddress to a global variable */
}

void I2C_Write(const uint8_t registerAddress, const uint8_t data) {
  EnterCritical(); /*!< Prevent Interrupts */
  WaitForIdle(); /*< Wait until not busy */
  switch(State) /*!< Flow of the States referenced from MMA8451Q Data Sheet (pg. 19) */
  {
    //Single Byte Write = ST + DA + W -> (AK) -> RA -> (AK) -> WB -> (AK) -> SP
    case START_CONDITION:
      I2C0_C1 |= I2C_C1_MST_MASK; /*!< Master mode selected */
      I2C0_C1 |= I2C_C1_TX_MASK; /*!< Transmit mode selected */
      State = DEVICE_ADDRESS_WRITE; /*!< Move on to next case */

    case DEVICE_ADDRESS_WRITE:
      I2C0_D = (SlaveAddress << 1) | I2C_D_WRITE; /*!< Set Device Address and set to write as per MMA8451Q Data Sheet (pg. 19)  */
      if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
        State = REGISTER_ADDRESS; /*!< Move on to next case */
      }

    case REGISTER_ADDRESS:
      I2C0_D = registerAddress; /*!< Set data register address as I2C data to be transmitted. */
      if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
        State = WRITE_BYTE; /*!< Move on to next case */
      }

    case WRITE_BYTE:
      I2C0_D = data; /*!< Set data as I2C data to be transmitted. */
      if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
        State = STOP_CONDITION; /*!< Move on to next case */
      }

    case STOP_CONDITION:
      I2C0_C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); /*!< Disable Master and Transmitter after writing completion */
      break;
  }
  State = 0; /*!< Reset State Value for next case statement usage*/
  ExitCritical(); /*!< Allow Interrupts */
}

void I2C_PollRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes) {
  uint8_t incomingDataCounter; /*!< Declare local variable to be used to counter through Data Bytes */
  EnterCritical(); /*!< Prevent Interrupts */
  WaitForIdle(); /*< Wait until not busy */

  switch(State) /*!< Flow of the States referenced from MMA8451Q Data Sheet (pg. 19) */
  {
    //Single Byte Read = ST + DA + W -> (AK) -> RA -> (AK) -> SR + DA + R -> (AK + RB) -> NAK + SP
    //Multi  Byte Read = ST + DA + W -> (AK) -> RA -> (AK) -> SR + DA + R -> (AK + RB) -> AK -> (RB) -> AK -> (RB) -> NAK + SP
    case START_CONDITION:
      I2C0_C1 |= I2C_C1_MST_MASK; //Master mode selected, start signal
      I2C0_C1 |= I2C_C1_TX_MASK; //Transmit mode selected
      State = DEVICE_ADDRESS_WRITE; /*!< Move on to next case */

    case DEVICE_ADDRESS_WRITE:
      I2C0_D = (SlaveAddress << 1) | I2C_D_WRITE; /*!< Set Device Address and set to write as per MMA8451Q Data Sheet (pg. 19)  */
      State = REGISTER_ADDRESS; /*!< Move on to next case */

    case REGISTER_ADDRESS:
      I2C0_D = registerAddress;
      if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
        State = REPEATED_START_CONDITION; /*!< Move on to next case */
      }

    case REPEATED_START_CONDITION:
      I2C0_C1 |= I2C_C1_RSTA_MASK; /*!< Repeat start */
      State = DEVICE_ADDRESS_READ; /*!< Move on to next case */


    case DEVICE_ADDRESS_READ:
      I2C0_D = (SlaveAddress << 1) | I2C_D_READ; /*!< Set Device Address and set to read as per MMA8451Q Data Sheet (pg. 19)  */
      State = READ_BYTE; /*!< Move on to next case */

    case READ_BYTE:
      if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
	I2C0_C1 &= ~(I2C_C1_TX_MASK); /*!< Receive Mode Selected */
	I2C0_C1 &= ~(I2C_C1_TXAK_MASK); /*!< Send ACK after each transmission */
        data[0] = I2C0_D; /*!< Assign data */
        for (incomingDataCounter = 0; incomingDataCounter < nbBytes-2; incomingDataCounter++)
        {
            data[incomingDataCounter] = I2C0_D; /*!< Read bytes (between 1 -nbBytes-2) */
            WaitForAcknowledgment(); /*!< Wait until Acknowledgment */
        }
        I2C0_C1 |= I2C_C1_TXAK_MASK;//Set NAK (pg. 1865)
        data[incomingDataCounter++] = I2C0_D; /*!< Read second last byte */
        if(WaitForAcknowledgment()) { /*!< Wait until Acknowledgment */
          State = STOP_CONDITION;
        }
      }

     case STOP_CONDITION:
       I2C0_C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); /*!< Disable Master and Transmit (becomes Receiver) */
       data[incomingDataCounter++] = I2C0_D; /*!< Read last byte */
       break;
  }
  State = 0; /*!< Reset State Value for next case statement usage*/
  ExitCritical(); /*!< Allow Interrupts */
}

void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes) {
  EnterCritical(); /*!< Prevent Interrupts */
  I2C0_C1 |= I2C_C1_IICIE_MASK; /*!< I2C Interrupt Enable */

  NumberOfBytes = nbBytes; /*!< Assign nbBytes to a global variable for ISR to access */
  RegisterAddress = registerAddress; /*!< Assign registerAddress to a global variable for ISR to access */
  RxData = data; /*!< Assign data to a global variable for ISR to access */
  WaitForIdle(); /*< Wait until not busy */
  switch(State) /*!< Flow of the States referenced from MMA8451Q Data Sheet (pg. 19) */
  {
    case START_CONDITION:
      I2C0_C1 |= I2C_C1_MST_MASK; /*!< Master Mode */
      I2C0_C1 |= I2C_C1_TX_MASK; /*!< Transmit Mode */
      State = DEVICE_ADDRESS_WRITE; /*!< Move on to next case */

    case DEVICE_ADDRESS_WRITE:
      I2C0_D = (SlaveAddress << 1) | I2C_D_WRITE; /*!< Set Device Address and set to write as per MMA8451Q Data Sheet (pg. 19)  */
      break;
  }
  State = 0; /*!< Reset State Value for next case statement usage*/
  ExitCritical(); /*!< Allow Interrupts */
}

void __attribute__ ((interrupt)) I2C_ISR(void)
{
  static ISRState = REGISTER_ADDRESS;
  static bool configureISR = true;
  static uint8_t dataCounter;
  ISRData = NumberOfBytes; /*!< Set case number to numofbytes */
  I2C0_S = I2C_S_IICIF_MASK; /* !< w1c, clearing the raised interrupt flag. */
  if(configureISR)
  {
    switch (ISRState)
    {
      case REGISTER_ADDRESS:
	I2C0_D = RegisterAddress;
	ISRState = REPEATED_START_CONDITION;

      case REPEATED_START_CONDITION:
	I2C0_C1 |= I2C_C1_RSTA_MASK; /*!< Repeat Start*/
	ISRState = DEVICE_ADDRESS_READ;

      case DEVICE_ADDRESS_READ:
	I2C0_D = (SlaveAddress << 1) | I2C_D_READ; /*!< Set Device Address and set to read as per MMA8451Q Data Sheet (pg. 19)  */
	I2C0_C1 &= ~(I2C_C1_TX_MASK); /*!< Receive mode  */
	I2C0_C1 &= ~(I2C_C1_TXAK_MASK);
	configureISR = false;
    }
  }
  if (I2C0_S & I2C_S_TCF_MASK) /*!< Check Transmission complete flag */
  {
    if (~(I2C0_C1 & I2C_C1_TX_MASK)) /*!< Check if it is receiving */
    { //Transmit
      if(!configureISR)
      {
	switch (ISRData)
	{
	  case (TX_LAST_BYTE):
	    I2C0_C1 |= I2C_C1_TXAK_MASK; /*!< Transmit Acknowledge Enable*/
	    RxData[dataCounter] = I2C0_D;
	    ISRData--;

	  case (LAST_BYTE):
	    I2C0_C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK); /*!<  disabling master and transmit */
	    ISRData--;
	    RxData[dataCounter] = I2C0_D;
	    dataCounter++;

	  case (COMPLETE):
	    configureISR = true;
	    ReadCompleteCallbackGlobal(ReadCompleteUserArgumentsGlobal); /*!< Run callback function */

	  default:
	    ISRData--;
	    RxData[dataCounter] = I2C0_D;
	    dataCounter++;
	  //  break;
	}
      }
    }
  }
}

/*! @brief Acknowledgment Check
 *
 *  Used to check whether acknowledgment has been successful
 *  @Return BOOL - TRUE if Acknowledgment successful.
 */
 bool WaitForAcknowledgment(void)
{
  uint16_t timer = 100000; /*!< Initiate local timer variable */
  while ((I2C0_S & ~I2C_S_IICIF_MASK) & (timer > 0)) /*!< Wait until either timer reaches 0 or IICIF bit raised*/
  {
    timer--; /* !< Decrement the timer */
  }
  if (I2C0_S & I2C_S_IICIF_MASK) /*!< Check IICIF bit raised */
  {
    return true; /*!< Acknowledgment successful */
  }
  else
  {
    return false; /*!< No acknowledgment */
  }
}

 /*! @brief Busy Check
  *
  *  Used to check whether I2C is currently busy.
  *  Waits until it is no longer busy and exits function/
  */
void WaitForIdle(void)
{
  while((I2C0_S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK) {}// Wait till bus is idle
}

/*!
* @}
*/

