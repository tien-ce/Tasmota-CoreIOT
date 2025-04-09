/*
  xdrv_104_lora_e32.ino - LoRa E32 support for Tasmota

  SPDX-FileCopyrightText: 2024 Theo Arends

  SPDX-License-Identifier: GPL-3.0-only
*/

#ifdef USE_LORA_E32_433
/*********************************************************************************************\
 * LoRa E32
\*********************************************************************************************/

#define XDRV_104 104

/*********************************************************************************************/

#include "LoRa_E32.h"
#include "HardwareSerial.h"

HardwareSerial * LoraSerial = nullptr;
LoRa_E32 * Lora = nullptr;

#define LORA_BUF_SIZE 255
bool lora_busy = false;
char lora_buf[LORA_BUF_SIZE];
void LoraE32Config(uint8_t channel = 20, uint8_t addrHigh = 0x01, uint8_t addrLow = 0x02, uint8_t baudRate = 3, uint8_t fixedTransmission = 0)
{
    Configuration configuration;
    configuration.ADDH = addrHigh; // Địa chỉ cao
    configuration.ADDL = addrLow;  // Địa chỉ thấp
    configuration.CHAN = channel;  // Kênh truyền
    configuration.SPED.uartParity = 0;           // 8N1
    configuration.SPED.uartBaudRate = baudRate;  // Tốc độ baud UART
    configuration.SPED.airDataRate = 2;          // Tốc độ truyền không khí mặc định
    configuration.OPTION.fixedTransmission = fixedTransmission;  // Chế độ Transparent
    configuration.OPTION.ioDriveMode = 1;
    configuration.OPTION.wirelessWakeupTime = 3; // Wakeup Time mặc định
    configuration.OPTION.fec = 1;
    configuration.OPTION.transmissionPower = 0;
    Lora->setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);

}
void printParameters(struct Configuration configuration)
{
  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));

  AddLog(LOG_LEVEL_INFO, PSTR("HEAD BIN: %d %d %X"), configuration.HEAD, configuration.HEAD, configuration.HEAD);
  AddLog(LOG_LEVEL_INFO, PSTR("AddH BIN: %d"), configuration.ADDH);
  AddLog(LOG_LEVEL_INFO, PSTR("AddL BIN: %d"), configuration.ADDL);
  AddLog(LOG_LEVEL_INFO, PSTR("Chan BIN: %d -> %s"), configuration.CHAN, configuration.getChannelDescription().c_str());

  AddLog(LOG_LEVEL_INFO, PSTR("SpeedParityBit BIN    : %d -> %s"),
         configuration.SPED.uartParity, configuration.SPED.getUARTParityDescription().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("SpeedUARTDataRate BIN : %d -> %s"),
         configuration.SPED.uartBaudRate, configuration.SPED.getUARTBaudRate().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("SpeedAirDataRate BIN  : %d -> %s"),
         configuration.SPED.airDataRate, configuration.SPED.getAirDataRate().c_str());

  AddLog(LOG_LEVEL_INFO, PSTR("OptionTrans BIN       : %d -> %s"),
         configuration.OPTION.fixedTransmission, configuration.OPTION.getFixedTransmissionDescription().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("OptionPullup BIN      : %d -> %s"),
         configuration.OPTION.ioDriveMode, configuration.OPTION.getIODroveModeDescription().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("OptionWakeup BIN      : %d -> %s"),
         configuration.OPTION.wirelessWakeupTime, configuration.OPTION.getWirelessWakeUPTimeDescription().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("OptionFEC BIN         : %d -> %s"),
         configuration.OPTION.fec, configuration.OPTION.getFECDescription().c_str());
  AddLog(LOG_LEVEL_INFO, PSTR("OptionPower BIN       : %d -> %s"),
         configuration.OPTION.transmissionPower, configuration.OPTION.getTransmissionPowerDescription().c_str());

  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));
}

