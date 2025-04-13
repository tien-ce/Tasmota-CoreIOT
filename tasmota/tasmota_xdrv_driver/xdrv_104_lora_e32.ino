/*
  xdrv_104_lora_e32.ino - LoRa E32 support for Tasmota

  SPDX-FileCopyrightText: 2024 Theo Arends

  SPDX-License-Identifier: GPL-3.0-only
*/

#ifdef USE_LORA_E32_433
#define USE_LORA_E32_433_GATEWAY //to use Thingsboard MQTT gateway
/*********************************************************************************************\
 * LoRa E32
\*********************************************************************************************/
#define XDRV_104 104

/*********************************************************************************************/

#include "LoRa_E32.h"
#include "HardwareSerial.h"

HardwareSerial * LoraE32Serial = nullptr;
LoRa_E32 * LoraE32 = nullptr;
Configuration * lorae32_cfg = nullptr;

#define LORA_BUF_SIZE 255
bool lorae32_busy = false;
char lorae32_buf[LORA_BUF_SIZE];
bool lorae32_send_sensors = true;

#define D_JSON_ADDH "ADDH"
#define D_JSON_ADDL "ADDL"
#define D_JSON_CHAN "CHAN"
#define D_JSON_uartParity "uartParity"
#define D_JSON_uartBaudRate "uartBaudRate"
#define D_JSON_airDataRate "airDataRate"
#define D_JSON_fixedTransmission "fixedTransmission"
#define D_JSON_ioDriveMode "ioDriveMode"
#define D_JSON_wirelessWakeupTime "wirelessWakeupTime"
#define D_JSON_fec "fec"
#define D_JSON_transmissionPower "transmissionPower"

void LoraE32Config(uint8_t channel = 20, uint8_t addrHigh = 0x01, uint8_t addrLow = 0x02, uint8_t baudRate = 3, uint8_t fixedTransmission = 0)
{
    lorae32_cfg->ADDH = addrHigh; // Địa chỉ cao
    lorae32_cfg->ADDL = addrLow;  // Địa chỉ thấp
    lorae32_cfg->CHAN = channel;  // Kênh truyền
    lorae32_cfg->SPED.uartParity = 0;           // 8N1
    lorae32_cfg->SPED.uartBaudRate = baudRate;  // Tốc độ baud UART
    lorae32_cfg->SPED.airDataRate = 2;          // Tốc độ truyền không khí mặc định
    lorae32_cfg->OPTION.fixedTransmission = fixedTransmission;  // Chế độ Transparent
    lorae32_cfg->OPTION.ioDriveMode = 1;
    lorae32_cfg->OPTION.wirelessWakeupTime = 3; // Wakeup Time mặc định
    lorae32_cfg->OPTION.fec = 1;
    lorae32_cfg->OPTION.transmissionPower = 0;
    LoraE32->setConfiguration(*lorae32_cfg, WRITE_CFG_PWR_DWN_SAVE);
}
void LoraE32Config2Json(void) {
  ResponseAppend_P(PSTR(",\"" D_JSON_ADDH "\":%d"), lorae32_cfg->ADDH);
  ResponseAppend_P(PSTR(",\"" D_JSON_ADDL "\":%d"), lorae32_cfg->ADDL);
  ResponseAppend_P(PSTR(",\"" D_JSON_CHAN "\":%d"), lorae32_cfg->CHAN);
  
  ResponseAppend_P(PSTR(",\"" D_JSON_uartParity "\":%d"), lorae32_cfg->SPED.uartParity);
  ResponseAppend_P(PSTR(",\"" D_JSON_uartBaudRate "\":%d"), lorae32_cfg->SPED.uartBaudRate);
  ResponseAppend_P(PSTR(",\"" D_JSON_airDataRate "\":%d"), lorae32_cfg->SPED.airDataRate);

  ResponseAppend_P(PSTR(",\"" D_JSON_fixedTransmission "\":%d"), lorae32_cfg->OPTION.fixedTransmission);
  ResponseAppend_P(PSTR(",\"" D_JSON_ioDriveMode "\":%d"), lorae32_cfg->OPTION.ioDriveMode);
  ResponseAppend_P(PSTR(",\"" D_JSON_wirelessWakeupTime "\":%d"), lorae32_cfg->OPTION.wirelessWakeupTime);
  ResponseAppend_P(PSTR(",\"" D_JSON_fec "\":%d"), lorae32_cfg->OPTION.fec);
  ResponseAppend_P(PSTR(",\"" D_JSON_transmissionPower "\":%d"), lorae32_cfg->OPTION.transmissionPower);
}
void LoraE32Json2Config(JsonParserObject root) {
  lorae32_cfg->ADDH = root.getUInt(PSTR(D_JSON_ADDH), lorae32_cfg->ADDH);
  lorae32_cfg->ADDL = root.getUInt(PSTR(D_JSON_ADDL), lorae32_cfg->ADDL);
  lorae32_cfg->CHAN  = root.getUInt(PSTR(D_JSON_CHAN), lorae32_cfg->CHAN );
  lorae32_cfg->SPED.uartParity  = root.getUInt(PSTR(D_JSON_uartParity), lorae32_cfg->SPED.uartParity );
  lorae32_cfg->SPED.uartBaudRate  = root.getUInt(PSTR(D_JSON_uartBaudRate), lorae32_cfg->SPED.uartBaudRate );
  lorae32_cfg->SPED.airDataRate  = root.getUInt(PSTR(D_JSON_airDataRate), lorae32_cfg->SPED.airDataRate );
  lorae32_cfg->OPTION.fixedTransmission  = root.getUInt(PSTR(D_JSON_fixedTransmission), lorae32_cfg->OPTION.fixedTransmission );
  lorae32_cfg->OPTION.ioDriveMode  = root.getUInt(PSTR(D_JSON_ioDriveMode), lorae32_cfg->OPTION.ioDriveMode );
  lorae32_cfg->OPTION.wirelessWakeupTime  = root.getUInt(PSTR(D_JSON_wirelessWakeupTime), lorae32_cfg->OPTION.wirelessWakeupTime );
  lorae32_cfg->OPTION.fec  = root.getUInt(PSTR(D_JSON_fec), lorae32_cfg->OPTION.fec );
  lorae32_cfg->OPTION.transmissionPower  = root.getUInt(PSTR(D_JSON_transmissionPower), lorae32_cfg->OPTION.transmissionPower );
}

