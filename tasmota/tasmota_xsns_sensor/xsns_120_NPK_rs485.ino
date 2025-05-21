#include "my_user_config.h"
#ifdef USE_RS485
#ifdef USE_NPK_RS485
#warning **** RS485 NPK is included... ****
#define XSNS_120 120
#include "RS485_driver.h"
// ==== Real-time content values (Read-only) ====
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
    public:
        RS485_NPK() {
            this->nameSensor = "NPK Sensor";
            this->startAddress = 0x0001;
            this->endAddress  = 0x0010;
            this->listKey = {"N","P","K"};
        }
        bool init() override{
            detectedSensors.DetectSensor(this->startAddress,this->endAddress,REG_ADDR_DEVICE_ADDRESS);
            std::vector<uint16_t> listDetected = detectedSensors.GetSensorList();
            this->isDetected = false;
            for(uint16_t it : listDetected){
                if(it >= this->startAddress && it <= this->endAddress){
                    this->address = it;
                    this->isDetected = true;
                    break;
                }
            }
            return this->isDetected;
        }
        void readPayload() override{

        }
        JsonObject getPayLoad() override{
            return {};
        }
        void changeAddress(uint16_t newAddress) override{

        }
        std::vector<String> getListKey() override{
            return this->listKey;
        }       
};
RS485_NPK  rs485NPK;
bool isInit = false;
void NPKInit(void){
    isInit = rs485NPK.init();
}
bool Xsns120(uint32_t function)
{
    bool result = false;
    if (FUNC_INIT == function)
    {
        bool isInit = rs485NPK.init();
    }
    else if (isInit)
    {
        switch (function)
        {
            case FUNC_EVERY_SECOND:
                AddLog(LOG_LEVEL_INFO,PSTR("DETECTED NPK SENSOR"));
                break;
//        case FUNC_EVERY_250_MSECOND:
//             NPKReadData();
//             break;
//         case FUNC_JSON_APPEND:
//             NPKShow(1);
//             break;
// #ifdef USE_WEBSERVER
//         case FUNC_WEB_SENSOR:
//             NPKShow(0);
//             break;
// #endif
        }
    }
    return result;
}
#endif
#endif