void printModuleInformation(struct ModuleInformation moduleInformation)
{
  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));

  AddLog(LOG_LEVEL_INFO, PSTR("HEAD BIN: %d %d %X"), moduleInformation.HEAD, moduleInformation.HEAD, moduleInformation.HEAD);
  AddLog(LOG_LEVEL_INFO, PSTR("Freq.: %X"), moduleInformation.frequency);
  AddLog(LOG_LEVEL_INFO, PSTR("Version  : %X"), moduleInformation.version);
  AddLog(LOG_LEVEL_INFO, PSTR("Features : %X"), moduleInformation.features);

  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));
}
void LoraE32Init() {
if (!PinUsed(GPIO_LORA_E32_RX) || !PinUsed(GPIO_LORA_E32_TX)) return;

#if CONFIG_IDF_TARGET_ESP32S3
  pinMode(Pin(GPIO_LORA_E32_RX), OUTPUT);
  digitalWrite(Pin(GPIO_LORA_E32_RX), HIGH);
  sleep(1);
#endif // CONFIG_IDF_TARGET_ESP32S3
  LoraSerial = new HardwareSerial(1); // HARD assigned UART1
  Lora = new LoRa_E32(Pin(GPIO_LORA_E32_TX), Pin(GPIO_LORA_E32_RX), LoraSerial, UART_BPS_RATE_9600, SERIAL_8N1);
  if(Lora && Lora->begin()) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: LoRa E32 Initialized successfully"));
  }else {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: LoRa E32 Initialized failed"));
    return;
  } 
  LoraE32Config();
}
void LoraE32Processing() {
  int data_len = Lora->available();
  if (data_len <= 0) return;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Receiving..."));
  lora_busy = true;
  ResponseContainer rc = Lora->receiveMessageUntil('\n');
  lora_busy = false;
  if(rc.status.code != E32_SUCCESS) return;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Rcvd (%d): %s"), rc.data.length(), rc.data.c_str());
}
void LoraE32PrintInfomation() {
  Configuration configuration;
  ModuleInformation moduleInformation;
  ResponseStructContainer rc;
  rc = Lora->getConfiguration();
  if(!rc.data) return;
  memcpy(&configuration, rc.data, sizeof(Configuration));
  rc.close();
  rc = Lora->getModuleInformation();
  if(!rc.data) return;
  memcpy(&moduleInformation, rc.data, sizeof(ModuleInformation));
  rc.close();
  printParameters(configuration);
  printModuleInformation(moduleInformation);
}
void LoraE32SendData() {
  if (lora_buf[0] == '\0') return;
  if (lora_busy) return;

  lora_busy = true;
  ResponseStatus rs = Lora->sendMessage(lora_buf);
  lora_busy = false;

  lora_buf[0] = '\0';
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/
const char kLoRaE32Commands[] PROGMEM = "|" // No Prefix
                                             "e32test|"
                                             "LoraSend|"
                                             "e32testset";
void (*const LoRaE32Command[])(void) PROGMEM = {
    &e32testCommand, &CmndLoraSend, &e32testsetCommand};

void e32testCommand(void)
{
  LoraE32PrintInfomation();
  ResponseCmndDone();
}
void e32testsetCommand(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Custom1 Command Executed!"));
  ResponseCmndDone();
}
void CmndLoraSend(void) {
  // LoRaSend "Hello Tiger"     - Send "Hello Tiger\n"
  // LoRaSend                   - Set to text decoding
  // LoRaSend1 "Hello Tiger"    - Send "Hello Tiger\n"
  // LoRaSend2 "Hello Tiger"    - Send "Hello Tiger"
  // LoRaSend3 "Hello Tiger"    - Send "Hello Tiger\f"
  if ((XdrvMailbox.index < 0) || (XdrvMailbox.index > 3)) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: Invalid LoraSend index"));
    ResponseCmndFailed();
    return;
  }

  memset(lora_buf, 0, LORA_BUF_SIZE);
  uint32_t len = XdrvMailbox.data_len;
  const char *src = XdrvMailbox.data;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Send (%d)"), len);
  switch (XdrvMailbox.index) {
    case 0:  // LoRaSend "abc" => "abc\n"
    case 1:
      len = snprintf(lora_buf, LORA_BUF_SIZE, "%s\n", src);
      break;

    case 2:
      strlcpy(lora_buf, src, LORA_BUF_SIZE);
      len = strlen(lora_buf);
      break;

    case 3:
      strlcpy(lora_buf, src, LORA_BUF_SIZE - 2);
      len = strlen(lora_buf);
      lora_buf[len++] = '\f';
      lora_buf[len] = '\0';
      break;
  }

  LoraE32SendData();
  ResponseCmndDone();
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv104(uint32_t function) {
  bool result = false;

  if (FUNC_INIT == function) {
    LoraE32Init();
  }
  else if (Lora) {
    switch (function) {
      case FUNC_LOOP:
      case FUNC_SLEEP_LOOP:
        LoraE32Processing();
        break;
      case FUNC_EVERY_100_MSECOND:
        LoraE32SendData();
        break;
      case FUNC_COMMAND:
        result = DecodeCommand(kLoRaE32Commands, LoRaE32Command);
        break;
      case FUNC_ACTIVE:
        result = true;
        break;
    }
  }
  return result;
}
#endif // USE_LORA_E32_433