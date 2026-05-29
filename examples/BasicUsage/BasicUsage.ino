#include "unio_EEPROM.h"
#define LC040_SIZE 512
Microchip_EEPROM LC040(UNIO_PIN);  //4096bit ,512Byte
byte arrRead[16] =  { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff };
byte arrWrite[16] = { 0xff, 0x55, 0xaa, 0x00, 0xff, 0x55, 0xaa, 0x00, 0xff, 0x55, 0xaa, 0x00, 0xff, 0x55, 0xaa, 0x00 };
byte LC040_512[LC040_SIZE];
uint16_t testAddr = 0;
void setup() {
  Serial.begin(115200);
  delay(1000);

  while (Serial.available() > 0) {
    Serial.read();
  }

  Serial.println("Usage:");
  Serial.println("s - to start; ");
  Serial.println("c - to connect,get the address to read/write;");
  Serial.println("write - to write to EEPROM at the address;");
  Serial.println("read - to read from EEPROM at the address; ");
  Serial.println("t - to test(read data,read status until Serial available);");
  Serial.println("Waiting for client..(s)");

  pinMode(UNIO_PIN, INPUT);
  delay(1);
  //uni/o requires a pull-up resistor, it is best to use open-drain output mode
  pinMode(UNIO_PIN, OUTPUT_OPEN_DRAIN);
  delay(5);
  digitalWrite(UNIO_PIN, LOW);  //power on init
  ESPdelayMicroseconds(100);
  digitalWrite(UNIO_PIN, HIGH);

  delay(5);
  while (!GUIConnected()) { delay(500); }  //Loop until client input 's'

void loop() {
  if (Serial.available()) {
    String command;
    command = Serial.readStringUntil('\n');
    if (command.equals("t")) {
      Serial.println("---------------------------test---------------------------");
      test();
    }
    if (command.equals("c")) {
      if (LC040.connect()) {
        Serial.printf("The EEPRom Connected,Awaiting input address(0,16,32...,The address must be a multiple of 16,max=496) to read/write:");
        while (!Serial.available())
          ;
        String line = Serial.readStringUntil('\n');
        testAddr = line.toInt();
        if (testAddr < 0 || testAddr > LC040_SIZE - 1) {
          Serial.printf("input error,address set to 0!\n", testAddr);
          testAddr = 0;
        }
        Serial.printf("Address(read/write) =%d\n", testAddr);
      } else {
        Serial.println("Fail,not conected");
      }
    }
    if (command.equals("read")) {
      readEEPROM(testAddr);
      dumpEEPRom((const uint8_t *)arrRead, sizeof(arrRead), false);
    }
    if (command.equals("write")) {
      Serial.printf("Data will be modified at address:%d,ENTER to cancel.", testAddr);
      Serial.printf("Awaiting input(length<=16):\n");
      while (!Serial.available())
        ;
      String line = Serial.readStringUntil('\n');
      if (line.length() > 0) {
        char buf[16];  //EEPROM write once max 16 chartacter
        if (line.length() > 16) {
          strncpy(buf, line.c_str(), 16);
          Serial.printf("String is longer than 16 symbols(length=%d),the first 16 are reserved.", line.length());
        } else {
          strncpy(buf, line.c_str(), line.length());
        }
        Serial.println("Got it:");
        dumpEEPRom((const uint8_t *)buf, 16, false);
        writeEEPROM(testAddr, buf);
      } else Serial.println("write Cancel!");
    }
  }
}

bool GUIConnected() {
  if (Serial.available()) {
    if (Serial.read() == 's') {
      Serial.println("plaese input c/write/read/t!");
      return true;
    }
  }
  return false;
}

void test() {
  for (int i = 0; i < (512); i = i + 16) {  //32
    delay(1);
    readEEPROM(i);
  }
  dumpEEPRom((const uint8_t *)LC040_512, sizeof(LC040_512), true);
  uint loopNUm = 1;
  while (!Serial.available()) {
    readStatusReg();
    printf("You can use an oscilloscope to observe SAK at GPIO Pin %d. loop num = %d.\n  ", UNIO_PIN, loopNUm++);
    delay(5000);
  };
  while (Serial.available() > 0) Serial.read();
}
void readStatusReg() {
  int testNum = 0, repeatNum = 5;
  byte Status;
  testNum = 0;
  Status = LC040.readStatus();
  while (Status == 0xff && testNum < repeatNum) {
    Status = LC040.readStatus();
    testNum++;
  };
  if (testNum < repeatNum)
    Serial.printf("Status read SUCCESS! Num(tried)=%d; Status value:%0x\n", testNum + 1, Status);
  else
    Serial.printf("Status read FAIL! Num(tried)=%d; Status value:%0x\n", testNum + 1, Status);
  ESPdelayMicroseconds(TSTBY);
}

void readEEPROM(uint16_t addr) {
  uint16_t Len = sizeof(arrRead);
  if (LC040.readAddress(addr, arrRead, Len)) {
    if ((addr + Len) <= sizeof(LC040_512)) {
      memcpy(&LC040_512[addr], arrRead, Len);
    }
  } else {
    Serial.printf("\nread Address %d failed! \n", addr);
  }
}

void writeEEPROM(uint16_t addr, char *arr) {
  if (LC040.writeAddress(addr, (byte *)arr)) {  //addr
    Serial.printf("write Address %d sucess,length=%d \n", addr, 16);
  } else {
    Serial.printf("write Address %d  failed! \n", addr);
  }
}

void printArray(char *arr, int Len) {
  for (int i = 0; i < Len; i++) {
    if (arr[i] < 0x10) Serial.print("0");
    Serial.print(arr[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}

void dumpEEPRom(const uint8_t *buf, size_t len, bool showLineNumber) {
  size_t line = 0;
  for (size_t base = 0; base < len; base += 16) {
    if (showLineNumber) {
      // Decimal line number (right-aligned, 4 digits)
      if (line < 10) Serial.print("   ");
      else if (line < 100) Serial.print("  ");
      else if (line < 1000) Serial.print(" ");
      Serial.print(line);
      Serial.print(": ");
      // Address offset (in hexadecimal)
      if (base < 0x10) Serial.print("000");
      else if (base < 0x100) Serial.print("00");
      else if (base < 0x1000) Serial.print("0");
      Serial.print(base, HEX);
      Serial.print(": ");
    }
    // HEX
    for (int i = 0; i < 16; i++) {
      if (base + i < len) {
        if (buf[base + i] < 0x10) Serial.print("0");
        Serial.print(buf[base + i], HEX);
      } else {
        Serial.print("  ");
      }
      Serial.print(" ");
    }
    Serial.print(" ");
    // ASCII
    for (int i = 0; i < 16; i++) {
      if (base + i < len) {
        char c = buf[base + i];
        Serial.print((c >= 32 && c <= 126) ? c : '.');
      }
    }
    Serial.println();
    line++;
  }
}