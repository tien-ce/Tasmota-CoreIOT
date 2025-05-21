#include "RS485_driver.h"
#define MODBUS_FUNC_READ_HOLDING_REG     0x03  // Đọc thanh ghi giữ (Read Holding Registers)
#define MODBUS_FUNC_WRITE_SINGLE_REG     0x06  // Ghi 1 thanh ghi (Write Single Register)
#define MODBUS_FUNC_WRITE_MULTI_REG      0x10  // Ghi nhiều thanh ghi (Write Multiple Registers)

/******************************* Lớp điều khiển giao tiếp RS485.**************************/
RS485_Driver::RS485_Driver() {
    this->rs485ModBus = nullptr;
    this->uart = nullptr;
    this->active = false;
}
RS485_Driver rs485;

/**
 * Khởi tạo giao tiếp RS485.
 * @param uart Con trỏ tới cấu hình UART.
 */
void RS485_Driver::begin(UART* uart) {
    if (uart != nullptr) {
        this->uart = uart;
        this->rs485ModBus = new TasmotaModbus(this->uart->rxPin, this->uart->txPin);
        uint8_t result = this->rs485ModBus->begin(this->uart->baudrate, this->uart->config);
        if (result) {
            this->active = true;
        } else {
            this->active = false;
        }
    } else {
        this->rs485ModBus = nullptr;
        this->active = false;
    }
}

/**
 * Kiểm tra giao tiếp RS485 đã khởi tạo chưa.
 * @return true nếu đã khởi tạo, false nếu chưa.
 */
bool RS485_Driver::IsBegin() {
    return this->active;
}

/**
 * Đọc thanh ghi từ thiết bị RS485.
 * @param device_addr Địa chỉ thiết bị.
 * @param startAddress Địa chỉ bắt đầu đọc.
 * @param count Số lượng thanh ghi cần đọc.
 * @return Mã trạng thái trả về từ thiết bị (0 nếu OK).
 */
uint8_t RS485_Driver::ReadRegister(uint8_t device_addr, uint16_t startAddress, uint16_t count) {
    return this->rs485ModBus->Send(device_addr, MODBUS_FUNC_READ_HOLDING_REG, startAddress, count);
}

/**
 * Ghi dữ liệu vào thanh ghi của thiết bị RS485.
 * @param device_addr Địa chỉ thiết bị.
 * @param startAddress Địa chỉ bắt đầu ghi.
 * @param num Số lượng thanh ghi cần ghi.
 * @param write_data Mảng dữ liệu cần ghi.
 */
void RS485_Driver::WriteRegister(uint8_t device_addr, uint16_t startAddress, int num, uint16_t* write_data) {
    this->rs485ModBus->Send(device_addr, MODBUS_FUNC_WRITE_SINGLE_REG, startAddress, num, write_data);
}

/**
 * Nhận phản hồi từ thiết bị RS485.
 * @param buffer Bộ đệm lưu dữ liệu phản hồi.
 * @param lenght Độ dài dữ liệu cần đọc.
 * @return Mã trạng thái nhận dữ liệu (0 nếu OK).
 */
uint8_t RS485_Driver::ReceiveRespone(uint8_t* buffer, int lenght) {
    return this->rs485ModBus->ReceiveBuffer(buffer, lenght);
}

/**
 * Nhận payload dữ liệu.
 * @param receive_payload Bộ đệm để lưu payload nhận được.
 * @param len Số byte payload cần lấy.
 * @return Mã lỗi, 0 nếu thành công.
 */
uint8_t RS485_Driver::ReceivePayload(uint8_t* receive_payload, int len) {
    uint8_t buffer[8];
    uint8_t err = this->ReceiveRespone(buffer, 8);
    if (err == 0) {
        for (int i = 0; i < len; i++) {
            receive_payload[i] = buffer[i + 3];
        }
    }
    return err;
}
/**************** TASMOTA MODBUS *********** */
    /* Return codes:
     * 0 = No error
     * 1 = Illegal Function,
     * 2 = Illegal Data Address,
     * 3 = Illegal Data Value,
     * 4 = Slave Error
     * 5 = Acknowledge but not finished (no error)
     * 6 = Slave Busy
     * 7 = Not enough minimal data received
     * 8 = Memory Parity error
     * 9 = Crc error
     * 10 = Gateway Path Unavailable
     * 11 = Gateway Target device failed to respond
     * 12 = Wrong number of registers
     * 13 = Register data not specified
     * 14 = To many registers
     */
#define NPKMODBUS_FUNC_READ_HOLDING_REG 0x03 



/********************************************************************** */
/**
 * Lớp quản lý danh sách các cảm biến đã phát hiện trên bus RS485.
 */
/**
 * Lấy danh sách địa chỉ các cảm biến đã phát hiện.
 */
std::vector<uint16_t> DetectedSensors::GetSensorList() {
    return this->addressDetecedList;
}
DetectedSensors detectedSensors;