void LoraE32Init() {
if (!(PinUsed(GPIO_LORA_E32_RX) && PinUsed(GPIO_LORA_E32_TX))) return;

int8_t lora_aux = -1, lora_m0 = -1, lora_m1 = -1, lora_tx = Pin(GPIO_LORA_E32_TX), lora_rx = Pin(GPIO_LORA_E32_RX);

if (PinUsed(GPIO_LORA_E32_AUX)) {
  lora_aux = Pin(GPIO_LORA_E32_AUX);
}
if (PinUsed(GPIO_LORA_E32_M0) && PinUsed(GPIO_LORA_E32_M1)){
  lora_m0 = Pin(GPIO_LORA_E32_M0);
  lora_m1 = Pin(GPIO_LORA_E32_M1);
}

#if CONFIG_IDF_TARGET_ESP32S3
  pinMode(Pin(GPIO_LORA_E32_RX), OUTPUT);
  digitalWrite(Pin(GPIO_LORA_E32_RX), HIGH);
  sleep(1);
#endif // CONFIG_IDF_TARGET_ESP32S3
  LoraE32Serial = new HardwareSerial(1); // HARD assigned UART1
  // Lora = new LoRa_E32(Pin(GPIO_LORA_E32_TX), Pin(GPIO_LORA_E32_RX), LoraE32Serial, UART_BPS_RATE_9600, SERIAL_8N1);
  LoraE32 = new LoRa_E32(lora_tx, lora_rx, LoraE32Serial, lora_aux, lora_m0, lora_m1, UART_BPS_RATE_9600, SERIAL_8N1);
  lorae32_cfg = new Configuration();
  if(!LoraE32 || !lorae32_cfg) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: LoraE32 E32 Initialized failed"));
    return;
  }
  if(LoraE32->begin()) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: LoRa E32 Initialized successfully aux:%d tx:%d rx:%d m0:%d m1:%d"),lora_aux,lora_tx,lora_rx, lora_m0, lora_m1);
  }else {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: LoRa E32 Initialized failed"));
    return;
  } 
