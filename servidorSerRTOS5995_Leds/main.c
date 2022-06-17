
/******************************************************************************
 * *Ejemplo de servidor de puerto serie con FreeRTOS
 *
 * Este ejemplo espera recibir comandos por medio del puerto serie y los envia
 * otras tareas para que sean ejecutados
 *
 * Incluye una pantalla LED multiplexada de 7 segmentos
 * La pantalla se conecta - anodos de los segmentos a P3.0 - P3.7,
 * catodos de los digitos a P5.0 - P5.2
 *
 * Hecho por Miguelangel Fraga Aguilar
 * Mayo de 2022
 */

// mp430.h se incluye en las cabeceras del port
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

#include <ctype.h>
#include <stdio.h>

#include "driverlib.h"
#include "serieRTOS.h"


#define pxPort_MS_TICS(MS) ( MS / portTICK_PERIOD_MS )

/*Cabecera de la rutina de configuracion de hardware para la aplicacion*/
static void prvSetupHardware(void);

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

//Cabecera de las funciones
void initHW();
char explora();
int digito(int d);

//Constantes globales
const unsigned char tabla7s[17]={0x3f,0x06,0x5b,0x4f,0x66,
0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x7b,0x71,0};

//Variables globales
int digitos[3]={0,16,16};

//Semaforo binario para la interrupcion
SemaphoreHandle_t xBinarySemaphore;
//Cola para comunicar los eventos de teclado
QueueHandle_t xFilaTeclado;

/* The heap is allocated here so the "persistent" qualifier can be used.  This
requires configAPPLICATION_ALLOCATED_HEAP to be set to 1 in FreeRTOSConfig.h.
See http://www.freertos.org/a00111.html for more information. */
#ifdef __ICC430__
    __persistent                    /* IAR version. */
#else
    #pragma PERSISTENT( ucHeap );   /* CCS version. */
#endif
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ] = { 0 };

    int PORCENTAJE(float porcentaje)
    {

        porcentaje = porcentaje *(9.9);
        return porcentaje;
    }

    int BRILLO(int brillo)
    {
        brillo=brillo%100;

        return brillo;
    }



    int LED(int led)
    {
        led=led/100;

        return led;
    }
int digito(int d)
{
    switch(d)
                {
                    case 'A':d=10;
                        break;
                    case 'B':d=11;
                        break;
                    case 'C':d=12;
                        break;
                    case 'D':d=13;
                        break;
                    case 'E':d=14;
                        break;

                    case 'F':d=15;
                            break;
                    case 'G':d=16;
                            break;
                    case 'H':d=17;
                            break;
                    case 'I':d=18;
                            break;
                    case 'J':d=19;
                            break;
                    case 'K':d=20;
                        break;
                    case 'L':d=21;
                        break;
                    case 'M':d=22;
                        break;
                    case 'N':d=23;
                        break;
                    case 'O':d=24;
                        break;




                }
    return d;
}


/*rutina para configrar los puertos*/
void initHW()
{
    // El teclado se conecta - Renglones (entradas con pull-up) a P6.0-P6.3
         // columnas (salidas en uno) a P8.0 - P8.3
        P1DIR=0xFF;
        P1OUT=0;
        P1SEL0 |= BIT2 | BIT3 | BIT5;                  // P1.2 and P1.3 options select
        P1SEL1 &= ~(BIT2 | BIT3 | BIT5);
        //P1OUT|=BIT2;

        P2DIR=0xFF;
            P2OUT=0;
            P2SEL0 |= BIT6;                  // P1.2 and P1.3 options select
            P2SEL1 &= ~(BIT6);





            P4DIR=0xFF;
            P4OUT=0;
            P4SEL0 |= BIT4;                  // P1.2 and P1.3 options select
            P4SEL1 &= ~(BIT4);




        P6DIR&=0xF0;
        P6OUT|=0x0F;
        P6REN|=0x0F;

        P8DIR|=0x0F;
        P8OUT|=0x0F;
         // La pantalla se conecta - anodos de los segmentos a P3.0 - P3.7,
         // catodos de los digitos a P5.0 - P5.2
        P3DIR=0xFF;
        P3OUT=0;
        P5DIR|=0x07;
        P5OUT|=0x07;

        P5SEL0 |= BIT7;                  // P1.2 and P1.3 options select
        P5SEL1 &= ~(BIT7);

    //Exhibe el digito 0
    P3OUT=tabla7s[digitos[0]];
}


