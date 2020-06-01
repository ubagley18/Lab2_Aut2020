/*! @file packet.c
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the Simple Serial Communication Protocol.
 *
 *  @author Uldis Bagley and Prashant Shrestha
 *  @date 2020-04-1
 */

// New types
#include "packet.h"
#include "UART\UART.h"

//

// Packet structure

//uint8_t Packet_Command,    /*!< The packet's command */
//		Packet_Parameter1, /*!< The packet's 1st parameter */
//		Packet_Parameter2, /*!< The packet's 2nd parameter */
//		Packet_Parameter3, /*!< The packet's 3rd parameter */
//		Packet_Checksum;   /*!< The packet's checksum */

static uint8_t currentState = 0; // current state of FSM




bool Packet_Init(const uint32_t moduleClk, const uint32_t baudRate)
{

	return UART_Init(moduleClk, baudRate);
}


bool Packet_Get(void)
{

	//Finite State machine here

	switch (currentState)
	{
	case 0:
		if (UART_InChar(&Packet_Command)) // true if packet received from RxFIFO
			currentState = 1; // currentState value will be changed to 1 if true
		else
			return false;
		break;
	case 1:
		if (UART_InChar(&Packet_Parameter1))  // true if packet received from RxFIFO
			currentState = 2; //currentState value will be changed to 2 if true
		else
			return false;
		break;
	case 2:
		if (UART_InChar(&Packet_Parameter2))  // true if packet received from RxFIFO
			currentState  = 3; //currentState value will be changed to 3 if true
		else
			return false;
		break;
	case 3:
		if (UART_InChar(&Packet_Parameter3))  // true if packet received from RxFIFO
			currentState  = 4; //currentState value will be changed to 4 if true
		else
			return false;
		break;
	case 4:
		if (UART_InChar(&Packet_Checksum))  // true if packet received from RxFIFO
			currentState  = 5; //currentState value will be changed to 5 if true
		else
			return false;
		break;
	case 5:
		//checking packet validity
		if ((Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3) == Packet_Checksum){
			currentState = 0;  // packet received is valid, resetting state
			return true; // packet received
		}
		else
		{
			//shift packets
			Packet_Command = Packet_Parameter1; // Checksum does not add up, right shift bytes
			Packet_Parameter1 = Packet_Parameter2;
			Packet_Parameter2 = Packet_Parameter3;
			Packet_Parameter3 = Packet_Checksum;
			currentState = 4; // go to state 4 if packet not valid and look for another one
			return false;
		}
		break;

	}
	return false;
}


bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
		if ((UART_OutChar(command)) &&
			(UART_OutChar(parameter1)) &&
			(UART_OutChar(parameter2)) &&
			(UART_OutChar(parameter3)) &&
			// value of checksum XORed before sending data
			(UART_OutChar(command ^ parameter1 ^ parameter2 ^ parameter3 )));
		{
			return true;
		}

		return false;
}
