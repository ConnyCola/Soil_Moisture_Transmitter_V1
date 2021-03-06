#include "mrfi.h"
#include "serial/serial.h"
#include "typedefs.h"
#include "defines.h"


#define msg_ON  0x02
#define BROADCAST 254

//#define _DEMO

mrfiPacket_t packet;
mrfiPacket_t packetreceived;
CMD cmd;
char node_ID = 10;

void main (void){
	WDTCTL = WDTPW+WDTHOLD;		// Stop watchdog timer
	P1OUT  = BIT0;				// P1.0 input+pullup
	P1REN |= BIT0;				// P1.0 input+pullup
    P1IE  |= BIT0;				// P1.0 interrupt enable
    P1IES |= BIT0;				// P1.0 Hi/Lo edge
    P1IFG &= ~BIT0;				// P1.0 IFG Flag cleared

	BSP_Init();
	MRFI_Init();				// Init SPI com with CC2500
	MRFI_SetRFPwr(2);			// RF transmitting power (0 to 2)
	mrfiRadioInterfaceWriteReg(CHANNR,180);	// set channel number

	MRFI_WakeUp();				// wake up the radio
	MRFI_RxOn(); 				// turn into Rx mode

	initSerial();				// init Serial interface
	//printf("\r\n\r\n!!!      Transmitter      !!!\r\n\r\n");

	__bis_SR_register(LPM0_bits + GIE);    // Enter LPM0, interrupts enabled
}

void MRFI_RxCompleteISR_new(){
	// in Components/mrfi/radios/family5/mrfi_radio.c
	//Receive Data from PC UART
	MRFI_Receive(&packetreceived);
	int read_ID = (packetreceived.frame[9] - '0') * 10;
	    read_ID += (packetreceived.frame[10] - '0');

#ifndef _DEMO
	if (read_ID == node_ID){
		CMD cmd;
		cmd.cmd = packetreceived.frame[11];
		//cmd.val1 = packetreceived.frame[11];
		//cmd.val2 = packetreceived.frame[12];
		if (cmd.cmd == CMD_RSSI){
			packetreceived.frame[0] = 16;
			while(MRFI_TX_RESULT_SUCCESS!=MRFI_Transmit(&packetreceived, MRFI_TX_TYPE_FORCED));
		}else
			send_short_CMD(cmd);
	}
	else if(read_ID == BROADCAST){

	}

#else
	if(read_ID != node_ID){
		switch (cmd.cmd) {
			case CMD_MOIS: //A
				packetreceived.frame[0]= 16;
				packetreceived.frame[12]=' ';
				packetreceived.frame[13]=' ';
				packetreceived.frame[14]='0'+(rand()%9)+1;
				packetreceived.frame[15]='0'+(rand()%9);
				while(MRFI_TX_RESULT_SUCCESS!=MRFI_Transmit(&packetreceived, MRFI_TX_TYPE_FORCED));

				break;
			case CMD_VOLT: //B
				break;
			case CMD_MIN: //C
				break;
			case CMD_MAX: //D
				break;
			case CMD_CALI: //E
				break;
			case CMD_DRY:  //F
				break;
			case CMD_WET:  //G
				break;
			case CMD_FIN:  //H
				break;
			case CMD_TEST: //I
				break;
			case CMD_VERS: //J
				break;
			case CMD_RSSI: //K
				packetreceived.frame[0] = 16;
				while(MRFI_TX_RESULT_SUCCESS!=MRFI_Transmit(&packetreceived, MRFI_TX_TYPE_FORCED));
				break;
			default:
				cmd.cmd = CMD_ERROR;
				break;
			}
	}

#endif
		//send_medium_CMD(node_ID,cmd);
}


#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void){
	// Data back from Sensor

	static int count = 0;
	static int error = 0;
	int offset = 9 + 2;
	char in_key = UCA0RXBUF;

	if(in_key == 0x0A) // '\n'
	{	packet.frame[offset -2] = (node_ID/10) + '0';
		packet.frame[offset -1] = (node_ID%10) + '0';
		if(!error)
			while(MRFI_TX_RESULT_SUCCESS!=MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED));
		count = 0;
		error = 0;
	}

	else if(((in_key >= '0')&&(in_key <= '9'))||((in_key >='A')&&(in_key <='Z'))||(in_key == ' ')||(in_key == 0x0A)){
		packet.frame[offset + count] = in_key;
		packet.frame[0] = offset + count;
		count++;
	}
	else{
		error = 1;
	}

}


#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= ~BIT0;				// P1.0 IFG Flag cleared
	WDTCTL &= ~WDTHOLD;			// Start watchdog timer
	while(1);					// will reset the programm
}
