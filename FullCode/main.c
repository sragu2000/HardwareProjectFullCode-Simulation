#define F_CPU 8000000UL
#define SREG   _SFR_IO8(0x3f)
#include "headerfiles.h"
int main(void){
	DDRD=0xE8;
	PORTD=0x04;
	LCD_Init();LCD_String("Initializing..");_delay_ms(50);LCD_Clear();
	I2C_Init();
	PWM_init();
	GGA_Index=0;
	USART_Init(9600);
	sei();
	LCD_String("Welcome");
	_delay_ms(100); //actual delay is 3000ms
	LCD_Clear();
	while (1){

		//flame detection
		if(pinRead(PINC,flame)==0x10){
			LCD_Clear();
			LCD_String("Flame Detected !");
			ringAlarm();
			onHazardLight();
			_delay_ms(500);
			stopAlarm();
			offHazardLight();
			LCD_Clear();
			sendLocation("Flame is Detected");
		}
		else{
			ADC_Init();
			int pressure = ADC_Read(1);
			if (pressure > 109){// if value gt 109 vehicle is moving in proteus 21
				int val=ADC_Read(0);//acce
				float speed=(val/1024.0)*255.0;//adc->pwm
				OCR0=(int)speed;//set speed
				LCD_Clear();
				LCD_String("Driving mode");
				if(pinRead(PINC,alcohol)==0x20){
					LCD_Clear();
					LCD_String("Alcohol Detected");
					ringAlarm();
					_delay_ms(100);
					stopAlarm();
					LCD_Clear();
					sendLocation("Alcohol Detected");
				}else{
					if(isDriverSleepingIR()&& (pinRead(PINC,0)==0x01)){
						LCD_Clear();
						LCD_String("Sleeping");
						ringAlarm();
						onHazardLight();
						LCD_Command(0xc0);
						LCD_String("Waiting..");
						GICR = 1<<INT0;		/* Enable INT0*/
						MCUCR = 1<<ISC01 | 1<<ISC00;  /* Trigger INT0 on rising edge */
						sei();			/* Enable Global Interrupt */
						_delay_ms(500); //actual delay is 5000ms
						LCD_Clear();
						LCD_String("Waiting");
						LCD_Command(0xc0);
						LCD_String("Complete");
						offHazardLight();
						stopAlarm();
						OCR0=0;
						LCD_Clear();
						LCD_String("Speed is");
						LCD_Command(0xc0);
						LCD_String("Reducing..");
						sendLocation("Driver is Sleeping");
						playRadio();
						while(1){}//let motor to slowdown fully
					}
				}
			}else{
				LCD_String("Vehicle is");
				LCD_Command(0xc0);
				LCD_String("not moving");
				_delay_ms(50);
				LCD_Clear();
			}
		}//else 1 end
	}//while end
}//main end


void ringAlarm(){
	portHigh(PORTD,buzzer);//buzzer
}
void stopAlarm(){
	portLow(PORTD,buzzer);
}
void onHazardLight(){
	portHigh(PORTD,led);
}
void offHazardLight(){
	portLow(PORTD,led);
}
void playRadio(){
	portHigh(PORTD,musicSystem);
}
int isDriverSleepingIR(){
	LCD_Clear();
	LCD_String("Checking Eyes");
	int timeInterval=5;
	int flag=0;
	for(int i=1;i<=timeInterval;i++){
		if(pinRead(PINC,irsensor)==0x40){
			flag++;
		}else{
			flag--;
		}
		_delay_ms(50);
	}
	LCD_Command(0xc0);
	LCD_String("  -Finished");
	if(flag==timeInterval){
		return 1;
	}else{
		return 0;
	}
}

void sendLocation(char* message){
	get_latitude(GGA_Pointers[0]);char* lat=lat_degrees_buffer;
	get_longitude(GGA_Pointers[2]);char* lngtd=long_degrees_buffer;
	get_altitude(GGA_Pointers[7]);char* altitude=Altitude_Buffer;
	PORTD=0x08;// change signal using mux
	sendMessage(message,lat,lngtd,altitude);
	PORTD=0x00;// turn back to normal
}

ISR(INT0_vect)
{
	LCD_Clear();
	portLow(PORTD,6);
	portLow(PORTD,7);
	_delay_ms(50);
	main();
}