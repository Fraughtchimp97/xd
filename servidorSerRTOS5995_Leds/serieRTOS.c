/******************************************************
 * Rutinas para manejar el USCI_A0 en modo UART.
 * Se usa interupciones y una cola circular tanto para
 * la transmision como para la recepcion
 *
 * Hecho por Miguelangel Fraga Aguilar
 * Agosto de 2017
 */

#include <msp430.h>

#include <FreeRTOS.h>
#include <queue.h>

#include <serieRTOS.h>

/* Buffer de salida -- buffer circular FIFO de 128 bytes    */
QueueHandle_t xFilaRX,xFilaTX;

void configuraUART(void)
{

	//  ACLK = n/a, MCLK = SMCLK = BRCLK = default DCO = 8MHz
	//  Estas conecciones ya estan hechas en el LaunchPad
	//
	//                MSP430FR5969
	//                MSP430FR5994
	//             -----------------
	//       RST -|     P2.0/UCA0TXD|----|
	//            |                 |    |
	//           -|                 |    |
	//            |     P2.1/UCA0RXD|----|
	//            |                 |

	// Configura GPIO
	  P2SEL1 |= BIT0 | BIT1;                    // P2.0 y P2.1 asignados al USCI_A0 UART
	  P2SEL0 &= ~(BIT0 | BIT1);


	// Configure USCI_A0 for UART mode
	UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset
	UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK

	// Configura la velocidad de transmision a 9600bps
	// Baud Rate calculation
	// 8000000/(16*9600) = 52.083
	// Fractional portion = 0.083
	// User's Guide Table 21-4: UCBRSx = 0x049
	// UCBRFx = int ( (52.083-52)*16) = 1
	UCA0BR0 = 52;                             // 8000000/16/9600
	UCA0BR1 = 0x00;
	UCA0MCTLW = 0x4900 | UCOS16 | UCBRF_1;

	// Configura la velocidad de transmision a 115200bps
	//UCA0BR0 = 4;                             // 8000000/16/115200
	//UCA0BR1 = 0x00;
	//UCA0MCTLW = 0x5500|UCOS16 | UCBRF_5;


	UCA0CTLW0 &= ~UCSWRST;                    // Inicializa el eUSCI
	UCA0IE |= UCRXIE;
	xFilaTX=xQueueCreate(32,sizeof( unsigned char));
	xFilaRX=xQueueCreate(32,sizeof( unsigned char));
}


int serial_vacio(void)
{
   return(uxQueueMessagesWaiting( xFilaRX )==0);
}

unsigned char getserial() {

    unsigned char c;

    xQueueReceive( xFilaRX, &c, 0 );
	return (c);
}

void putserial(unsigned char val) {
	if(UCA0IFG & (UCTXIFG| UCTXCPTIFG)){ //Si el buffer de transmision esta vacio
		UCA0IFG &= ~UCTXCPTIFG;
		UCA0TXBUF = val;
	}else{   // sino almacena el caracter en la cola de transmision
	    xQueueSendToBack( xFilaTX, &val, 1 );
	}
	/* Se asegura que la interrupcion de transmision esta habilitada */
	UCA0IE |= UCTXIE;
}

//Funcion para enviar una cadena por el puerto serie
void wrcadserial(unsigned char *p)
{
    while(*p!='\0'){ //mientras no sea el final de cadena
      putserial(*p); //envia el caracter actual
      p++; //apunta al siguiente caracter
    }
}

//Rutina de atención a interrupciones del eUSCI en modo UART
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    BaseType_t pxHigherPriorityTaskWoken;
    unsigned char c;
	switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
	  {
	    case USCI_NONE: break;
	    case USCI_UART_UCRXIFG:
	        //coloca el caracter recibido en la fila
	                                  //de recepcion
	        c=UCA0RXBUF;
	        xQueueSendToBackFromISR(xFilaRX,&c,&pxHigherPriorityTaskWoken);

			//__bic_SR_register_on_exit( LPM0_bits ); //sale de modo LPM0
			//para que el ciclo principal pueda seguir ejecutandose
			//y procese el caracter recibido

	      break;
	    case USCI_UART_UCTXIFG:
			if ( uxQueueMessagesWaitingFromISR( xFilaTX )==0 ) {
			    //si la cola de transmision esta vacia
				// terminado -- desactiva la interrupcion del transmisor
				UCA0IE &= ~ UCTXIE;
			} else {
			    xQueueReceiveFromISR( xFilaTX, &c, &pxHigherPriorityTaskWoken );
				UCA0TXBUF=c;
			}
			break;
	    case USCI_UART_UCSTTIFG: break;
	    case USCI_UART_UCTXCPTIFG: break;
	  }
}

