/*
	In the Arduino environment, using ESP32S3 to read and write 
	Microchip UNI/O EEPROM class (referencing the library written by Pavlo Ponomarov), 
	written by Qian Hongliang,2026.5.20
	Henan Zhongyuan Optoelectronic Measurement & Control Technology Co., Ltd.
*/
#ifndef unio_EEPROM_h
#define unio_EEPROM_h

#include "Arduino.h"
#include <String.h>
#define LOG_DEBUG 1

//=================================Device Parameters and Commands====================================
//---------------------------------Time constraints------------------------------------
#define TSTBY 605            // StandBy-Pulse 600µs + 5µs offset
#define TSS 15               // StartHeader(high) setup time 10µs + 5µs offset
#define THDR 10              // StartHeader low pulse time 5µs + 5µs offset 
#define TE_quarter 8         // TE_quarter=6,7,8,9,10....24
#define TE (TE_quarter * 4)  
#define TE_half (TE_quarter * 2) // BitPeriod time >10µs + 10µs offset; >=24,ok.
//#define THDL TE*10 // BitPeriod time 10µs + 5µs offset
#define TWC 5005  // BitPeriod time 5000µs + 5µs offset
//---------------------------------Start header, address, and command-------------------
#define UNIO_STARTHEADER 0x55
#define UNIO_device_address 0xA0
//#define UNIO_device_address       0xA1
#define UNIO_READ 0x03
#define UNIO_CRRD 0x06
#define UNIO_WRITE 0x6c
#define UNIO_WREN 0x96
#define UNIO_WRDI 0x91
#define UNIO_RDSR 0x05  //Read STATUS register
#define UNIO_WRSR 0x6e
#define UNIO_ERAL 0x6d
#define UNIO_SETAL 0x67
//---------------------------------Response bit--------------------------------------
#define SAK 1
#define noSAK 0
//============================================================================================

//！！！！！！！！！！！！！！！！！！！！！Related to the MCU model！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
// To achieve more accurate and shorter latency, use registers to set GPIO instead of pinMode and digitalRead/Write
//You must include this header file to access the GPIO structure, which loses compatibility.
#include "soc/gpio_struct.h"
#define UNIO_PIN 14  //Modify the pin number according to your actual connection
#define pinModeReg(pin, mode) mode == INPUT ? GPIO.enable_w1tc = (1ULL << pin) : GPIO.enable_w1ts = (1ULL << pin)
#define digitalWriteReg(pin, isHigh) ((isHigh == HIGH) ? GPIO.out_w1ts = (1ULL << pin) : GPIO.out_w1tc = (1ULL << pin))
#define digitalReadReg(pin) ((GPIO.in >> pin) & 0x1)
#define unio_output(_unio_pin) pinModeReg(_unio_pin, OUTPUT)
#define unio_input(_unio_pin) pinModeReg(_unio_pin, INPUT)
inline void ESPDelay1us() {
	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;	GPIO.in;  //10
	GPIO.in;
	GPIO.in;  //GPIO.in TIME  0.076us at 240Mhz
	NOP();	NOP();	NOP();
	NOP();  //Precisely adjusted to 1 µs, with an error of less than 50 ns
}
inline void ESPdelayMicroseconds(uint32_t us) {
	volatile uint32_t i = 0;
	while (i < us) {
		ESPDelay1us();
		i++;
	};
}  //{delayMicroseconds(us);} //
//！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

class Microchip_EEPROM {
private:
	int _pin;
	inline void sendBit(int8_t bit);
	inline uint8_t receiveBit();
	inline bool MAK();
	inline bool noMAK();
	inline bool isSAK();
	void transmitByte(uint8_t value);
	byte readByte();
	bool transmitAddress(uint16_t addr);
	bool enableWrite();
	bool disableWrite();
	bool writeAllToZero();
	bool writeAllToOne();
	int getBit(char value, int bit);
	int isWriteEnabled();
	int isReadyToWrite();
public:
	Microchip_EEPROM(int pin);
	void pulseStandBy();
	void pulseTWC();

	bool nextCmd();
	bool getSAK();
	bool connect();
	byte readStatus();

	bool readAddress(uint16_t addr, byte *array, uint16_t arrLen);
	bool writeAddress(uint16_t addr, byte *array);
};
#endif
