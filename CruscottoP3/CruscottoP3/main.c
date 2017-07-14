#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "stlcd.h"


int packetcount=0;
//int displayupdater=20; //aggiorno il display meno frequentemente delle marce e giri per non sovraccaricare il processore e evitare l'effetto sfarfallio
char packet[]={0,0,0,0,0,0,0,0};

ISR (TIMER0_COMPA_vect)//interrupt per i led giri
{
		PORTA^=0b11000000;
		PORTC^=0b00111111;
		PORTD^=0b11000000;
		TCNT0 = 0;
}

ISR (INT0_vect)
{
	PORTC^=0b00100000;
	_delay_ms(1000);
	PORTC^=0b00100000;
}

void USART_Init( unsigned int baud )
{
	/* Set baud rate */
	UBRR0H = (unsigned char)(baud>>8);
	UBRR0L = (unsigned char)baud;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);
	/* Set frame format: 8data, 1stop bit */
	UCSR0C = ((1<<UCSZ01)|(1<<UCSZ00));
}

void timer0_init()
{
	// set up timer with prescaler = 1024
	TCCR0B |= (1 << CS02)|(1 << CS00);	
	// initialize counter
	TCNT0 = 0;
	OCR0A=240;
}

void DisplayGear(int gear)
{	
	PORTA &= 0b11000000;
	PORTC &= 0b00111111;
	
	switch (gear)
	{
		//resetto solo i led relativi alla marcia
		case 0:
		{
			PORTA |= 0b00010111;
			PORTC |= 0b11000000;

		}
		break;
		case 1:
		{
			PORTA |= 0b00010001;
		}
		break;
		case 2:
		{
			PORTA |= 0b00001011;
			PORTC |= 0b11000000;
		}
		break;
		case 3:
		{
			PORTA |= 0b00011011;
			PORTC |= 0b01000000;
		}
		break;
		case 4:
		{
			PORTA |= 0b00011101;
			PORTC |= 0b00000000;
		}
		break;
		case 5:
		{
			PORTA |= 0b00011110;
			PORTC |= 0b01000000;
		}
		break;
		case 6:
		{
			PORTA |= 0b00011110;
			PORTC |= 0b11000000;
		}
		break;
		case 7:
		{
			PORTA |= 0b00010011;
		}
		break;
		case 8:
		{
			PORTA |= 0b00011111;
			PORTC |= 0b11000000;
		}
		break;
		case 9:
		{
			PORTA |= 0b00011111;
			PORTC |= 0b01000000;
		}
		break;
		default://caso errore, mostra E
		{
			PORTA |= 0b00001110;
			PORTC |= 0b11000000;
		}
		break;
	}
}

void DisplayRPM(int RPM)
{	
	if(RPM>=6000)//stai al limitatore!!
	{	
		if(!(TIMSK0&(1<<OCIE0A)))
		{				
			PORTA|=0b11000000;
			PORTC|=0b00111111;
			PORTD|=0b11000000;		
			
			TCNT0 = 0;
			TIMSK0|=(1<<OCIE0A);//abilito l'interrupt lampeggio led giri
		}
	}
	else 
	{
		TIMSK0&=!(1<<OCIE0A);//disattivo l'interrupt led giri
		PORTA&=0b00111111;	//spengo i led RPM di PORTA
		PORTC&=0b11000000;	//spengo i led RPM di PORTC
		PORTD&=0b00111111;	//spengo i led RPM di PORTD
		
		if(RPM>=5600)
		{
			PORTA|=0b11000000;	
			PORTC|=0b00111111;	
			PORTD|=0b11000000;
		}
		else if(RPM>=5200)
		{
			PORTA|=0b10000000;
			PORTC|=0b00111111;	
			PORTD|=0b11000000;	
		}
		else if(RPM>=4800)
		{
			PORTC|=0b00111111;	
			PORTD|=0b11000000;	
		}
		else if(RPM>=4400)
		{
			PORTC|=0b00011111;
			PORTD|=0b11000000;
		}
		else if(RPM>=4000)
		{
			PORTC|=0b00001111;
			PORTD|=0b11000000;
		}
		else if(RPM>=3600)
		{
			PORTC|=0b00000111;
			PORTD|=0b11000000;
		}
		else if(RPM>=3200)
		{
			PORTC|=0b00000011;
			PORTD|=0b11000000;
		}
		else if(RPM>=2800)
		{
			PORTC|=0b00000001;
			PORTD|=0b11000000;
		}
		else if(RPM>=2400)
		{
			PORTD|=0b11000000;
		}
		else if(RPM>=2000)
		{
			PORTD|=0b01000000;
		}
	}
}

void PacketManager(char value)
{
	if(value==204)//204 è un magic number!!! serve per indicare una nuova trasmissione
	packetcount=0;
	else
	{
		packet[packetcount]=value;
		packetcount++;
	}
	
	if(packetcount>=8)//ho un pacchetto intero
	{
		DisplayRPM(packet[0]*100);	
		DisplayGear(packet[1]);
		//displayupdater++;
		//if(displayupdater>=20)
		//{
			display_update(packet[1],packet[3],packet[4],packet[5],packet[6],packet[7]);//aggiorno il matrice di punti + ho aggiunto packet[2] per la lambda
			//displayupdater=0;
		//}
		packetcount=0;
	}
}

int main(void)
{	
	DDRA=0b11111111;		//tutta PORTA è di output
	PORTA=0b00000000;	//tutte le uscite sono basse
	DDRC=0b11111111;		//tutta PORTC è di output
	PORTC=0b00000000;	//tutte le uscite sono basse
	DDRD=0b11111111;	//imposto tutta PORTD come uscita, tanto vengono sovrascritte da funzioni speciali (RS232 e external interrupt)
	PORTD=0b00000000;
	
	display_init();
	
	//devo mostrare la P di P3
	PORTA |= 0b00001111;
	PORTC |= 0b10000000;
	
	_delay_ms(1000);
	
	//mostra il 3 di P3
	DisplayGear(3);
	
	// initialize timer
	timer0_init();
	USART_Init(12);//12 è UBRR per 1Mhz a 4800 baud
	
	//servono per gli external interrupt, vedere perché non va!!!!
	//EICRA|=(1<<ISC01);
	//EIMSK|=(1<<INT0);
	sei();
	_delay_ms(1000);
	
	//resetto il display delle marce
	PORTA &= 0b11000000;
	PORTC &= 0b00111111;
	
    while (1)
    {
		while ( !(UCSR0A & (1<<RXC0)) );

		PacketManager(UDR0);
		
	    //for(int i=0;i<14;i++)
	    //{
		    //DisplayGear(i);
		    //DisplayRPM(2800+i*400);
		    //_delay_ms(1000);
	    //}
    }
}

