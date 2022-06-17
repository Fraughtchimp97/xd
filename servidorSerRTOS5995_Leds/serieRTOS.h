#ifndef _serieRTOS_
#define _serieRTOS_

//Funcion de configuracion del UART
void configuraUART(void);

//Funcion que regresa verdadero si la cola de recepcion esta vacio
//falso si hay un caracter por leer
int serial_vacio(void);

//Funcion que lee un caracter de la cola de recepcion
unsigned char getserial(void);

//Funcion que envia un caracter por el puerto serie
//En caso de que este ocupado lo deja en la cola de
//transmision
void putserial(unsigned char val);

//Funcion para enviar una cadena por el puerto serie
void wrcadserial(unsigned char *p);

#endif

