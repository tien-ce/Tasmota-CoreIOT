/*
  xdrv_128_1_lorae32.ino - LoRa E32 module integration for Tasmota
  Based on Lora E32 UART module communication
*/
#include "my_user_config.h"
#ifdef USE_LORA_E32

#ifndef ESP32
#define ESP32
#endif
#define NAME_NODE "SI Soil Moisture 1"
/*********************************************************************************************\
 * LoRa E32 bridge for Tasmota
 * Communicates with LoRa E32 modules over UART
 * Sends and receives messages via MQTT
\*********************************************************************************************/

#warning **** LoRa E32 Driver is included... ****
#define XDRV_128 128 // Định danh driver trong hệ thống Tasmota
#include "LoRa_E32.h"
#include <Arduino.h>
#include "ArduinoJson.h" 
/****************************************************************************************** */
uint32_t M0_Pin = -1;
uint32_t M1_Pin = -1;
uint32_t AUX_Pin = -1;
/*********************************************************************************************\
* LoraE32 command
// This variable will be set to true after initialization
bool AddLoginitSuccess1 = false;

/*
  Optional: if you need to pass any command for your device
  Commands are issued in Console or Web Console
  Commands:
    Say_Hello  - Only prints some text. Can be made something more useful...
    Send_Data  - Send data in list data
    Send_Attribute  - Send attribute of Lora Device
    Help       - Prints a list of available commands
*/

struct LoraSerial_t
{
    bool active = false;
    byte tx = 0;
    byte rx = 0;
    LoRa_E32 *LoraSerial = nullptr;
}LoraSerial;

HardwareSerial *mySerial = nullptr;
class Device_info
{
public:
    String device_name;
    uint8_t channel;  // kênh truyền
    uint8_t addrHigh; // Địa chỉ cao
    uint8_t addrLow;  // Địa chỉ thấp
    float longitude;  // Kinh độ
    float latitude;   // Vĩ độ
    uint8_t target_addrHigh;
    uint8_t target_addrLow;
    Device_info()
    {
        this->device_name = "";
    }
    Device_info(String device_name)
    {
        this->device_name = device_name;
    }
    Device_info( uint8_t addr_high,uint8_t addr_low)
    {
        this->addrHigh = addr_high;
        this->addrLow = addr_low;
    }
    Device_info(String device_name, uint8_t addr_high,uint8_t addr_low,
        uint8_t channel, float longitude, float latitude, uint8_t target_addr_high,uint8_t target_addr_low)
    {
        this->device_name = device_name;
        this->addrHigh = addr_high;
        this->addrLow = addr_low;
        this->channel = channel;
        this->longitude = longitude;
        this->latitude = latitude;
        this->target_addrHigh = target_addr_high;
        this->target_addrLow = target_addr_low;
    }
    void setLoraName(String device_name)
    {
        this->device_name = device_name;
    }
};
/* ********************************Biến toàn cục***********************************/
StaticJsonDocument<256> Lora_mapping;
JsonObject Lora_mapping_ojb = Lora_mapping.to<JsonObject>();
/************************************************************************************* */
Device_info device_info("SI Water Meter 1",0x12,0x34,23, 106.76940000, 10.90682000,0x01,0x02);
/*************************************In cấu hình LORA ******************************* */
/*************************************** LOG DETAIL LORA ****************************************************** */
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


