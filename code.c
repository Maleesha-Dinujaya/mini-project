// CONFIG
#pragma config FOSC = HS       // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = OFF        // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
//End of CONFIG registers

#define _XTAL_FREQ 20000000
#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7

#define SIM900_OK 1
#define SIM900_READY 2
#define SIM900_FAIL 3
#define SIM900_RING 4
#define SIM900_NC 5
#define SIM900_UNLINK 6

#include<xc.h>

// Wait for any response on the input
inline unsigned char _SIM900_waitResponse(void);
    int recv;
    char p =1;

//LCD Functions Developed by Circuit Digest.
void Lcd_SetBit(char data_bit) //Based on the Hex value Set the Bits of the Data Lines
{
if(data_bit& 1) 
    D4 = 1;
else
    D4 = 0;
if(data_bit& 2)
    D5 = 1;
else
    D5 = 0;
if(data_bit& 4)
    D6 = 1;
else
    D6 = 0;
if(data_bit& 8) 
    D7 = 1;
else
    D7 = 0;
}

void Lcd_Cmd(char a)
{
RS = 0;           
Lcd_SetBit(a); //Incoming Hex value
EN  = 1;         
        __delay_ms(4);
        EN  = 0;         
}

void Lcd_Clear()
{
Lcd_Cmd(0); //Clear the LCD
Lcd_Cmd(1); //Move the curser to first position
}

void Lcd_Set_Cursor(char a, char b)
{
char temp,z,y;
if(a== 1)
    {
     temp = 0x80 + b - 1; //80H is used to move the curser
    z = temp>>4; //Lower 8-bits
    y = temp & 0x0F; //Upper 8-bits
    Lcd_Cmd(z); //Set Row
    Lcd_Cmd(y); //Set Column
    }

else if(a== 2)
    {
    temp = 0xC0 + b - 1;
    z = temp>>4; //Lower 8-bits
    y = temp & 0x0F; //Upper 8-bits
    Lcd_Cmd(z); //Set Row
    Lcd_Cmd(y); //Set Column
    }
}

void Lcd_Start()
{
  Lcd_SetBit(0x00);
  for(int i=1065244; i<=0; i--)  NOP();  
  Lcd_Cmd(0x03);
__delay_ms(5);
  Lcd_Cmd(0x03);
__delay_ms(11);
  Lcd_Cmd(0x03); 
  Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
  Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
  Lcd_Cmd(0x08); //Select Row 1
  Lcd_Cmd(0x00); //Clear Row 1 Display
  Lcd_Cmd(0x0C); //Select Row 2
  Lcd_Cmd(0x00); //Clear Row 2 Display
  Lcd_Cmd(0x06);
}

void Lcd_Print_Char(char data)  //Send 8-bits through 4-bit mode
{
   char Lower_Nibble,Upper_Nibble;
   Lower_Nibble = data&0x0F;
   Upper_Nibble = data&0xF0;
   RS = 1;             // => RS = 1
   Lcd_SetBit(Upper_Nibble>>4);             //Send upper half by shifting by 4
   EN = 1;
   for(int i=2130483; i<=0; i--)  NOP(); 
   EN = 0;
   Lcd_SetBit(Lower_Nibble); //Send Lower half
   EN = 1;
   for(int i=2130483; i<=0; i--)  NOP();
   EN = 0;
}

void Lcd_Print_String(char *a)
{
int i;
for(i=0;a[i]!='\0';i++)
  Lcd_Print_Char(a[i]);  //Split the string using pointers and call the Char function 
}

/****End of LCD Functions****/

void _delay_ms(unsigned int milliseconds) {
    unsigned int i, j;
    for (i = 0; i < milliseconds; i++)
        for (j = 0; j < 165; j++); // Adjust this loop for proper delay based on your system
}

//**Initialize UART for SIM900*//
void Initialize_SIM900(void)
{
    //***Setting I/O pins for UART***//
    TRISC6 = 0; // TX Pin set as output
    TRISC7 = 1; // RX Pin set as input
    //________I/O pins set __________//

    /**Initialize SPBRG register for required 
    baud rate and set BRGH for fast baud_rate**/
    SPBRG = 129; //SIM900 operates at 9600 Baud rate so 129
    BRGH  = 1;  // for high baud_rate
    //________End of baud_rate setting________//

    //***Enable Asynchronous serial port******//
    SYNC  = 0;    // Asynchronous
    SPEN  = 1;    // Enable serial port pins
    //____Asynchronous serial port enabled______//
    //*Lets prepare for transmission & reception*//
    TXEN  = 1;    // enable transmission
    CREN  = 1;    // enable reception
    //_UART module up and ready for transmission and reception_//
    //*Select 8-bit mode*//  
    TX9   = 0;    // 8-bit reception selected
    RX9   = 0;    // 8-bit reception mode selected
    //_8-bit mode selected_//     
}

