import json
import string

# Cấu hình Lora mặc định
var lora_config = {
  "channel": 20,
  "addrHigh": 0x1,
  "addrLow": 0x2,
  "Baudrate": 3,
  "fixedTransmission": 0
}

# Bảng map cho 10 giá trị telemetry: V1 ... V10
var telemetry_map = {
  "V1":   nil,
  "V2":   nil,
  "V3":   nil,
  "V4":   nil,
  "V5":   nil,
  "V6":   nil,
  "V7":   nil,
  "V8":   nil,
  "V9":   nil,
  "V10":  nil
}

#####################################
# Các hàm setup cấu hình Lora       #
#####################################

def setChannel(val)
  lora_config["channel"] = val
  print(f"Channel is: {val}, please beginLora to set")
end

def setAddrHigh(val)
  lora_config["addrHigh"] = val
  print(f"AddrHigh is: {val}, please beginLora to set")
end

def setAddrLow(val)
  lora_config["addrLow"] = val
  print(f"AddrLow is: {val}, please beginLora to set")
end

def setBaudrate(val)
  var baudrate_map = {
    "1200":   0,
    "2400":   1,
    "4800":   2,
    "9600":   3,
    "19200":  4,
    "38400":  5,
    "57600":  6,
    "115200": 7
  }

  var found = false
  for key: baudrate_map.keys()
    if string(key) == string(val)
      lora_config["Baudrate"] = baudrate_map[key]
      print(f"Baudrate set to {val} (code {baudrate_map[key]}), please beginLora to set")
      found = true
    end
  end

  if found == false
    print("Invalid baudrate! Valid options: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200")
  end
end

def setFixedTransmission(val)
  lora_config["fixedTransmission"] = val
  print(f"FixedTransmission is: {val}, please beginLora to set")
end

def beginLora()
  var json_str = json.dump(lora_config)
  print("Sending BeginLora config: " + json_str)
  tasmota.cmd("BeginLora " + json_str)
end

#####################################
# Hàm đọc tất cả sensor từ Tasmota  #
#####################################
def getsensors()
  var sensors = json.load(tasmota.read_sensors())
  if sensors == nil || type(sensors) != 'instance'
    return nil
  end

  # Flatten tất cả (key:subkey -> value)
  var ressen = {}
  for entry: sensors.keys()
    if type(sensors[entry]) == 'instance'
      for subentry: sensors[entry].keys()
        ressen[entry + '-' + subentry] = sensors[entry][subentry]
      end
    end
  end

  if ressen.size() > 0
    return ressen
  else
    return nil
  end
end

#####################################
# Quản lý telemetry V1..V10         #
#####################################

# Thêm map: Vx -> sensorKey
def Add(vKey, sensorKey)
  # Nếu muốn linh hoạt, ta có thể ép "1" -> "V1", v.v. 
  # Ở đây ta giả sử đầu vào đã là "V1", "V2", ...
  var available_sensors = getsensors()
  if available_sensors == nil
    print("No sensors available; cannot add telemetry!")
    return false
  end

  # Kiểm tra sensorKey có trong list sensors không
  if available_sensors.find(sensorKey) != nil
    telemetry_map[vKey] = sensorKey
    print(f"Mapping {vKey} to sensor '{sensorKey}'")
  else
    print(f"Error: '{sensorKey}' not found in sensor list!")
    print("Available sensors: " + json.dump(available_sensors))
  end
end

#####################################
# Gửi data qua Lora (cron)          #
#####################################

def sendLoraBerry()
    var available_sensors = getsensors()
    if available_sensors == nil
        print("No sensors found, nothing to send!")
        return
    end

    # Xây dict telemetry_out = { "V1": <value>, "V2": <value>, ... }
    var vx_order = ["V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "V10"]
    var telemetry_out = {}

    for key: vx_order
        var sensorKey = telemetry_map[key]
        if sensorKey != nil && available_sensors.find(sensorKey) != nil
            telemetry_out[key] = available_sensors[sensorKey]
        else
            telemetry_out[key] = nil
        end
    end
    var json_str = "{"
    var first = true
    for k: vx_order
    if !first
        json_str += ","
    else
        first = false
    end
    json_str += "\"" + k + "\":" + json.dump(telemetry_out[k])
    end

    json_str += "}"
    print("Sending LoRa Telemetry: " + json_str)
    tasmota.cmd("SendLoraTelemetry " + json_str)
end

# Tự động gọi mỗi 10 giây (có thể giữ hoặc bỏ tuỳ bạn)
tasmota.add_cron("*/10 * * * * *", sendLoraBerry, "every_10_seconds")