/*rutina para explorar un teclado matricial e indicar si una tecla esta
  presionada y que tecla es la que se presiona*/
char explora(){
    unsigned char columna,renglon,tecla,mascara,mascaraCol;

    tecla=0;
    P8OUT|=0x0f;        //*****Pone todas las columnas en 1****
    mascaraCol=1;
    for(columna=0;columna<4;columna++){
        P8OUT&=(~mascaraCol);//habilita la columna actual
        mascara=1;
        for(renglon=0;renglon<4;renglon++){
            if((P6IN&mascara)==0){//si la tecla esta presionada
                tecla=columna*4+renglon;
                tecla|=0x80;//Indica que si hay una tecla presionada
            }
            mascara<<=1;//Revisa la siguiente entrada
        }
        P8OUT|=mascaraCol;//deshabilita la columna actual
        mascaraCol<<=1;
    }
    return(tecla);
}





void vTareaPantalla( void *pvParameters ){
    TickType_t xLastWakeTime;

    int digito=0,mascara=0x01;

    (void) pvParameters; //Para evitar advertencia del compilador sobre parametro no usado
    xLastWakeTime = xTaskGetTickCount(); //obtiene el tiempo actual para hacer el evento periodico
    for(;;){
        //Muestra el digito actual por 10mS
        vTaskDelayUntil( &xLastWakeTime, pxPort_MS_TICS(10) );
        //Desactiva el digito actual
        P5OUT|=mascara;

        //Exhibe el sigiuiente digito
        digito++;
        mascara<<=1;
        if(digito>2){
            digito=0;
            mascara=0x01;
        }

        unsigned char temp=tabla7s[digitos[digito]];
        P3OUT=temp;

        P5OUT&=(~mascara);

    }

}


//Tarea de interfaz
void vTareaServidor( void *pvParameters){
    //int indice=2;

    int c=0,d=0,f=0;


#define CALADC12_12V_30C  *((unsigned int *)0x1A1A)   // Temperature Sensor Calibration-30 C
                                                      //See device datasheet for TLV table memory mapping
#define CALADC12_12V_85C  *((unsigned int *)0x1A1C)   // Temperature Sensor Calibration-85 C

    configuraUART();

    // Initialize the shared reference module
    // By default, REFMSTR=1 => REFCTL is used to configure the internal reference
    while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON;             // Enable internal 1.2V reference

    /* Initialize ADC12_A */
    ADC12CTL0 &= ~ADC12ENC;                   // Disable ADC12
    ADC12CTL0 = ADC12SHT0_8 + ADC12ON;        // Set sample time
    ADC12CTL1 = ADC12SHP;                     // Enable sample timer
    ADC12CTL3 = ADC12TCMAP;                   // Enable internal temperature sensor
    ADC12MCTL0 = ADC12VRSEL_1 + ADC12INCH_30; // ADC input ch A30 => temp sense
    //ADC12IER0 = 0x001;                        // ADC_IFG upon conv result-ADCMEMO

    while(!(REFCTL0 & REFGENRDY));            // Wait for reference generator
                                            // to settle
    ADC12CTL0 |= ADC12ENC;
    wrcadserial("Servidor Serie\n\r");
    for(;;){
        //espera indefinidamente a recibir una byte del puerto serie
        if(!serial_vacio()){
            c=getserial();
            if((int)c>64)
            {
                d=digito(c);
            }
            else
            {
                d=c-48;
            }
            d=d*25;




            if(c!=0)
            {
               while(serial_vacio())vTaskDelay(1);
               c=getserial();
            }

            if((int)c>64)
            {
                f=digito(c);
            }
            else
            {
                f=c-48;
            }
            f=f+d;
            fflush(stdin);
            int Led=LED(f);
            int brillo=BRILLO(f);
            float porcentaje=PORCENTAJE(brillo);


           switch(Led)
            {
            case 0: TA1CCR1=porcentaje;
                    TA1CCR2=porcentaje;
                    TB0CCR2=porcentaje;
                    TB0CCR5=porcentaje;
                    TB0CCR1=porcentaje;
                break;
            case 1:TA1CCR1=porcentaje;
                break;
            case 2:TA1CCR2=porcentaje;
                break;
            case 3:TB0CCR2=porcentaje;
                break;
            case 4: TB0CCR5 = porcentaje;
                break;
            case 5:TB0CCR1 = porcentaje;
                break;
            }



            if('t'==tolower(c)){
                            //  Description: A single sample is made on A10 with internal reference voltage
                            //  1.2V. Software manually sets ADC12SC to start sample and conversion and
                            //  automatically cleared at EOC. It uses ADC12OSC to convert the sameple.
                            //  The Mainloop sleeps the MSP430 in LPM4 to save power until ADC conversion
                            //  is completed. ADC12_ISR forces exit from LPMx in on exit from interrupt
                            //  handler so that the mainloop can execute and calculate oC and oF.
                            //  ACLK = n/a, MCLK = SMCLK = default DCO ~ 1.045MHz, ADC12CLK = ADC12OSC
                            //
                            //  Un-calibrated temperature measured from device to device will vary due to
                            //  slope and offset variance from device to device - please see datasheet.
                            //  Note: This example uses the TLV calibrated temperature to calculate
                            //  the temperature
                            // (the TLV CALIBRATED DATA IS STORED IN THE INFORMATION SEGMENT, SEE DEVICE DATASHEET)
                            //

                            long temperatureDegC; //en decimas de grado


                            ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start

                            //Espera el final de la conversion
                            while(ADC12IFGR0&ADC12IFG0==0);
                            ADC12IFGR0&=~ADC12IFG0;//apaga la bandera de interrupcion
                            // Temperature in Celsius. See the Device Descriptor Table section in the
                            // System Resets, Interrupts, and Operating Modes, System Control Module
                            // chapter in the device user's guide for background information on the
                            // used formula.
                            temperatureDegC = (((long)ADC12MEM0 - CALADC12_12V_30C) * (85 - 30)*10) /
                                    (CALADC12_12V_85C - CALADC12_12V_30C) + 300;
                            char cadena[10];
                            sprintf(cadena,"%ld,",temperatureDegC);
                            wrcadserial((unsigned char *)cadena);
                        }

        }
    }
}

