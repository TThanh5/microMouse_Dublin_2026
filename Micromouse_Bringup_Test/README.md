# HƯỚNG DẪN BẢN CHẠY THỬ & KIỂM TRA PHẦN CỨNG (HACKATHON BRING-UP TEST)

Bản firmware này được thiết kế chuyên biệt cho buổi Hackathon để **kiểm tra toàn bộ phần cứng ngay sau khi lắp ráp xong** và **chạy thử robot (Pre-run)** trước khi bước vào tối ưu giải thuật mê cung.

Tất cả các định nghĩa phần cứng, chân GPIO, địa chỉ I2C đều tuân thủ 100% tài liệu chuẩn [`MICROMOUSE_MASTER_PROMPT.md`](../MICROMOUSE_MASTER_PROMPT.md).

---

## 1. Cấu trúc thư mục

Thư mục `Micromouse_Bringup_Test` chứa đầy đủ mã nguồn dạng module, Arduino IDE tự động mở tất cả các file này làm các Tab:

- `Micromouse_Bringup_Test.ino`: Chương trình chính, khởi tạo và menu điều khiển qua Serial.
- `config_pins.h`: Định nghĩa chân GPIO chuẩn cho ESP32-C6 và các thông số có thể tinh chỉnh nhanh (TBD parameters).
- `motors.h` / `motors.cpp`: Trình điều khiển motor TB6612 / DRI0044 (có cờ đảo chiều tiện lợi).
- `encoders.h` / `encoders.cpp`: Ngắt giải mã xung Encoder Trái / Phải.
- `sensors.h` / `sensors.cpp`: Quét I2C, khởi tạo tuần tự 3 cảm biến VL53L0X và đọc MPU-6050.
- `prerun.h` / `prerun.cpp`: Vòng lặp chạy thử tự hành (giữ tâm giữa 2 tường và dừng khi có tường trước).

---

## 2. Chuẩn bị trên Arduino IDE

1. **Cài đặt Board ESP32:**
   - Vào `Tools` -> `Board` -> `ESP32 Arduino` -> Chọn **ESP32-C6 Dev Module** (hoặc ESP32-C6-DevKitC-1).
2. **Cài đặt Thư viện:**
   - Vào `Sketch` -> `Include Library` -> `Manage Libraries...`
   - Tìm kiếm: `Adafruit_VL53L0X` -> Bấm **Install** (cài đặt kèm các phụ thuộc nếu được hỏi).
   *(Lưu ý: MPU-6050 đã được tích hợp driver I2C trực tiếp trong mã nguồn nên không bắt buộc phải cài thêm thư viện phụ cho IMU).*
3. **Nạp Code:**
   - Mở file `Micromouse_Bringup_Test.ino` bằng Arduino IDE.
   - Chọn đúng cổng COM của ESP32-C6.
   - Bấm **Upload**.

---

## 3. Quy trình kiểm tra 5 bước tại Hackathon (Theo Serial Monitor - 115200 baud)

Sau khi nạp, mở **Serial Monitor**, đặt tốc độ **115200 baud**. Bạn sẽ thấy Menu:

```text
========================================================
      MICROMOUSE ESP32-C6 BRING-UP & TEST RUN MENU     
========================================================
 [1] Run I2C Bus Scanner
 [2] Live ToF & IMU Monitor (Wave hand to test distance)
 [3] Live Encoder Monitor (Spin wheels by hand)
 [4] Motor Direction Test (Check forward/reverse wiring)
 [5] AUTONOMOUS PRE-RUN (Chạy thử: giữ tâm làn & dừng)
 [s] Emergency Stop (Stop all motors immediately)
 [m] Show this menu again
========================================================
```

### Bước 1: Gõ `1` — Kiểm tra I2C Scanner
- Kỳ vọng xuất hiện 4 địa chỉ:
  - `0x30`: VL53L0X Trái
  - `0x31`: VL53L0X Trước
  - `0x32`: VL53L0X Phải
  - `0x68`: MPU-6050
- *Xử lý lỗi:* Nếu thiếu cảm biến nào, kiểm tra ngay dây nối 3.3V, GND, chân SDA (GPIO 6), SCL (GPIO 7) hoặc chân XSHUT tương ứng.

### Bước 2: Gõ `2` — Kiểm tra cảm biến ToF & IMU
- Dùng tay hoặc một tấm bìa chắn phía trước, bên trái, bên phải.
- Quan sát trên Serial xem số đo mm có giảm khi đưa tay lại gần không.
- Nghiêng robot để thấy trục Z của IMU thay đổi.
- Bấm phím bất kỳ để dừng xem.

### Bước 3: Gõ `3` — Kiểm tra Encoder
- Nhấc robot lên, dùng tay quay bánh xe tiến về phía trước.
- Quan sát số tick của bánh Trái và bánh Phải: cả 2 phải **tăng dần** (dương).
- Bấm phím bất kỳ để dừng.

### Bước 4: Gõ `4` — Kiểm tra chiều quay Motor
- Đặt robot kê bánh lên cao (hoặc để trên bàn không chạm đất).
- Khi chọn `4`, motor Trái sẽ quay tiến 600ms, lùi 600ms; sau đó motor Phải quay tiến 600ms, lùi 600ms.
- *Xử lý nếu motor quay ngược:* Không cần rút dây hàn lại! Chỉ cần mở file `config_pins.h`, đổi:
  ```cpp
  constexpr bool MOTOR_LEFT_INVERT  = true; // Nếu motor trái bị ngược
  constexpr bool MOTOR_RIGHT_INVERT = true; // Nếu motor phải bị ngược
  ```

### Bước 5: Gõ `5` — CHẠY THỬ THỰC TẾ (Autonomous Pre-run)
- Đặt robot vào đường chạy hoặc giữa 2 bức tường mẫu.
- Gõ `5`, robot sẽ đếm ngược 3, 2, 1 rồi bắt đầu bò về phía trước.
- Robot sẽ tự động:
  - Bò ở tốc độ an toàn (`PRE_RUN_BASE_SPEED = 90`).
  - Lấy sai lệch khoảng cách giữa tường trái và tường phải để tự uốn lái giữ robot nằm chính giữa làn.
  - **Tự dừng khẩn cấp** khi gặp tường phía trước cách $\le 100\text{ mm}$.
  - Gõ bất kỳ ký tự nào trên Serial để ngắt động cơ khẩn cấp bất cứ lúc nào.
