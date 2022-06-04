#include "lcd.h"
#include "avrgpio.h"
#include "waitloop.h"

#ifdef LCD_ENABLED

#define LCD_ADDRESS 0x27
#define LCD_CMD 0
#define LCD_CHR 1
#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04

avr::GPIO<avr::B, 4> i2c_clk;
avr::GPIO<avr::B, 5> i2c_dat;

// software I2C
class I2C {
private:
	void delay()
	{
		asm("nop");
	}

	// 初期化
	void init_i2c()
	{
		i2c_clk.setpullup(true);
		i2c_dat.setpullup(true);
	}

	void i2c_cl_0()
	{
		i2c_clk.write(false);
	}

	void i2c_cl_1()
	{
		i2c_clk.write(true);
	}

	void i2c_da_0()
	{
		i2c_dat.write(false);
	}

	void i2c_da_1()
	{
		i2c_dat.write(true);
	}

	uint8_t i2c_get_da()
	{
		return i2c_dat.read();
	}

	// スタートコンディション
	void i2c_start()
	{
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// ストップコンディション
	void i2c_stop()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_1(); // SDA=1
		delay();
	}

	// リピーテッドスタートコンディション
	void i2c_repeat()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// 1バイト送信
	bool i2c_write(uint8_t c)
	{
		bool nack;

		delay();

		// 8ビット送信
		for (uint8_t i = 0; i < 8; i++) {
			if (c & 0x80) {
				i2c_da_1(); // SCL=1
			} else {
				i2c_da_0(); // SCL=0
			}
			c <<= 1;
			delay();
			i2c_cl_1(); // SCL=1
			delay();
			i2c_cl_0(); // SCL=0
			delay();
		}

		i2c_da_1(); // SDA=1
		delay();

		i2c_cl_1(); // SCL=1
		delay();
		// NACKビットを受信
		nack = i2c_get_da();
		i2c_cl_0(); // SCL=0

		return nack;
	}

	uint8_t address; // I2Cデバイスアドレス

public:
	I2C(uint8_t address)
		: address(address)
	{
		init_i2c();
	}

	// デバイスのレジスタに書き込む
	void write(uint8_t reg, uint8_t data)
	{
		i2c_start();                   // スタート
		i2c_write(address << 1);       // デバイスアドレスを送信
		i2c_write(reg);                // レジスタ番号を送信
		i2c_write(data);               // データを送信
		i2c_stop();                    // ストップ
	}

	void write(uint8_t data)
	{
		i2c_start();                   // スタート
		i2c_write(address << 1);       // デバイスアドレスを送信
		i2c_write(data);               // データを送信
		i2c_stop();                    // ストップ
	}

};


I2C wire(LCD_ADDRESS);

void lcd_byte(uint8_t bits, uint8_t mode)
{
	uint8_t hi = mode | (bits & 0xf0) | LCD_BACKLIGHT;
	uint8_t lo = mode | ((bits << 4) & 0xf0) | LCD_BACKLIGHT;
	usleep(10);
	const int W = 1;
	wire.write(hi);
	usleep(W);
	wire.write(hi | ENABLE);
	usleep(W);
	wire.write(hi);
	usleep(W);
	wire.write(lo);
	usleep(W);
	wire.write(lo | ENABLE);
	usleep(W);
	wire.write(lo);
	usleep(10);
}

void lcd::home()
{
	lcd_byte(0x80, LCD_CMD);
}

void lcd::putchar(char c)
{
	lcd_byte(c, LCD_CHR);
}

void lcd::print(const char *ptr)
{
	while (*ptr) {
		putchar(*ptr);
		ptr++;
	}
}

void lcd::clear()
{
	lcd_byte(0x01, LCD_CMD);
	msleep(2);
}

void lcd::init()
{
	lcd_byte(0x33, LCD_CMD);
	usleep(40);
	lcd_byte(0x32, LCD_CMD);
	usleep(40);
	lcd_byte(0x06, LCD_CMD);
	usleep(40);
	lcd_byte(0x0c, LCD_CMD);
	usleep(40);
	lcd_byte(0x28, LCD_CMD);
	usleep(40);
	clear();
}

#endif // LCD_ENABLED