//_______UART module Initialized_________//
//*Function to send one byte of date to UART*//

void _SIM900_putch(char bt)  
{
    while(!TXIF);  // hold the program till TX buffer is free
    TXREG = bt; //Load the transmitter buffer with the received value
}

//____________End of function_______________//
//*Function to get one byte of date from UART*//

char _SIM900_getch()   
{
    if(OERR) // check for Error 
    {
        CREN = 0; //If error -> Reset 
        CREN = 1; //If error -> Reset 
    }
    
    while(!RCIF);  // hold the program till RX buffer is free
    return RCREG; //receive the value and send it to main function
}

//____________End of function_______________//
//*Function to convert string to byte*//

void SIM900_send_string(char* st_pt)
{
    while(*st_pt) //if there is a char
        _SIM900_putch(*st_pt++); //process it as a byte data
}

//__________End of function_____________//
//*End of modified Codes*//

void SIM900_print(const char *ptr) {
    while (*ptr != '\0') {
        _SIM900_putch(*ptr++);
    }
}

int SIM900_isStarted(void) {
    SIM900_print("AT\r\n");
    return (_SIM900_waitResponse() == SIM900_OK);
}

int SIM900_isReady(void) {
    SIM900_print("AT+CPIN?\r\n");
    return (_SIM900_waitResponse() == SIM900_READY);
}

inline unsigned char _SIM900_waitResponse(void) {
    unsigned char so_far[6] = {0,0,0,0,0,0};
    unsigned const char lengths[6] = {2,12,5,4,6,6};
    unsigned const char* strings[6] = {"OK", "+CPIN: READY", "ERROR", "RING", "NO CARRIER", "Unlink"};
    unsigned const char responses[6] = {SIM900_OK, SIM900_READY, SIM900_FAIL, SIM900_RING, SIM900_NC, SIM900_UNLINK};
    unsigned char received;
    unsigned char response;
    char continue_loop = 1;
    while (continue_loop) {
        received = _SIM900_getch();
        for (unsigned char i = 0; i < 6; i++) {
            if (strings[i][so_far[i]] == received) {
                so_far[i]++;
                if (so_far[i] == lengths[i]) {
                    response = responses[i];
                    continue_loop = 0;
                }
            } else {
                so_far[i] = 0;
            }
        }
    }
    return response;
}

void main(void)
{   
    //I/O Declarations//
    TRISD = 0x00;  //LCD pins on port D as output
    //End of I/O declaration//
    Lcd_Start();   //Initialize LCD 
    Initialize_SIM900();//lets get our Serial ready for action

    Lcd_Set_Cursor(1,1);
    Lcd_Print_String("SIM900 & PIC");
       /Check if the SIM900 communication is successful/
    // Clear LCD
    Lcd_Clear();

    // Check SIM900 communication
    do {
        Lcd_Set_Cursor(1, 1);
        Lcd_Print_String("SIM900 & PIC");
    } while (!SIM900_isStarted());

    Lcd_Clear();
    Lcd_Set_Cursor(2, 1);
    Lcd_Print_String("Module Detected");
    _delay_ms(1500);

    // Check SIM card detection
    do {
        Lcd_Set_Cursor(2, 1);
        Lcd_Print_String("SIM not found");
    } while (!SIM900_isReady());

    Lcd_Clear();
    Lcd_Set_Cursor(2, 1);
    Lcd_Print_String("SIM Detected");
    _delay_ms(1500);

    Lcd_Clear();

    // Place a Phone Call
    do {
        SIM900_print("ATD+94718984922;\r\n");  // Place a call to number 0718984922
        Lcd_Set_Cursor(1, 1);
        Lcd_Print_String("Placing Call....");
    } while (_SIM900_waitResponse() != SIM900_OK);

    Lcd_Set_Cursor(1, 1);
    Lcd_Print_String("Call Placed....");
    _delay_ms(1500);

    while (1) {
        if (_SIM900_waitResponse() == SIM900_RING) {
            Lcd_Set_Cursor(2, 1);
            Lcd_Print_String("Incoming Call!!");
        }
    }
}
