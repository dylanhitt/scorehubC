#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>

// Define some device parameters
#define I2C_ADDR 0x27 // I2C device address

// Define some device constants
#define LCD_CHR 1 // Mode - Sending data
#define LCD_CMD 0 // Mode - Sending command

#define LINE0 0x80
#define LINE1 0xC0
#define LINE2 0x94
#define LINE3 0xD4

#define LCD_BACKLIGHT_ON 0x08 // On
#define LCD_BACKLIGHT_OFF 0x00 // Off

#define ENABLE 0b00000100 // Enable bit

void lcd_init(void);
void lcd_loc(int line); // move cursor
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);

void lcd_on(void);
void lcd_off(void);
void lcd_clear(void);      // clr LCD return home
void lcd_print_at(int row, int col, const char *s);
void type_ln(const char *s);

int fd; // seen by all subroutines

int main(int argc, char *argv[])
{
    if (wiringPiSetup() == -1)
        exit(1);

    fd = wiringPiI2CSetup(I2C_ADDR);
	lcd_init();
	
	// TODO make this better with --off flag
	if (argc == 2)
	{
		lcd_off();
		return 0;
	} 
	else 
	{
		lcd_on();
	}
	
    char array1[] = "Hello world!";
    lcd_print_at(2, 0, array1);
	

    return 0;
}

void lcd_print_at(int row, int col, const char *s)
{
    int line;
    switch (row)
    {
    case 0:
        line = LINE0;
        break;
    case 1:
        line = LINE1;
        break;
    case 2:
        line = LINE2;
        break;
    case 3:
        line = LINE3;
        break;
    }

    lcd_loc(line + col);
    type_ln(s);
}

// turn off lcd backlight and clear
void lcd_on(void)
{
	int bits = LCD_CMD | 0xF0 | LCD_BACKLIGHT_ON;
	wiringPiI2CReadReg8(fd, bits);
}

// turn off lcd backlight and clear
void lcd_off(void)
{
	lcd_clear();
	int bits = LCD_CMD | 0xF0 | LCD_BACKLIGHT_OFF;
	wiringPiI2CReadReg8(fd, bits);
}	

// go to location on LCD
void lcd_loc(int line)
{
    lcd_byte(line, LCD_CMD);
}

// clr lcd go home loc 0x80
void lcd_clear(void)
{
    lcd_byte(0x01, LCD_CMD);
}

// this allows use of any size string
void type_ln(const char *s)
{
    while (*s)
        lcd_byte(*(s++), LCD_CHR);
}

void lcd_byte(int bits, int mode)
{
    // Send byte to data pins
    //  bits = the data
    //  mode = 1 for data, 0 for command
    int bits_high;
    int bits_low;
    // uses the two half byte writes to LCD
    bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT_ON;
    bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT_ON;

    // High bits
    wiringPiI2CReadReg8(fd, bits_high);
    lcd_toggle_enable(bits_high);

    // Low bits
    wiringPiI2CReadReg8(fd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)
{
    // Toggle enable pin on LCD display
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
    delayMicroseconds(500);
}

void lcd_init()
{
    // Initialise display
    lcd_byte(0x33, LCD_CMD); // Initialise
    lcd_byte(0x32, LCD_CMD); // Initialise
    lcd_byte(0x06, LCD_CMD); // Cursor move direction
    lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
    lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
    lcd_byte(0x01, LCD_CMD); // Clear display
    delayMicroseconds(500);
}
