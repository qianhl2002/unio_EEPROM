#include "unio_EEPROM.h"
Microchip_EEPROM::Microchip_EEPROM(int pin) {
	_pin = pin;
	//_log_cnt = 0;
	unio_input(_pin);
	digitalWriteReg(_pin, HIGH);  //After powering on, clearly define the value of the pin register bit, set to 1;
}
void Microchip_EEPROM::pulseStandBy() {
	unio_output(_pin);
	digitalWriteReg(_pin, HIGH);
	ESPdelayMicroseconds(TSTBY);
}

inline void Microchip_EEPROM::sendBit(int8_t bit) {
	if (bit) {
		digitalWriteReg(_pin, LOW);
		ESPdelayMicroseconds(TE_half);  // Trimmed-Bit-Period
		digitalWriteReg(_pin, HIGH);
		ESPdelayMicroseconds(TE_half);  // Trimmed-Bit-Period
	} else {
		digitalWriteReg(_pin, HIGH);
		ESPdelayMicroseconds(TE_half);  // Trimmed-Bit-Period
		digitalWriteReg(_pin, LOW);
		ESPdelayMicroseconds(TE_half);  // Trimmed-Bit-Period
	}
}

inline uint8_t Microchip_EEPROM::receiveBit() {
	ESPdelayMicroseconds(TE_quarter);
	uint8_t initial = digitalReadReg(_pin);
	ESPdelayMicroseconds(TE_half);
	uint8_t mid_state = digitalReadReg(_pin);
	ESPdelayMicroseconds(TE_quarter);
	if (initial == 0 && mid_state == 1) {
		return 1;
	} else if (initial == 1 && mid_state == 0) {
		return 0;
	} else return 0xff;
}

inline bool Microchip_EEPROM::MAK() {
	sendBit(1);
	return true;
}
inline bool Microchip_EEPROM::noMAK() {
	sendBit(0);
	return false;
}

bool Microchip_EEPROM::isSAK() {
	uint8_t existSAK = 0x00;
	unio_input(_pin);
	existSAK = receiveBit();
	unio_output(_pin);
	if (existSAK == 1) return true;
	return false;
}

void Microchip_EEPROM::pulseTWC() {
	digitalWriteReg(_pin, HIGH);
	unio_output(_pin);
	ESPdelayMicroseconds(TWC);
}

void Microchip_EEPROM::transmitByte(uint8_t value) {
	for (int i = 7; i >= 0; i--) {
		sendBit((value >> i) & 0x01);
	}
}

byte Microchip_EEPROM::readByte() {
	byte data = 0x0, error = 0;
	unio_input(_pin);
	for (int i = 7; i >= 0; i--) {
		uint8_t bit = receiveBit();
		if (bit == 1 || bit == 0) {
			data = (data << 1) | bit;
		} else {
			error = 0xff;
		}
	}
	unio_output(_pin);
	if (error == 0)
		return data;
	else
		return error;
}

//=======================================================================================
bool Microchip_EEPROM::nextCmd() {
	ESPdelayMicroseconds(TSS);
	digitalWriteReg(_pin, LOW);
	ESPdelayMicroseconds(THDR);
	transmitByte(UNIO_STARTHEADER);
	MAK();
	ESPdelayMicroseconds(TE);
	transmitByte(UNIO_device_address);
	MAK();
	return isSAK();
}

bool Microchip_EEPROM::getSAK() {
	pulseStandBy();
	return nextCmd();
}
bool Microchip_EEPROM::connect() {
	return getSAK();
}

byte Microchip_EEPROM::readStatus() {
	byte Status = 0xff;
	if (getSAK()) {
		transmitByte(0x05);
		MAK();
		if (isSAK()) {
			Status = readByte();
			noMAK();
			if (isSAK()) return Status;
			else return 0xff;
		}
	}
	return Status;
}

bool Microchip_EEPROM::transmitAddress(uint16_t addr) {
	uint8_t addrByte = addr >> 8;
	transmitByte(addrByte);
	MAK();
	if (isSAK() == false) {
		return 0;
	}
	addrByte = addr & 0xFF;
	transmitByte(addrByte);
	MAK();
	if (isSAK() == false) {
		return 0;
	}
	return 1;
}

bool Microchip_EEPROM::readAddress(uint16_t addr, byte *array, uint16_t arrLen) {  //read eeprom data
	if (getSAK()) {
		transmitByte(UNIO_READ);
		MAK();
		if (isSAK() == false) {
			if(LOG_DEBUG) Serial.println("write cmd(0x03) failed!");
			return 0;
		}
		if (!transmitAddress(addr)) {
			if(LOG_DEBUG) Serial.println("transmit Address failed!");
			return 0;
		}
	}
	int i = 0;

	for (i = 0; i < arrLen; i++) {
		array[i] = readByte();
		if (i == arrLen)
			noMAK();
		else
			MAK();
		if (isSAK() == false) {
			if(LOG_DEBUG) Serial.printf("\n read Byte failed! byte_num= %d\n", i);
			return 0;
		}
	}
	return 1;
}

bool Microchip_EEPROM::enableWrite() {
	if (getSAK()) {
		transmitByte(UNIO_WREN);  //0x96
		noMAK();
		return isSAK();
	}
	return false;
}

bool Microchip_EEPROM::disableWrite() {  //0x91
	if (getSAK()) {
		transmitByte(UNIO_WRDI);  //0x96
		noMAK();
		return isSAK();
	}
	return false;
}

bool Microchip_EEPROM::writeAllToZero() {  //0x6d
	if (getSAK()) {
		transmitByte(UNIO_ERAL);  //0x96
		noMAK();
		return isSAK();
	}
	return false;
}

bool Microchip_EEPROM::writeAllToOne() {  //0x67
	if (getSAK()) {
		transmitByte(UNIO_SETAL);  //0x96
		noMAK();
		return isSAK();
	}
	return false;
}
int Microchip_EEPROM::isWriteEnabled() {  // WEL = bit1 = 1, write enable
	byte rdsr_byte = readStatus();          //readByte();
	if (rdsr_byte == 0xff) return false;
	if (getBit(rdsr_byte, 1) == 0) {
		return false;
	}
	return true;
}

int Microchip_EEPROM::isReadyToWrite() {  // WIP = bit0 = 1, writing,  return true, free
	byte rdsr_byte = readStatus();          //readByte();
	if (rdsr_byte == 0xff) return false;

	if (getBit(rdsr_byte, 0) == 1) {
		return false;
	}
	return true;
}

bool Microchip_EEPROM::writeAddress(uint16_t addr, byte *array) {  // write 16Byte
	if (enableWrite() && nextCmd()) {
		transmitByte(UNIO_WRITE);
		MAK();
		if (isSAK() == false) {
			if(LOG_DEBUG) Serial.println("write cmd(0x6c) failed!");
			return false;
		}
		if (!transmitAddress(addr)) {
			if(LOG_DEBUG) Serial.println("transmit Address failed!");
			return false;
		}
	}
	int i = 0;
	for (i = 0; i < 16; i++) {
		transmitByte(array[i]);
		if (i == 15)
			noMAK();
		else
			MAK();
		if (isSAK() == false) {
			return false;
		}
	}
	pulseTWC();
	return true;
}

int Microchip_EEPROM::getBit(char a, int bit) {
	return ((a >> bit) & 0x01);
}