/*-----------------------------------------------------------*/

int main( void )
{
    /* See http://www.FreeRTOS.org/MSP430FR5969_Free_RTOS_Demo.html */

    /* Configure the hardware ready to run the demo. */
    prvSetupHardware();
    initHW();

    xFilaTeclado=xQueueCreate(16,sizeof(char));
    xBinarySemaphore = xSemaphoreCreateBinary();
    if(xFilaTeclado!=NULL)  {
        xTaskCreate(
            vTareaPantalla
            ,  (const portCHAR *)"Pantalla" //Nombre
            ,  256      //Tamanio de la pila
            ,  NULL
            ,  4        //Prioridad
            ,  NULL ); //
        xTaskCreate(
            vTareaServidor
            ,  (const portCHAR *)"Servidor" //Nombre
            ,  256      //Tamanio de la pila
            ,  NULL
            ,  3        //Prioridad
            ,  NULL ); //


    }
    //arranca el agendador
    vTaskStartScheduler();
    //este codigo no debe ejecutarse
    for(;;);

    //return 0;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.
    See http://www.freertos.org/Stacks-and-stack-overflow-checking.html */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    __bis_SR_register( LPM4_bits + GIE );
    __no_operation();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
    #if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0 )
    {
        /* Call the periodic event group from ISR demo. */
        //vPeriodicEventGroupsProcessing();

        /* Call the code that 'gives' a task notification from an ISR. */
        //xNotifyTaskFromISR();
    }
    #endif
}
/*-----------------------------------------------------------*/

