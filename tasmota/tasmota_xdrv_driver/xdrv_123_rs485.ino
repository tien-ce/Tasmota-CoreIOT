#include "my_user_config.h"
#ifdef USE_RS485

#define XDRV_123 123
#warning **** RS485 Driver is included... ****
#include <TasmotaModbus.h>
#include "ArduinoJson.h"
#include "RS485_driver.h"
#define MAX_SENSORS 100
/********************   Detected Sensor ************************** */
void DetectedSensors::PrintListSensor() {
    if (this->addressDetecedList.empty()) {
        AddLog(LOG_LEVEL_INFO, PSTR("No address detected"));
        return;
    }
    for (uint16_t it : this->addressDetecedList) {
        AddLog(LOG_LEVEL_INFO, PSTR("Detected Sensor List: 0x%04X"), it);
    }
}

/**
 * Tìm kiếm và phát hiện cảm biến trên dải địa chỉ chỉ định.
 */
void DetectedSensors::DetectSensor(uint16_t startAddress, uint16_t endAddress, uint16_t RegisterAddr) {
    for (uint16_t i = startAddress; i <= endAddress; i++) {
        uint8_t result = rs485.ReadRegister(i, RegisterAddr, 0x0001);
        delay(200);
        if (result == 0) {
            uint8_t buffer[8];
            uint8_t err = rs485.ReceiveRespone(buffer, 7);
            if (err) {
                AddLog(LOG_LEVEL_INFO, PSTR("[DEBUG] err: %d at 0x%04X"), err, i);
            } else {
                char hexString[7 * 5 + 1] = {0};
                for (int j = 0; j < 7; j++) {
                    snprintf(&hexString[j * 5], 6, "0x%02X ", buffer[j]);
                }
                AddLog(LOG_LEVEL_INFO, PSTR("Raw Payload: %s"), hexString);
            }

            uint16_t addr = (buffer[3] << 8) | buffer[4];
            if (addr == i) {
                AddLog(LOG_LEVEL_INFO, PSTR("Detected Address: 0x%04X"), i);
                this->addressDetecedList.push_back(i);
                return;
            }
        }
    }
}

/*************************************************************************************** */
void Rs485Init(void){
    UART uart(6,7,4800,SERIAL_8N1);
    rs485.begin(&uart);
    if(rs485.IsBegin()){
        AddLog(LOG_LEVEL_INFO, PSTR("RS485: RS485 using GPIO%d(RX) and GPIO%d(TX)"),6, 7);
    }
    else{
        AddLog(LOG_LEVEL_ERROR, PSTR("RS485 BEGIN FAILED"));
    }
}
bool Xdrv123(uint32_t function)
{
    bool result = false;
    if(FUNC_PRE_INIT == function)
    {
        Rs485Init();
        if(rs485.IsBegin()){
            AddLog(LOG_LEVEL_INFO,PSTR("RS485 is init"));
        } 
    }
    else if(rs485.IsBegin())
    {
        switch (function)
        {
            case FUNC_EVERY_SECOND:
                detectedSensors.PrintListSensor();
                break;
        }
    }
    return result;
}
#endif