#ifdef USE_LORA_E32_433_GATEWAY
  LoraE32GatewayInit();
#endif
}

void LoraE32DataHander(String data) {
// #ifdef USE_MQTT_TB_IOT
#ifdef USE_LORA_E32_433_GATEWAY
  String json = data;
  JsonParser parser((char*)json.c_str());
  if (!parser) return;
  JsonParserObject root = parser.getRootObject();
  if (!root.isValid()) return;
  // contain key "device" and "id" -> RPC Response
  if (root["device"] && root["id"]) {
    return;
  }
  // key is Object -> Attribute
  for (auto key : root) {
    if (key.getValue().isObject()) {
      MqttPublishPayload("v1/gateway/attributes",data.c_str());
    }
  }
  // key is Array -> Telemetry
  for (auto key : root) {
      if (key.getValue().isArray()) {
        MqttPublishPayload("v1/gateway/telemetry",data.c_str());
      }
  }
  return;
#else //Node Handle RPC
  JsonParser parser((char*)data.c_str());
  JsonParserObject rootData = parser.getRootObject();

  if (!rootData.isValid()) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: Not Json"));
    return;
  }

  String deviceStr = rootData.getStr("device", "");
  if (deviceStr != TasmotaGlobal.mqtt_client) {
    return;
  }

  JsonParserToken dataToken = rootData["data"];
  if (!dataToken.isValid()) {
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: No 'data' object"));
    return;
  }
  JsonParserObject dataObj = dataToken.getObject();

  String methodStr = dataObj.getStr("method", "");
  String payload   = dataObj.getStr("params", "");

  String topicStr = "/" + methodStr;

  static char topic[64], mqtt_data[128];
  strlcpy(topic, topicStr.c_str(), sizeof(topic));
  strlcpy(mqtt_data, payload.c_str(), sizeof(mqtt_data));

  XdrvMailbox.topic    = topic;
  XdrvMailbox.data     = mqtt_data;
  XdrvMailbox.data_len = strlen(mqtt_data);
  XdrvMailbox.index    = strlen(topic);

  if (XdrvCall(FUNC_MQTT_DATA)) return;

  ShowSource(SRC_MQTT);
  TasmotaGlobal.last_source = SRC_MQTT;

  CommandHandler(topic, mqtt_data, XdrvMailbox.data_len);
#endif //USE_LORA_E32_433_GATEWAY
// #endif //USE_MQTT_TB_IOT
}

void LoraE32Processing() {
  int data_len = LoraE32->available();
  if (data_len <= 0) return;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Receiving..."));
  lorae32_busy = true;
  ResponseContainer rc = LoraE32->receiveMessageUntil('\n');
  lorae32_busy = false;
  if(rc.status.code != E32_SUCCESS) return;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Rcvd (%d): %s"), rc.data.length(), rc.data.c_str());
  LoraE32DataHander(rc.data);
}

void LoraE32SendData() {
  if (lorae32_buf[0] == '\0') return;
  if (lorae32_busy) return;

  lorae32_busy = true;
  ResponseStatus rs = LoraE32->sendMessage(lorae32_buf);
  lorae32_busy = false;

  lorae32_buf[0] = '\0';
}

