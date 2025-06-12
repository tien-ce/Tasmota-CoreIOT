#include "my_user_config.h"
#ifdef USE_RS485
#ifdef USE_NPK_RS485
#warning **** RS485 NPK is included... ****
#define XSNS_120 120
#include "RS485_driver.h"
// ==== Real-time content values (Read-only) ====
#define REG_ADDR_NITROGEN_CONTENT            0x001E  // 40031: Nitrogen content (read-only)
#define REG_ADDR_PHOSPHORUS_CONTENT          0x001F  // 40032: Phosphorus content (read-only)
#define REG_ADDR_POTASSIUM_CONTENT           0x0020  // 40033: Potassium content (read-only)

// ==== Nitrogen Coefficient (Read/Write, IEEE754) ====
#define REG_ADDR_N_COEFF_HIGH                0x03E9  // 41001: Nitrogen coefficient high 16 bits
#define REG_ADDR_N_COEFF_LOW                 0x03EA  // 41002: Nitrogen coefficient low 16 bits
#define REG_ADDR_N_CALIBRATION               0x03EB  // 41003: Calibration value (Integer)

// ==== Phosphorus Coefficient (Read/Write, IEEE754) ====
#define REG_ADDR_P_COEFF_HIGH                0x03F2  // 41011: Phosphorus coefficient high 16 bits
#define REG_ADDR_P_COEFF_LOW                 0x03F3  // 41012: Phosphorus coefficient low 16 bits
#define REG_ADDR_P_CALIBRATION               0x03F4  // 41013: Calibration value (Integer)

// ==== Potassium Coefficient (Read/Write, IEEE754) ====
#define REG_ADDR_K_COEFF_HIGH                0x03FC  // 41021: Potassium coefficient high 16 bits
#define REG_ADDR_K_COEFF_LOW                 0x03FD  // 41022: Potassium coefficient low 16 bits
#define REG_ADDR_K_CALIBRATION               0x03FE  // 41023: Calibration value (Integer)

// ==== Device Settings ====
#define REG_ADDR_DEVICE_ADDRESS              0x07D0  // 42001: Device address (1-254), default 1
#define REG_ADDR_DEVICE_BAUDRATE             0x07D1  // 42002: Baud rate (0 = 2400, 1 = 4800, 2 = 9600)

/*******************************************************
 * CRC Code: Two-byte check code (Checksum)
 * 
 * Host Inquiry Frame Structure:
 * ┌─────────┬──────────────┬──────────────────┬────────────────┬──────────────────┬──────────────────┐
 * │ Address │  Function    │ Register Start   │ Register       │ Checksum Low     │ Checksum High    │
 * │ Code    │  Code        │ Address (2 bytes)│ Length (2 bytes)│ Byte (1 byte)    │ Byte (1 byte)    │
 * └─────────┴──────────────┴──────────────────┴────────────────┴──────────────────┴──────────────────┘
 * |   1B    |     1B       |       2B          |       2B       |        1B         |        1B         |
 * 
 * Slave Response Frame Structure:
 * ┌─────────┬──────────────┬──────────────────┬───────────┬───────────┬────────────┬──────────────────┬──────────────────┐
 * │ Address │  Function    │ Number of Valid  │ Data Area │ Data Area │ Data N Area│ Checksum Low     │ Checksum High    │
 * │ Code    │  Code        │ Bytes (1 byte)   │   (2B)    │   2B      │    2B      │ Byte (1 byte)    │ Byte (1 byte)    │
 * └─────────┴──────────────┴──────────────────┴───────────┴───────────┴────────────┴──────────────────┴──────────────────┘
 * |   1B    |     1B       |       1B          |     2B    |    2B     |    ...     |        1B         |        1B         |
 * 
 * Notes:
 * - Address Code: Slave device address.
 * - Function Code: Specifies the function (e.g., Read, Write).
 * - Register Start Address: Starting address of the target register (2 bytes).
 * - Register Length: Number of registers to read/write (2 bytes).
 * - Number of Valid Bytes: Indicates how many data bytes are valid in response.
 * - Checksum: CRC16, Low byte first, then High byte.
 *******************************************************/
class RS485_NPK : public RS485_t{
    private:
        int16_t payload[3];
    public:
        RS485_NPK() {
            this->nameSensor = "NPK Sensor";
            this->startAddress = 0x0001;
            this->endAddress  = 0x0010;
            this->listKey = {"N","P","K"};
        }
        bool init() override{
            if(!rs485.IsBegin()){
                return false;
            }
            detectedSensors.DetectSensor(this->startAddress,this->endAddress,REG_ADDR_DEVICE_ADDRESS);
            std::vector<uint16_t> listDetected = detectedSensors.GetSensorList();
            this->isDetected = false;
        #ifdef USE_DEBUG
            if(listDetected.empty()){
                AddLog(LOG_LEVEL_INFO,PSTR("List Detected is empty"));
            }
        #endif
            for(uint16_t it : listDetected){
                AddLog(LOG_LEVEL_INFO,PSTR("it : %04X"), it);
                if(it >= this->startAddress && it <= this->endAddress){
                    this->address = it;
                    this->isDetected = true;
                    AddLog(LOG_LEVEL_INFO,PSTR("NPK Sensor is detected"));
                    break;
                }
            }
            return this->isDetected;
        }