/* The MSP430X port uses this callback function to configure its tick interrupt.
This allows the application to choose the tick interrupt source.
configTICK_VECTOR must also be set in FreeRTOSConfig.h to the correct
interrupt vector for the chosen tick interrupt source.  This implementation of
vApplicationSetupTimerInterrupt() generates the tick from timer A0, so in this
case configTICK_VECTOR is set to TIMER0_A0_VECTOR. */
void vApplicationSetupTimerInterrupt( void )
{
const unsigned short usACLK_Frequency_Hz = 32768;

    /* Ensure the timer is stopped. */
    TA0CTL = 0;

    /* Run the timer from the ACLK. */
    TA0CTL = TASSEL_1;

    /* Clear everything to start with. */
    TA0CTL |= TACLR;

    /* Set the compare match value according to the tick rate we want. */
    TA0CCR0 = usACLK_Frequency_Hz / configTICK_RATE_HZ;
    TA0CCR1=usACLK_Frequency_Hz;

    /* Enable the interrupts. */
    TA0CCTL0 = CCIE;

    /* Start up clean. */
    TA0CTL |= TACLR;

    /* Up mode. */
    TA0CTL |= MC_1;

    TA1CCR0 = 1000-1;                       // PWM Period
                TA1CCTL1 = OUTMOD_7;                    // CCR1 reset/set       1.2
                TA1CCR1 = 0;                          // CCR1 PWM duty cycle

                TA1CCTL2 = OUTMOD_7;                    // CCR2 reset/set          1.3
                TA1CCR2 = 0;                          // CCR2 PWM duty cycle



                TA1CTL = TASSEL__SMCLK | MC__UP | TACLR;// SMCLK, up mode, clear TAR


        TB0CCR0 = 1000-1;                       // PWM Period
                TB0CCTL1 = OUTMOD_7;                    // CCR1 reset/set      2.6
                TB0CCR1 = 0;                          // CCR1 PWM duty cycle

                TB0CCTL2 = OUTMOD_7;                    // CCR1 reset/set     1.5
                TB0CCR2 = 0;  // CCR2 PWM duty cycle


                TB0CCTL5 = OUTMOD_7;                    // CCR1 reset/set     4.4
                TB0CCR5 = 0;  // CCR2 PWM duty cycle

                TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;// SMCLK, up mode, clear TAR
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

    /* Set all GPIO pins to output and low. */
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setOutputLowOnPin( GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setOutputLowOnPin( GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setOutputLowOnPin( GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN8 | GPIO_PIN9 | GPIO_PIN10 | GPIO_PIN11 | GPIO_PIN12 | GPIO_PIN13 | GPIO_PIN14 | GPIO_PIN15 );
    GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setAsOutputPin( GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setAsOutputPin( GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 );
    GPIO_setAsOutputPin( GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN8 | GPIO_PIN9 | GPIO_PIN10 | GPIO_PIN11 | GPIO_PIN12 | GPIO_PIN13 | GPIO_PIN14 | GPIO_PIN15 );

    /* Configure P2.0 - UCA0TXD and P2.1 - UCA0RXD. */
    GPIO_setOutputLowOnPin( GPIO_PORT_P2, GPIO_PIN0 );
    GPIO_setAsOutputPin( GPIO_PORT_P2, GPIO_PIN0 );
    GPIO_setAsPeripheralModuleFunctionInputPin( GPIO_PORT_P2, GPIO_PIN1, GPIO_SECONDARY_MODULE_FUNCTION );
    GPIO_setAsPeripheralModuleFunctionOutputPin( GPIO_PORT_P2, GPIO_PIN0, GPIO_SECONDARY_MODULE_FUNCTION );

    /* Set PJ.4 and PJ.5 for LFXT. */
    GPIO_setAsPeripheralModuleFunctionInputPin(  GPIO_PORT_PJ, GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION  );

    /* Set DCO frequency to 8 MHz. */
    CS_setDCOFreq( CS_DCORSEL_0, CS_DCOFSEL_6 );

    /* Set external clock frequency to 32.768 KHz. */
    CS_setExternalClockSource( 32768, 0 );

    /* Set ACLK = LFXT. */
    CS_initClockSignal( CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Set SMCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Set MCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Start XT1 with no time out. */
    CS_turnOnLFXT( CS_LFXT_DRIVE_0 );

    /* Disable the GPIO power-on default high-impedance mode. */
    PMM_unlockLPM5();
}
/*-----------------------------------------------------------*/

int _system_pre_init( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

    /* Return 1 for segments to be initialised. */
    return 1;
}


// Port 6 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT6_VECTOR
__interrupt void Port_6(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT6_VECTOR))) Port_6 (void)
#else
#error Compiler not supported!
#endif
{
  BaseType_t xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;
  __bic_SR_register_on_exit(LPM4_bits);     // Exit LPM4
  P6IFG &= 0xf0;                           // Borra P6.0 a P6.3 IFG
  P6IE &= 0xf0;                           // Dehabilita interrupciones P6.0 a P6.3
  xSemaphoreGiveFromISR( xBinarySemaphore, &xHigherPriorityTaskWoken );
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