void LoraE32SendSensors() {
  if (!lorae32_send_sensors) return;
  String sensors_data = "{";
  ResponseClear();
  XsnsCall(FUNC_JSON_APPEND);
  sensors_data += ResponseData();
  sensors_data += "}";
  if(sensors_data.length() < 3) return;
// #ifdef USE_MQTT_TB_IOT
  JsonParser parser((char*)sensors_data.c_str());
  JsonParserObject sensorsRoot = parser.getRootObject();
  if (!sensorsRoot.isValid()){
    AddLog(LOG_LEVEL_INFO, PSTR("Invalid JSON"));
    return;
  }
  String newPayload = "{\"";
  newPayload += TasmotaGlobal.mqtt_client;
  newPayload += "\":[{";
  for (auto sensorKey : sensorsRoot) {
    // sensor is of type JsonParserKey
    const char *sensorName = sensorKey.getStr();
    JsonParserObject sensors = sensorKey.getValue().getObject();
    bool firstKey = true;
    if (sensors) {
      for (auto subsensorKeyToken : sensors) {
        const char * subsensorKey = subsensorKeyToken.getStr();
        JsonParserToken subsensor = subsensorKeyToken.getValue();
        if (subsensor.isNull()) {
          AddLog(LOG_LEVEL_ERROR, PSTR("LOR: Sensor %s info %s is null."), sensorName, subsensorKey);
          continue;
        }
        if (subsensor.isObject()) {
          // If there is a nested json on sensor data, second level entitites will be created
          JsonParserObject subsensors = subsensor.getObject();
          char sensorInfo[100];
          for (auto subsensor2Key : subsensors) {
            if (subsensor2Key.isNull()) {
              AddLog(LOG_LEVEL_ERROR, PSTR("LOR: Sensor %s info %s.%s is null."), sensorName, subsensorKey, subsensor2Key.getStr());
              continue;
            }
            if (firstKey) {
              snprintf_P(sensorInfo, sizeof(sensorInfo), PSTR("\"%s-%s-%s\":%s"), sensorName, subsensorKey, subsensor2Key.getStr(), subsensor.getStr());
              firstKey = false;
            } else {
              snprintf_P(sensorInfo, sizeof(sensorInfo), PSTR(", \"%s-%s-%s\":%s"), sensorName, subsensorKey, subsensor2Key.getStr(), subsensor.getStr());
            }
            newPayload += String(sensorInfo);
          }
        } else {
          char sensorInfo[100];
          if (firstKey) {
            snprintf_P(sensorInfo, sizeof(sensorInfo), PSTR("\"%s-%s\":%s"), sensorName, subsensorKey, subsensor.getStr());
            firstKey = false;
          } else {
            snprintf_P(sensorInfo, sizeof(sensorInfo), PSTR(", \"%s-%s\":%s"), sensorName, subsensorKey, subsensor.getStr());
          }
          newPayload += String(sensorInfo);
        }
      }
    } else { // ignore non sensor info
      continue;
    }
  }
  newPayload += "}]}";
  if(newPayload.length() >= LORA_BUF_SIZE) {
    AddLog(LOG_LEVEL_ERROR, PSTR("LOR: Sensor data is too long (%u)"), newPayload.length());
    return;
  }
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Send sensors data = %s"), newPayload.c_str());
  strlcpy(lorae32_buf, newPayload.c_str(), sizeof(lorae32_buf));
// #else 
  // strlcpy(lorae32_buf, sensors_data.c_str(), sizeof(lorae32_buf));
// #endif //USE_MQTT_TB_IOT
  LoraE32SendData();
}
#ifdef USE_LORA_E32_433_GATEWAY
bool LoraE32GatewayHandleMqttData() {
  if (!XdrvMailbox.topic || !XdrvMailbox.data) return false;

  String topicStr = XdrvMailbox.topic;
  String payloadStr = XdrvMailbox.data;
  if (topicStr == "v1/gateway/rpc"){
    AddLog(LOG_LEVEL_INFO, PSTR("LOR: Foward rpc: %s"), payloadStr.c_str());
    strlcpy(lorae32_buf, payloadStr.c_str(), sizeof(lorae32_buf));
    LoraE32SendData();
  }
  return true;  
}
void LoraE32GatewayInit(){
  MqttSubscribe("v1/gateway/rpc");
}
#endif
/*********************************************************************************************\
 * Commands
\*********************************************************************************************/
const char kLoRaE32Commands[] PROGMEM = "|" // No Prefix
                                             "LoraE32Get|"
                                             "LoraE32Send|"
                                             "LoraE32SendSensors|"
                                             "LoraE32Config";