        void readPayload(bool *success) override{
            uint8_t err = rs485.ReadRegister(this->address,REG_ADDR_NITROGEN_CONTENT,0x0003);
            if(err){
#ifdef USE_DEBUG
                AddLog(LOG_LEVEL_ERROR, PSTR("[DEBUG] READ NPK err : %d"),err);
#endif
                *(success) = false;
                return;
            }
            else{
                delay(20);
                uint8_t respone[12]; // Nitro : 16 bit , Kali : 16 bit , PhotPho 16 bit
                err = rs485.ReceiveRespone(respone,11);
                if(err){
#ifdef USE_DEBUG
                    AddLog(LOG_LEVEL_ERROR,PSTR("[DEBUG] RECEIVE NPK err : %d"),err);
#endif
                    *(success) = false;
                    return;
                }
                else{
                    // Bỏ qua 3 byte đầu (addr, func, byte count), lấy data từ index 3 trở đi
                    uint16_t nitrogen   = ((uint16_t)respone[3] << 8) | respone[4];// 0x10 0x20 , nitro = ( 0x0010 << 8 | 0x20) -> ( 0x1000 | 0x20 ) -> 0x1020
                    uint16_t phosphorus = ((uint16_t)respone[5] << 8) | respone[6];
                    uint16_t potassium  = ((uint16_t)respone[7] << 8) | respone[8];
                    this->payload[0] = nitrogen;
                    this->payload[1] = phosphorus;
                    this->payload[2] = potassium;
                    *(success) = true;
                }
            }
        }

        void setFalsePayload(){
            this->payload[0] = -1;
            this->payload[1] = -1;
            this->payload[2] = -1;
        }
        int16_t* getPayload() {
            return this->payload;
        }

        void changeAddress(uint16_t newAddress) override{

        }
        std::vector<String> getListKey() override{
            return this->listKey;
        }       
};
/***************** INIT *************** */
RS485_NPK  rs485NPK;
bool isInit = false;
void NPKInit(void){
    isInit = rs485NPK.init();
    if(isInit){
        AddLog(LOG_LEVEL_INFO,PSTR("NPK sensor is Init"));
    }
    else{
        AddLog(LOG_LEVEL_ERROR,PSTR("NPK sensor not Init"));
    }
}

/******************* READ Payload************** */
void NPKreadPayload(void){
    bool success = false;
    rs485NPK.readPayload(&success);
#ifdef USE_DEBUG
    if(success){
        AddLog(LOG_LEVEL_INFO, PSTR("[DEBUG] ReadNPK sucess"));
    }
    else{
        rs485NPK.setFalsePayload();
        AddLog(LOG_LEVEL_ERROR,PSTR("[DEBUG] ReadNPK Failed"));
    }
#endif
}
/************************ SHOW ******************** */
const char D_JSON_SOIL_NITROGEN[]   =   "Nitrogen";
const char D_JSON_SOIL_PHOSPHORUS[] =   "Phosphorus";
const char D_JSON_SOIL_POTASSIUM[]  =   "Kali";
const char HTTP_SNS_SM_NITRO[]       PROGMEM = "{s} %s {m} %d mg/kg";
const char HTTP_SNS_SM_PHOSPHORUS[]  PROGMEM = "{s} %s {m} %d mg/kg";
const char HTTP_SNS_SM_POTASSIUM[]   PROGMEM = "{s} %s {m} %d mg/kg";

void NPKShow(bool json){
    bool sucess = false;
    int16_t* payload = rs485NPK.getPayload();
    if (json) {
        ResponseAppend_P(PSTR(",\"%s\":{"), "NPK_Sensor");  // hoặc dùng biến tên nếu có
        ResponseAppend_P(PSTR("\"" "nitro" "\":%d,"), payload[0]);
        ResponseAppend_P(PSTR("\"" "photpho" "\":%d,"), payload[1]);
        ResponseAppend_P(PSTR("\"" "kali" "\":%d"), payload[2]);
        ResponseJsonEnd();
        
    }
#ifdef USE_WEBSERVER
    else
    {
        WSContentSend_PD(HTTP_SNS_SM_NITRO, D_JSON_SOIL_NITROGEN,payload[0]);
        WSContentSend_PD(HTTP_SNS_SM_PHOSPHORUS, D_JSON_SOIL_PHOSPHORUS,payload[1]);
        WSContentSend_PD(HTTP_SNS_SM_POTASSIUM, D_JSON_SOIL_POTASSIUM,payload[2]);
    }
#endif
}

/*************************************************** */
bool Xsns120(uint32_t function)
{
    bool result = false;
    if (FUNC_INIT == function)
    {
        delay(20);
        NPKInit();
    }
    else if (isInit)
    {
        switch (function)
        {
       case FUNC_EVERY_SECOND:
            NPKreadPayload();
            break;
        case FUNC_JSON_APPEND:
            NPKShow(1);
            break;
#ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
            NPKShow(0);
            break;
#endif
        }
    }
    return result;
}
#endif
#endif