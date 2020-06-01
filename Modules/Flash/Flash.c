/*! @file Flash.c
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author Uldis Bagley and Prashant Shrestha
 *  @date 2020-04-10
 */

#include "Flash.h"
#include "fsl_common.h"
#include "fsl_port.h"


#define NB_ADDRESS_REG 3
#define NB_DATA_REG  8
#define BYTE 1
#define HALF_WORD 2
#define WORD 4

typedef struct
{
  uint8_t command;
  union
  {
    uint32_t combined;
    struct
    {
      uint8_t flashAddress0;
      uint8_t flashAddress1;
      uint8_t flashAddress2;
      uint8_t flashAddress3;
    } separate;
  } address;  // The first 4 FCCOB registers (0-3)
  union
  {
    uint64_t combined;
    struct
    {
      uint8_t dataByte0;
      uint8_t dataByte1;
      uint8_t dataByte2;
      uint8_t dataByte3;
      uint8_t dataByte4;
      uint8_t dataByte5;
      uint8_t dataByte6;
      uint8_t dataByte7;
    } separate;
  } data;  // FCCOB registers 4-B
} FCCOB_t;

// Private functions:

/*! @brief Launches command
 *
 *	@param commonCommandObject Pointer to FCCOB structure which holds address and data register values
 *  @return bool - TRUE if the command was launched successfully.
 */
static bool LaunchCommand(FCCOB_t* commonCommandObject);


/*! @brief Writes a phrase into Flash
 *
 *  @param address The address of the data.
 *  @param phrase The 64-bit data to write.
 *  @return bool - TRUE if the command was executed successfully.
 */
static bool WritePhrase(const uint32_t address, const uint64union_t phrase);


/*! @brief Erases a sector in Flash
 *
 *  @param address The address of the data.
 *  @return bool - TRUE if the command was executed successfully.
 */
static bool EraseSector(const uint32_t address);

/*! @brief Erases and writes a phrase into Flash
 *
 *  @param address The address of the data.
 *  @param phrase The 64-bit data to write.
 *  @return bool - TRUE if the command was executed successfully.
 */
static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase);

//representing the allocated flash memory
static uint8_t FlashMemory = 0xFF;
const uint8_t HALF_WORD_ALIGNED = 0;
const uint8_t PHRASE_ALIGNED = 0;
const uint8_t WORD_ALIGNED = 0;
const uint8_t WRITE = 0x07;
const uint8_t ERASE = 0x09;


bool Flash_Init(void)
{
	// Nothing to initialize
	return true;
}


static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase)
{
	//first erase flash, then write entire phrase
	if (Flash_Erase)
		return WritePhrase(address, phrase);
}


bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  static uint8_t memoryAlloc; //memory mask to allocate memory position
  static int variableAddress; //temporary variable address

  //check the size of the variable
  switch (size)
  {
    //assign potential memory depending on the size
    case 1:
      memoryAlloc = 0x01; // 0000 0001
      break;
    case 2:
      memoryAlloc = 0x03; // 0000 0011
      break;
    case 4:
      memoryAlloc = 0x0F; // 0000 1111
      break;
  }

  //loop through the address incrementing by variable size to find an unused memory
  for (variableAddress = FLASH_DATA_START; variableAddress <= FLASH_DATA_END; variableAddress += size)
  {
    //check if the flash is empty
    if (memoryAlloc == (FlashMemory & memoryAlloc))
    {
      //write the selected address to the address of the memory
      *variable = (void *) variableAddress;
      //change the current memory as used
      FlashMemory = (FlashMemory ^ memoryAlloc);
      return true;
    }
    //shift down the memoryAlloc until Flash Memory is empty
    memoryAlloc = memoryAlloc << size;
  }
  return false;
}