/********************************************************************************************** */
void LoraSerialInit(void)
{
    LoraSerial.active = false;
    if(PinUsed(GPIO_LORA_E32_TX) && PinUsed(GPIO_LORA_E32_RX))
    {
      LoraSerial.rx = Pin(GPIO_LORA_E32_RX);
      LoraSerial.tx = Pin(GPIO_LORA_E32_TX);
      LoraSerial.active = true;
    }

    if(LoraSerial.active)
    {
        mySerial = new HardwareSerial(1); // Use UART2
        // mySerial->begin(9600, SERIAL_8N1, LoraSerial.tx, LoraSerial.rx);
        String mac_str = TasmotaGlobal.mqtt_client; // 		DVES_18E8AC
        // mac_str = WiFi.macAddress(); // "AB:CD:EF:GH:JK:LM"
        int first = mac_str.lastIndexOf("_");             // At _18....
        String mac_device = mac_str.substring(first + 1); //"18E8AC"
        // device_info.setLoraName(mac_device);
        device_info.setLoraName(NAME_NODE);
        //Serial1.begin(9600, SERIAL_8N1, LoraSerial.tx, LoraSerial.rx);
        LoraSerial.LoraSerial = new LoRa_E32(LoraSerial.tx, LoraSerial.rx, mySerial, UART_BPS_RATE_9600, SERIAL_8N1);
        ResponseStatus rs;
        bool check = LoraSerial.LoraSerial->begin();
        if (check)
        {
            //LoraSerial.active = true;
            ResponseStructContainer c = LoraSerial.LoraSerial->getConfiguration();
            Configuration configuration = *(Configuration*) c.data;
            AddLog(LOG_LEVEL_INFO, "%s", c.status.getResponseDescription().c_str());

            // configuration.ADDH = 0x12;
            // configuration.ADDL = 0x34;
            // configuration.CHAN = 20;
            // configuration.SPED.uartParity = MODE_00_8N1;     
            // configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;  
            // configuration.SPED.uartBaudRate = 3;
            // configuration.OPTION.fixedTransmission = 1;
            // configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS;  
            // configuration.OPTION.wirelessWakeupTime = WAKE_UP_750;             
            // configuration.OPTION.fec = FEC_1_ON;                                
            // configuration.OPTION.transmissionPower = POWER_20;                 

            // rs = LoraSerial.LoraSerial->setConfiguration(configuration,WRITE_CFG_PWR_DWN_SAVE);
            AddLog(LOG_LEVEL_INFO, "%s", c.status.getResponseDescription().c_str());
            c = LoraSerial.LoraSerial->getConfiguration();
            // It's important get configuration pointer before all other operation
            configuration = *(Configuration*) c.data;
            AddLog(LOG_LEVEL_INFO, "%s", c.status.getResponseDescription().c_str());
            if(c.status.code == SUCCESS){
                printParameters(configuration);
                device_info.addrHigh = configuration.ADDH;
                device_info.addrLow = configuration.ADDL;
            }
            else{
                AddLog(LOG_LEVEL_INFO,PSTR("ERROR TO READ CONFIGURATION"));
            }
            c.close();
            AddLog(LOG_LEVEL_INFO, "LoraSerial: Init OK");
        }
        else
        {
            delete mySerial;
            mySerial = nullptr;
            delete LoraSerial.LoraSerial;
            LoraSerial.LoraSerial = nullptr;
            LoraSerial.active = false;
            AddLog(LOG_LEVEL_ERROR, PSTR("LoraSerial: Init failed %s"), rs.getResponseDescription().c_str());
        }
    }

}

#define D_CMND_SEND_LORA_SERIAL "SendLora"
#define D_CMND_SEND_LORA_TELEMETRY "SendLoraTelemetry"
#define D_CMND_SEND_Lora_ATTRIBUTE "SendLoraAttribute"
#define D_CMND_PRINT_MAPPING_Lora "PrintLoraMapping"
#define D_CMND_SET_CONF_Lora "SetLoraConf"
#define D_CMND_PRINT_CONF_Lora "PrintLoraConf"
#define D_CMND_KEY_CONF_Lora "PrintKeyLoraConf"
const char kLoraSerialCommands[] PROGMEM = "|"
    D_CMND_SEND_LORA_SERIAL "|"
    D_CMND_SEND_LORA_TELEMETRY"|"
    D_CMND_SEND_Lora_ATTRIBUTE"|"
    D_CMND_PRINT_MAPPING_Lora "|"
    D_CMND_PRINT_CONF_Lora "|"
    D_CMND_SET_CONF_Lora"|"
    D_CMND_KEY_CONF_Lora;

