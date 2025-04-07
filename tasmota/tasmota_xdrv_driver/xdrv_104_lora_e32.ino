#ifdef USE_LORA_E32_433
#define XDRV_104 104 // Định danh driver trong hệ thống Tasmota

#include "LoRa_E32.h"
#include "HardwareSerial.h"
#define LORA_E32_433_RX 7 // RX của ESP32 (gắn với TX của LoRa)
#define LORA_E32_433_TX 6 // TX của ESP32 (gắn với RX của LoRa)
HardwareSerial * LoraSerial = nullptr;
LoRa_E32 * Lora = nullptr;
bool lora_e32 = false;
struct Pkg {
uint8_t id;
char payload [64];
};
const char LoRaE32Commands[] PROGMEM = "|" // No Prefix
                                             "SendLora|"
                                             "e32test|"
                                             "e32testset";
void (*const LoRaE32Command[])(void) PROGMEM = {
    &CmdSendLora,
    &e32testCommand, &e32testsetCommand};

void CmdSendLora(void)
{
    if (XdrvMailbox.data_len == 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Nothing to transmit"));
        ResponseCmndDone();
        return;
    }
    char *tran = XdrvMailbox.data;
    AddLog(LOG_LEVEL_INFO, PSTR("Transmit data: %s"), tran);
    ResponseStatus rs = Lora->sendMessage(tran);
    AddLog(LOG_LEVEL_INFO, rs.getResponseDescription().c_str());
    ResponseCmndDone();
}
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
void LoraE32Init()
{
if (!PinUsed(GPIO_LORA_E32_RX) || !PinUsed(GPIO_LORA_E32_TX)) return;

#if CONFIG_IDF_TARGET_ESP32S3
  pinMode(Pin(GPIO_LORA_E32_RX), OUTPUT);
  digitalWrite(Pin(GPIO_LORA_E32_RX), HIGH);
  sleep(1);
#endif // CONFIG_IDF_TARGET_ESP32S3
  LoraSerial = new HardwareSerial(1);
  Lora = new LoRa_E32(Pin(GPIO_LORA_E32_TX), Pin(GPIO_LORA_E32_RX), LoraSerial, UART_BPS_RATE_9600, SERIAL_8N1);
  if(!Lora) return;
  lora_e32 = Lora->begin();
  if(lora_e32) {
     AddLog(LOG_LEVEL_INFO, PSTR("LoRa E32 Initialized successfully"));
  } else AddLog(LOG_LEVEL_INFO, PSTR("LoRa E32 Initialized failed"));
  LoraE32Config();
}
void LoraE32Processing()
{
  if (!lora_e32)
    return;

  // Kiểm tra có tin nhắn từ LoRa
  if (Lora->available() > 1)
  {
    ResponseContainer rc = Lora->receiveMessageUntil('!');
    if (rc.status.code == 1)
    {
      AddLog(LOG_LEVEL_INFO, PSTR("Receive Mess: "));
      AddLog(LOG_LEVEL_INFO, rc.data.c_str());
    }
    else
    {
      AddLog(LOG_LEVEL_INFO, PSTR("ERROR!"));
    }
  }
  // if(Lora->available() > 1){
  //   ResponseStructContainer rc = Lora->receiveMessage(sizeof(Pkg));
  //   if (rc.status.code == 1)
  //     {
  //       Pkg pkg;
  //       memcpy(&pkg, rc.data, sizeof(Pkg));
  //       rc.close();
  //       AddLog(LOG_LEVEL_INFO, PSTR("ID: %u, Payload: %s"), pkg.id, pkg.payload);
  //       // AddLog(LOG_LEVEL_INFO, rc.data.c_str());
  //     }
  //     else
  //     {
  //       AddLog(LOG_LEVEL_INFO, PSTR("ERROR!"));
  //     }
  // }
}
void LoraE32PrintInfomation(){
  if(!Lora) return;
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
bool Xdrv104(uint32_t function)
{

  bool result = false;

  if (FUNC_INIT == function)
  {
    // AddLog(LOG_LEVEL_INFO, PSTR("INIT"));
    LoraE32Init();
  }
  else if (lora_e32)
  {

    switch (function)
    {
      //    Select suitable interval for polling your function
    // case FUNC_EVERY_SECOND:
    //   AddLog(LOG_LEVEL_INFO, PSTR("EVERY SECOND"));
    //   break;
    case FUNC_EVERY_250_MSECOND:
      LoraE32Processing();
      break;
    case FUNC_COMMAND:
      result = DecodeCommand(LoRaE32Commands, LoRaE32Command);
      break;
      // case FUNC_EVERY_SECOND:
      //   break;
      //    case FUNC_EVERY_200_MSECOND:
      //    case FUNC_EVERY_100_MSECOND:
    }
  }

  return result;
}
#endif // USE_LORA_E32_433