static bool LaunchCommand(FCCOB_t* commonCommandObject)
{
	// write parameters into registers with big endian notation
  FTFE->FCCOB0 = commonCommandObject->command;
  FTFE->FCCOB1 = commonCommandObject->address.separate.flashAddress2;
  FTFE->FCCOB2 = commonCommandObject->address.separate.flashAddress1;
  FTFE->FCCOB3 = commonCommandObject->address.separate.flashAddress0;
  FTFE->FCCOB4 = commonCommandObject->data.separate.dataByte3;
  FTFE->FCCOB5 = commonCommandObject->data.separate.dataByte2;
  FTFE->FCCOB6 = commonCommandObject->data.separate.dataByte1;
  FTFE->FCCOB7 = commonCommandObject->data.separate.dataByte0;
  FTFE->FCCOB8 = commonCommandObject->data.separate.dataByte7;
  FTFE->FCCOB9 = commonCommandObject->data.separate.dataByte6;
  FTFE->FCCOBA = commonCommandObject->data.separate.dataByte5;
  FTFE->FCCOBB = commonCommandObject->data.separate.dataByte4;

	FTFE->FSTAT = FTFE_FSTAT_CCIF_MASK; // clear the CCIF to launch the command

	// Wait for the CCIF to be set so a command can be executed
	while (FTFE->FSTAT != FTFE_FSTAT_CCIF_MASK);

	return true;
}


static bool WritePhrase(const uint32_t address, const uint64union_t phrase)
{
	FCCOB_t write;

	// Wait for the CCIF to be set so a command can be executed
	while (FTFE->FSTAT != FTFE_FSTAT_CCIF_MASK);

	// clear protection violation and access error flag if set
	if ((FTFE->FSTAT & FTFE_FSTAT_FPVIOL_MASK) == FTFE_FSTAT_FPVIOL_MASK)
		FTFE->FSTAT = FTFE_FSTAT_FPVIOL_MASK;
	if ((FTFE->FSTAT & FTFE_FSTAT_ACCERR_MASK) == FTFE_FSTAT_ACCERR_MASK)
			FTFE->FSTAT = FTFE_FSTAT_ACCERR_MASK;

	// Load FCCOB struct with values

	write.command = WRITE;
	write.address.combined = address;
	write.data.combined = phrase.l;

	return LaunchCommand(&write);
}


bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  uint64union_t phrase; //phrase union 64bit
  uint32_t newAddress = (uint32_t)address; //declare new address as 32bit

  //check if the address is aligned with phrase
  if (((newAddress/4) % 2) == PHRASE_ALIGNED)
  {
    phrase.s.Lo = data; //store data into low 32-bit of union
    phrase.s.Hi = _FW(newAddress+4); //read from flash into high 32-bit of union
    return ModifyPhrase(newAddress, phrase); //modify phrase to erase and write
  }
  else
  {
    phrase.s.Lo = _FW(newAddress-4); //read from flash into low 32-bit of union
    phrase.s.Hi = data; //store data into high 32-bit of union
    return ModifyPhrase(newAddress-4, phrase); //modify phrase to erase and write
  }
}

bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  uint32union_t word; //word union 32bit
  uint32_t newAddress = (uint32_t)address; //declare new address as 32bit

  //check if the address is aligned with word
  if((newAddress % 4) == WORD_ALIGNED)
  {
    word.s.Lo = data; //store data into low 16-bit of union
    word.s.Hi = _FH(newAddress+2); //read from flash into high 16-bit of union
    return Flash_Write32(&(_FW(newAddress)), word.l); //write to 32bit flash
  }
  else
  {
    word.s.Lo = _FH(newAddress-2); //read from flash into low 16-bit of union
    word.s.Hi = data; //store data into high 16-bit of union
    return Flash_Write32(&(_FW(newAddress-2)), word.l); //write to 32bit flash
  }
}

bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  uint16union_t halfword; //halfword union 16bit
  uint32_t newAddress = (uint32_t)address; //declare new address as 32bit

  //check if the address is aligned with halfword
  if ((newAddress % 2) == HALF_WORD_ALIGNED)
  {
    halfword.s.Lo = data; //store data into low byte of union
    halfword.s.Hi = _FB(newAddress+1); //read from flash into high byte of union
    return Flash_Write16(&(_FH(newAddress)), halfword.l); //write to 16bit flash
  }
  else
  {
    halfword.s.Lo = _FB(newAddress-1); //read from flash into low byte of union
    halfword.s.Hi = data;  //store data into high byte of union
    return Flash_Write16(&(_FH(newAddress-1)),halfword.l); //write to 16bit flash
  }
}



bool Flash_Erase(void)
{
	uint32_t sectorErase = FLASH_DATA_START;
	return EraseSector(sectorErase);
}

static bool EraseSector(const uint32_t address)
{
	FCCOB_t erase;

	erase.command = ERASE;
	//assign address to be erased
	erase.address.combined = address;
	return LaunchCommand(&erase);
}