void (* const LoraSerialCommand[])(void) PROGMEM = {
    &CmndSendLora,
    &CmndSendLoraTelemetry,
    &CmndSendLoraAttribute,
    &CmndPrintMapLora,
    &CmndPrintConfLora,
    &CmndSetConfLora,
    &CmndPrintKeyLoraConf
};
void CmndPrintKeyLoraConf(void) {
    AddLog(LOG_LEVEL_INFO, PSTR("------ USE THESE KEYS FOR SET CONF ------"));

    AddLog(LOG_LEVEL_INFO, PSTR("ADDH               ---> Address High Byte"));
    AddLog(LOG_LEVEL_INFO, PSTR("ADDL               ---> Address Low Byte"));
    AddLog(LOG_LEVEL_INFO, PSTR("TAR_ADDH           ---> Target Address High"));
    AddLog(LOG_LEVEL_INFO, PSTR("TAR_ADDL           ---> Target Address Low"));
    AddLog(LOG_LEVEL_INFO, PSTR("NETID              ---> Network ID"));
    AddLog(LOG_LEVEL_INFO, PSTR("CHAN               ---> Channel (0-83)"));

    AddLog(LOG_LEVEL_INFO, PSTR("uartBaudRate       ---> UART Baud Rate (e.g., 9600, 115200)"));
    AddLog(LOG_LEVEL_INFO, PSTR("airDataRate        ---> Air Data Rate"));
    AddLog(LOG_LEVEL_INFO, PSTR("uartParity         ---> UART Parity Mode (e.g., 8N1)"));

    AddLog(LOG_LEVEL_INFO, PSTR("subPacketSetting   ---> Sub-Packet Size Setting"));
    AddLog(LOG_LEVEL_INFO, PSTR("RSSIAmbientNoise   ---> Enable/Disable RSSI Ambient Noise"));
    AddLog(LOG_LEVEL_INFO, PSTR("transmissionPower  ---> Transmission Power Level"));

    AddLog(LOG_LEVEL_INFO, PSTR("enableRSSI         ---> Enable RSSI Reporting"));
    AddLog(LOG_LEVEL_INFO, PSTR("fixedTransmission  ---> Fixed Transmission Mode"));
    AddLog(LOG_LEVEL_INFO, PSTR("enableRepeater     ---> Enable Repeater Function"));
    AddLog(LOG_LEVEL_INFO, PSTR("enableLBT          ---> Enable Listen Before Talk"));
    AddLog(LOG_LEVEL_INFO, PSTR("WORTransceiverControl ---> WOR Mode: Transmitter/Receiver"));
    AddLog(LOG_LEVEL_INFO, PSTR("WORPeriod          ---> WOR Wake Period"));

    AddLog(LOG_LEVEL_INFO, PSTR("CRYPT_H            ---> Encryption Key High Byte"));
    AddLog(LOG_LEVEL_INFO, PSTR("CRYPT_L            ---> Encryption Key Low Byte"));

    ResponseCmndDone();
}

void CmndPrintConfLora(void){
    ResponseStructContainer c = LoraSerial.LoraSerial->getConfiguration();
    Configuration configuration = *(Configuration*) c.data;
    AddLog(LOG_LEVEL_INFO, "%s", c.status.getResponseDescription().c_str());

    if(c.status.code != SUCCESS){
        AddLog(LOG_LEVEL_INFO,PSTR("ERROR READ CONFIGTION"));
        ResponseCmndDone();
        return;
    }
    printParameters(configuration);
    ResponseCmndDone();
}
void CmndSetConfLora(void) {
    if (XdrvMailbox.data_len == 0) {
        AddLog(LOG_LEVEL_INFO, PSTR("No data for config"));
        ResponseCmndDone();
        return;
    }
    ResponseStructContainer c;
	c = LoraSerial.LoraSerial->getConfiguration();
	// It's important get configuration pointer before all other operation
	Configuration configuration = *(Configuration*) c.data;
    StaticJsonDocument<256> mailboxDoc;
    DeserializationError error = deserializeJson(mailboxDoc, XdrvMailbox.data);
    if (error) {
        AddLog(LOG_LEVEL_INFO, PSTR("ERROR TO PARSE JSON"));
        ResponseCmndDone();
        return;
    }

    if (mailboxDoc.containsKey("ADDH")) 
    {
        configuration.ADDH = mailboxDoc["ADDH"];
        device_info.addrHigh =  mailboxDoc["ADDH"];
    }
    if (mailboxDoc.containsKey("ADDL")){
        configuration.ADDL = mailboxDoc["ADDL"];
        device_info.addrLow = mailboxDoc["ADDL"];
    }
    if(mailboxDoc.containsKey("TAR_ADDH")) device_info.target_addrHigh = mailboxDoc["TAR_ADDH"];
    if(mailboxDoc.containsKey("TAR_ADDL")) device_info.target_addrHigh = mailboxDoc["TAR_ADDL"];
    if (mailboxDoc.containsKey("CHAN")) configuration.CHAN = mailboxDoc["CHAN"];

    if (mailboxDoc.containsKey("uartBaudRate")) {
        uint8_t baud = mailboxDoc["uartBaudRate"];
        if (baud == UART_BPS_1200 || baud == UART_BPS_2400 || baud == UART_BPS_4800 ||
            baud == UART_BPS_9600 || baud == UART_BPS_19200 || baud == UART_BPS_38400 ||
            baud == UART_BPS_57600 || baud == UART_BPS_115200) {
            configuration.SPED.uartBaudRate = baud;
        }
    }

    if (mailboxDoc.containsKey("airDataRate")) configuration.SPED.airDataRate = mailboxDoc["airDataRate"];
    if (mailboxDoc.containsKey("uartParity")) configuration.SPED.uartParity = mailboxDoc["uartParity"];
    if (mailboxDoc.containsKey("transmissionPower")) configuration.OPTION.transmissionPower = mailboxDoc["transmissionPower"];

    if (mailboxDoc.containsKey("fixedTransmission")) configuration.OPTION.fixedTransmission = mailboxDoc["fixedTransmission"];
	  ResponseStatus rs = LoraSerial.LoraSerial->setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
	AddLog(LOG_LEVEL_INFO, "%s", c.status.getResponseDescription().c_str());

    if(c.status.code== SUCCESS){
        AddLog(LOG_LEVEL_INFO, PSTR("Lora config updated"));
    }
    else{
        AddLog(LOG_LEVEL_INFO, PSTR("ERROR TO SET CONF LORA"));
    }
    c.close();
    ResponseCmndDone();
}
void CmndSendLoraTelemetry(void)
{
    if (XdrvMailbox.data_len == 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("No data to transmit"));
        ResponseCmndDone();
        return;
    }

    StaticJsonDocument<256> doc;
    JsonArray arr = doc.createNestedArray(device_info.device_name);
    JsonObject data = arr.createNestedObject();

    StaticJsonDocument<128> mailboxDoc;
    DeserializationError error = deserializeJson(mailboxDoc, XdrvMailbox.data);
    if (error)
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("JSON parse error"));
        ResponseCmndDone();
        return;
    }
    for (JsonPair p : mailboxDoc.as<JsonObject>())
    {
        data[p.key()] = p.value();
    }
    String json_string = "";
    serializeJson(doc, json_string);
    int len = json_string.length();
    // ResponseStatus rs = LoraSerial.LoraSerial->sendFixedMessage(device_info.target_addrHigh,device_info.target_addrLow,device_info.channel,json_string);
    ResponseStatus rs = LoraSerial.LoraSerial->sendMessage(json_string);
    AddLog(LOG_LEVEL_INFO, PSTR("LoRa Send: %s : %s, with len is %d"), rs.getResponseDescription().c_str(),json_string, len);
    ResponseCmndDone();
}