void (*const LoRaE32Command[])(void) PROGMEM = {
  &CmndLoraE32Get, &CmndLoraE32Send,&CmndLoraE32SendSensors, &CmndLoraE32Config
};

void CmndLoraE32Get(void) {
  Configuration configuration;
  ModuleInformation moduleInformation;
  ResponseStructContainer rc;
  rc = LoraE32->getConfiguration();
  if(!rc.data) return;
  memcpy(&configuration, rc.data, sizeof(Configuration));
  rc.close();
  rc = LoraE32->getModuleInformation();
  if(!rc.data) return;
  memcpy(&moduleInformation, rc.data, sizeof(ModuleInformation));
  rc.close();

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

  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));
  AddLog(LOG_LEVEL_INFO, PSTR("HEAD BIN: %d %d %X"), moduleInformation.HEAD, moduleInformation.HEAD, moduleInformation.HEAD);
  AddLog(LOG_LEVEL_INFO, PSTR("Freq.: %X"), moduleInformation.frequency);
  AddLog(LOG_LEVEL_INFO, PSTR("Version  : %X"), moduleInformation.version);
  AddLog(LOG_LEVEL_INFO, PSTR("Features : %X"), moduleInformation.features);
  AddLog(LOG_LEVEL_INFO, PSTR("----------------------------------------"));
  ResponseCmndDone();
}

void CmndLoraE32Config(void) {
  // LoRaConfig                                       - Show all parameters
  // LoRaConfig 1                                     - Set default parameters
  // LoRaConfig {"ADDH":0,"ADDL":1}                   - Enter byte parameters
  if (XdrvMailbox.data_len > 0) {
    if (XdrvMailbox.payload == 1) {
      LoraE32Config();
    }
    else {
      JsonParser parser(XdrvMailbox.data);
      JsonParserObject root = parser.getRootObject();
      if (root) { 
        LoraE32Json2Config(root);
        LoraE32->setConfiguration(*lorae32_cfg, WRITE_CFG_PWR_DWN_SAVE);
      }
    }
  }
  ResponseCmnd();  // {"LoRaConfig":
  ResponseAppend_P(PSTR("{"));
  LoraE32Config2Json();
  ResponseAppend_P(PSTR("}}"));
}

void CmndLoraE32Send(void) {
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

  memset(lorae32_buf, 0, LORA_BUF_SIZE);
  uint32_t len = XdrvMailbox.data_len;
  const char *src = XdrvMailbox.data;
  AddLog(LOG_LEVEL_INFO, PSTR("LOR: Send (%d)"), len);
  switch (XdrvMailbox.index) {
    case 0:  // LoRaSend "abc" => "abc\n"
    case 1:
      len = snprintf(lorae32_buf, LORA_BUF_SIZE, "%s\n", src);
      break;

    case 2:
      strlcpy(lorae32_buf, src, LORA_BUF_SIZE);
      len = strlen(lorae32_buf);
      break;

    case 3:
      strlcpy(lorae32_buf, src, LORA_BUF_SIZE - 2);
      len = strlen(lorae32_buf);
      lorae32_buf[len++] = '\f';
      lorae32_buf[len] = '\0';
      break;
  }

  LoraE32SendData();
  ResponseCmndDone();
}

void CmndLoraE32SendSensors(void) {
  // LoRaConfig 1                                      - Send all sensor data
  if (XdrvMailbox.data_len > 0) {
    if (XdrvMailbox.payload == 1) {
      lorae32_send_sensors = true;
    }
    else {
      lorae32_send_sensors = false;
    }
  }
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
  else if (LoraE32) {
    switch (function) {
      case FUNC_LOOP:
      case FUNC_SLEEP_LOOP:
        LoraE32Processing();
        break;
      case FUNC_EVERY_100_MSECOND:
        LoraE32SendData();
        break;
      case FUNC_AFTER_TELEPERIOD:
        LoraE32SendSensors();
        break;
#ifdef USE_LORA_E32_433_GATEWAY
      case FUNC_MQTT_DATA:
        result = LoraE32GatewayHandleMqttData();
        break;
#endif //USE_LORA_E32_433_GATEWAY
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