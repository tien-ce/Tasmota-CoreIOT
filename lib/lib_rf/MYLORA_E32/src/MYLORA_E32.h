#ifndef MY_LORA_E32_H
#define MY_LORA_E32_H
#ifndef ESP32
#define ESP32
#endif
#include "LoRa_E32.h"
#include "ArduinoJson.h"
#include "HardwareSerial.h"
#include "employess.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_common.h"
/***************************ENCODE - DECODE********************************** */
bool encode_string(pb_ostream_t *stream ,const pb_field_t *field,void * const *arg);
bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool encode_byte(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
bool decode_byte(pb_istream_t *stream, const pb_field_t *field, void **arg);
/**************************************************************************** */
class Device_info{
    public:
    String device_name;
    uint8_t channel; // kênh truyền
    uint8_t addrHigh; // Địa chỉ cao
    uint8_t addrLow; // Địa chỉ thấp
    float longitude; // Kinh độ 
    float latitude; // Vĩ độ
    Device_info(){
        this->device_name = "";
    }
    Device_info(String device_name){
        this->device_name = device_name;    
    }
    Device_info(String device_name,float longitude,float latitude){
        this->device_name = device_name;
        this->longitude = longitude;
        this->latitude = latitude;
    }
    void setLoraName(String device_name){
        this->device_name = device_name;
    }
};
// Đối tượng chứa thông tin thiết bị
extern Device_info device_info;
// Định nghĩa chân sử dụng cho LoRa E32
#define MY_LORA_RX 7 // RX của ESP32 (gắn với TX của LoRa)
#define MY_LORA_TX 6 // TX của ESP32 (gắn với RX của LoRa)
// Khai báo đối tượng HardwareSerial (sẽ được định nghĩa trong MY_LORA_E32.cpp)
extern HardwareSerial LoraSerial;

// Khai báo đối tượng LoRa_E32 (sẽ được định nghĩa trong MY_LORA_E32.cpp)
extern LoRa_E32 my_lora_e32;

// Thông tin thiết bị
// Khai báo các hàm cấu hình và in thông số
void configMyLoraE32(uint8_t channel, uint8_t addrHigh, uint8_t addrLow, uint8_t baudRate = 3,uint8_t fixedTransmission = 0);
void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);
void processRpcCommand(const char* jsonMessage);
#endif // MY_LORA_E32_H
