#ifndef RS485_driver
#define RS485_driver
#include <TasmotaModbus.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
/**
 * Cấu trúc UART chứa thông tin cấu hình giao tiếp UART.
 */
struct UART {
    int rxPin;          ///< Chân RX của UART
    int txPin;          ///< Chân TX của UART
    long baudrate;      ///< Tốc độ baud rate
    uint32_t config;    ///< Cấu hình UART (parity, stop bits, v.v.)
};

/*
 * Lớp điều khiển giao tiếp RS485
 * 
 * Mô tả:
 * - Đóng vai trò trung gian xử lý việc gửi và nhận dữ liệu thông qua bus RS485.
 * - Dựa trên thư viện TasmotaModbus để thực hiện giao tiếp Modbus RTU.
 * - Cho phép khởi tạo UART, gửi lệnh đọc/ghi thanh ghi và nhận dữ liệu phản hồi.
 * 
 * Thành viên dữ liệu:
 * - rs485ModBus : Con trỏ tới đối tượng TasmotaModbus quản lý giao tiếp RS485.
 * - uart        : Con trỏ tới cấu hình UART được sử dụng.
 * - active      : Cờ trạng thái cho biết giao tiếp đã được khởi tạo thành công hay chưa.
 * 
 * Phương thức:
 * - RS485_Driver()                                         : Hàm khởi tạo mặc định.
 * - begin(UART* uart)                                      : Khởi tạo giao tiếp RS485 với thông số UART.
 *      @param uart Con trỏ tới cấu hình UART cần sử dụng.
 * - IsBegin()                                              : Kiểm tra giao tiếp đã được khởi tạo chưa.
 *      @return true nếu đã sẵn sàng, false nếu chưa.
 * - ReadRegister(uint8_t, uint16_t, uint16_t)              : Đọc giá trị từ thanh ghi Modbus.
 *      @param device_addr Địa chỉ thiết bị.
 *      @param startAddress Địa chỉ bắt đầu.
 *      @param count Số lượng thanh ghi cần đọc.
 *      @return Mã trạng thái trả về từ thiết bị (0 nếu OK).
 * - WriteRegister(uint8_t, uint16_t, int, uint16_t*)       : Ghi dữ liệu vào thanh ghi thiết bị.
 *      @param device_addr Địa chỉ thiết bị.
 *      @param startAddress Địa chỉ bắt đầu ghi.
 *      @param num Số lượng thanh ghi cần ghi.
 *      @param write_data Mảng dữ liệu cần ghi.
 * - ReceiveRespone(uint8_t* buffer, int lenght)            : Nhận phản hồi từ thiết bị.
 *      @param buffer Bộ đệm chứa dữ liệu phản hồi.
 *      @param lenght Số byte cần đọc.
 *      @return Mã lỗi (0 nếu OK).
 * - ReceivePayload(uint8_t* receive_payload, int len)      : Trích xuất dữ liệu payload từ phản hồi.
 *      @param receive_payload Bộ đệm lưu dữ liệu payload.
 *      @param len Số byte payload cần lấy.
 *      @return Mã lỗi (0 nếu OK).
 */

class RS485_Driver {
private:
    TasmotaModbus* rs485ModBus;
    UART* uart;
    bool active;

public:
    RS485_Driver();

    void begin(UART* uart);
    bool IsBegin();
    uint8_t ReadRegister(uint8_t device_addr, uint16_t startAddress, uint16_t count);
    void WriteRegister(uint8_t device_addr, uint16_t startAddress, int num, uint16_t* write_data);
    uint8_t ReceiveRespone(uint8_t* buffer, int lenght);
    uint8_t ReceivePayload(uint8_t* receive_payload, int len);
};
extern RS485_Driver rs485;

/*
 * Lớp quản lý danh sách cảm biến RS485 đã phát hiện
 * 
 * Mô tả:
 * - Thực hiện tìm kiếm cảm biến đang hoạt động trên bus RS485 bằng cách quét qua dải địa chỉ.
 * - Lưu danh sách các thiết bị tìm thấy vào vector để xử lý sau.
 * - Cung cấp khả năng in ra log các thiết bị đã phát hiện.
 * 
 * Thành viên dữ liệu:
 * - num_device         : Số lượng cảm biến (chưa sử dụng trong logic hiện tại).
 * - addressDetecedList : Danh sách địa chỉ các cảm biến đã phát hiện.
 * 
 * Phương thức:
 * - GetSensorList()                                          : Trả về danh sách các địa chỉ đã phát hiện.
 *      @return Vector chứa địa chỉ cảm biến.
 * - PrintListSensor()                                        : Ghi log danh sách cảm biến đã tìm thấy.
 * - DetectSensor(uint16_t, uint16_t, uint16_t RegisterAddr)  : Quét bus để phát hiện cảm biến.
 *      @param startAddress Địa chỉ bắt đầu quét.
 *      @param endAddress Địa chỉ kết thúc quét.
 *      @param RegisterAddr Địa chỉ thanh ghi chứa địa chỉ thiết bị để xác minh.
 */

class DetectedSensors {
private:
    int num_device;
    std::vector<uint16_t> addressDetecedList;

public:
    std::vector<uint16_t> GetSensorList();
    void PrintListSensor();
    void DetectSensor(uint16_t startAddress, uint16_t endAddress, uint16_t RegisterAddr);
};

extern DetectedSensors detectedSensors;
/* 
 * Lớp phương thức cảm biến cho RS485 (Abstract Class)
 * 
 * Mô tả:
 * - Định nghĩa giao diện trừu tượng cho các loại cảm biến giao tiếp qua RS485.
 * - Các lớp con cần hiện thực đầy đủ các phương thức thuần ảo để xử lý cảm biến cụ thể.
 * 
 * Thành viên dữ liệu:
 * - nameSensor    : Tên loại cảm biến.
 * - address       : Địa chỉ thiết bị RS485.
 * - listKey       : Danh sách các khóa dữ liệu (các giá trị cảm biến).
 * - startAddress  : Địa chỉ thanh ghi bắt đầu.
 * - endAddress    : Địa chỉ thanh ghi kết thúc.
 * - isDetected    : Cờ kiểm tra thiết bị đã được phát hiện hay chưa.
 * 
 * Phương thức:
 * - init()            : Khởi tạo cảm biến.
 * - readPayload()     : Đọc dữ liệu từ cảm biến.
 * - getPayLoad()      : Lấy dữ liệu dạng JSON.
 * - changeAddress()   : Thay đổi địa chỉ thiết bị.
 *      @param newAddress Địa chỉ mới (uint16_t).
 * - getListKey()      : Lấy danh sách các key của dữ liệu.
 */

class RS485_t {
protected:
    String nameSensor;
    uint16_t address;
    std::vector<String> listKey;
    uint16_t startAddress;
    uint16_t endAddress;
    bool isDetected;

public:
    RS485_t(){

    }
    
    virtual bool init() = 0;

    virtual void readPayload() = 0;

    /**
     * @return Đối tượng JsonObject chứa dữ liệu đo được.
     */
    virtual JsonObject getPayLoad() = 0;

    /**
     * @param newAddress Địa chỉ mới cần gán cho thiết bị RS485.
     */
    virtual void changeAddress(uint16_t newAddress) = 0;

    /**
     * @return Danh sách key của các dữ liệu cảm biến (vector<String>).
     */
    virtual std::vector<String> getListKey() = 0;
};
#endif