void CmndPrintMapLora(void)
{
    AddLog(LOG_LEVEL_INFO, PSTR("===== Lora MAPPING ====="));

    for (JsonPair p : Lora_mapping_ojb)
    {
        const char *key = p.key().c_str();                // "V1", "V2", ...
        const char *value = p.value().as<const char *>(); // "DHT11-Temperature", ...
        AddLog(LOG_LEVEL_INFO, PSTR("%s => %s"), key, value);
    }
    AddLog(LOG_LEVEL_INFO, PSTR("========================="));
}
void CmndSendLoraAttribute(void)
{
    StaticJsonDocument<256> doc;
    JsonObject deviceA = doc.createNestedObject(device_info.device_name);
    deviceA["addrHigh"] = device_info.addrHigh;
    deviceA["addrLow"] = device_info.addrLow;
    deviceA["channel"] = device_info.channel;
    deviceA["longitude"] = device_info.longitude;
    deviceA["latitude"] = device_info.latitude;
    String json_string;
    serializeJson(doc, json_string);
    int len = json_string.length();
    ResponseStatus rs = LoraSerial.LoraSerial->sendMessage(json_string);
    AddLog(LOG_LEVEL_INFO, PSTR("LoRa Send: %s : %s, with len is %d"), rs.getResponseDescription().c_str(),json_string.c_str(), len);
    // ResponseStatus rs = LoraSerial.LoraSerial->sendFixedMessage(device_info.target_addrHigh,device_info.target_addrLow,device_info.channel,json_string);
    // AddLog(LOG_LEVEL_INFO, rs.getResponseDescription().c_str());
    ResponseCmndDone();
}
void CmndSendLora(void)
{
    if (XdrvMailbox.data_len == 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("Nothing to transmit"));
        ResponseCmndDone();
        return;
    }
    char *tran = XdrvMailbox.data;
    AddLog(LOG_LEVEL_INFO, PSTR("Transmit data: %s"), tran);
    ResponseStatus rs = LoraSerial.LoraSerial->sendMessage(tran);
    AddLog(LOG_LEVEL_INFO, rs.getResponseDescription().c_str());
    ResponseCmndDone();
}

void LoraSerial_COLLECT_DATA()
{

    ResponseClear();
    XsnsCall(FUNC_JSON_APPEND);
    const char *raw = ResponseData();
    //  Bắt lỗi dấu phẩy ở đầu
    String fixed = raw;
    if (fixed.startsWith(","))
    {
        fixed = fixed.substring(1); // Bỏ dấu phẩy đầu
    }
    if(fixed == ""){
        AddLog(LOG_LEVEL_INFO,PSTR("No data to tran"));
        return;
    }
    fixed = "{" + fixed + "}"; // Bọc thành JSON hoàn chỉnh
    int ran = random(0,2000);
    delay(ran);
    AddLog(LOG_LEVEL_INFO, PSTR("Sensor JSON fixed: %s"), fixed.c_str());

    // Parse JSON
    StaticJsonDocument<256> input_doc;
    DeserializationError err = deserializeJson(input_doc, fixed);
    if (err)
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("Sensor JSON parse error: %s"), err.c_str());
        return;
    }

    // Gói lại JSON kiểu {"Device":[{...}]}
    StaticJsonDocument<512> out_doc;
    JsonArray arr = out_doc.createNestedArray(String(device_info.device_name));
    JsonObject data = arr.createNestedObject();
    int v_count = 1; // Đếm số biến V
    for (JsonPair p : input_doc.as<JsonObject>())
    {
        const char *sensorName = p.key().c_str(); // "DHT11"
        JsonObject inner = p.value().as<JsonObject>();
        for (JsonPair q : inner)
        {
            String sensor_sub = String(sensorName) + "-" + q.key().c_str();
            // Tạo key dạng V1, V2, ...
            String v_key = "V" + String(v_count);
            Lora_mapping_ojb[v_key] = sensor_sub;
            // data[v_key] = q.value();
            data[q.key().c_str()] = q.value();
            v_count++;
        }
    }
    
    String final_payload = "";
    serializeJson(out_doc, final_payload);
    int len = final_payload.length();

    // ResponseStatus rs = LoraSerial.LoraSerial->sendFixedMessage(device_info.addrHigh,device_info.addrLow,device_info.channel,final_payload);
    ResponseStatus rs = LoraSerial.LoraSerial->sendMessage(final_payload);
    AddLog(LOG_LEVEL_INFO, PSTR("LoRa Send: %s : %s, with len is %d"), rs.getResponseDescription().c_str(),final_payload.c_str(), len);
}

void LoraSerialProcessing()
{
    if(!LoraSerial.active) return;

    if(LoraSerial.LoraSerial -> available() > 1)
    {
        ResponseContainer rc = LoraSerial.LoraSerial -> receiveMessage();
        if(rc.status.code == 1)
        {
            String receive_data = rc.data;
            AddLog(LOG_LEVEL_INFO, PSTR("Receive Mess: "));
            AddLog(LOG_LEVEL_INFO, PSTR(receive_data.c_str()));
            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, receive_data);
            if(!err){
                if (doc.containsKey("device") && doc.containsKey("data")) {
                    JsonObject data = doc["data"];

                    if (data.containsKey("id") && data.containsKey("method") && data.containsKey("params")) {

                        String device = doc["device"].as<String>();
                        if (device != device_info.device_name) {
                            AddLog(LOG_LEVEL_INFO, PSTR("RPC is not for us"));
                            return;
                        }
                        int id = data["id"];
                        String method = data["method"].as<String>();
                        String param = data["params"].as<String>();
                        AddLog(LOG_LEVEL_INFO, PSTR("Method: %s | Value: %s"), method.c_str(), param.c_str());

                        char command[64];
                        snprintf(command, sizeof(command), "%s %s", method.c_str(), param.c_str());
                        ExecuteCommand(command, SRC_SERIAL);

                    } else {
                        AddLog(LOG_LEVEL_INFO, PSTR("Invalid RPC: missing id, method, or params"));
                    }
                } else {
                    AddLog(LOG_LEVEL_INFO, PSTR("Invalid RPC: missing device or data"));
                }
            }
            else{
                AddLog(LOG_LEVEL_ERROR,PSTR("Error parse Json"));
                return;
            }
        }
    }
}
bool Xdrv128(uint32_t function)
{
    bool result = false;
    if(FUNC_PRE_INIT == function)
    {
        LoraSerialInit();
    }
    else if(LoraSerial.active)
    {
        switch(function)
        {
            case FUNC_ACTIVE:
                result = true;
                break;
            case FUNC_EVERY_250_MSECOND:
                LoraSerialProcessing();
                break;
            case FUNC_COMMAND:
                result = DecodeCommand(kLoraSerialCommands,LoraSerialCommand);
                break;
            case FUNC_AFTER_TELEPERIOD:
                LoraSerial_COLLECT_DATA();
        }
    }
    return result;
}
#endif // USE_LORA_UART
