# BẢN NGUYÊN MẪU TỰ HÀNH GIẢI MÊ CUNG (AUTONOMOUS FLOOD FILL FOR ESP32-C6)

Thư mục này là bản chuyển đổi (port) toàn diện từ thuật toán **Flood Fill** của dự án tham khảo `Technoxian_2023_Arduino_Micromouse` sang cho phần cứng thực tế của bạn:
- **Vi điều khiển:** ESP32-C6 DevKitC-1 (3.3V)
- **Cảm biến:** 3x Cảm biến khoảng cách ToF Laser VL53L0X (`0x30`, `0x31`, `0x32`) + IMU MPU-6050 (`0x68`)
- **Động cơ & Driver:** 2x GA12-N20 + DFRobot DRI0044 (TB6612FNG)
- **Encoder:** Hall-effect quadrature ngắt phần cứng 2 kênh (Trái: GPIO 0/1, Phải: GPIO 2/3)

---

## 1. Cấu trúc mã nguồn trong Arduino IDE

Tất cả các file đều được viết dưới dạng tab `.ino` quen thuộc của Arduino IDE:

1. **`Micromouse_ESP32C6_Autonomous.ino`:**
   - Quản lý máy trạng thái (Chờ lệnh $\rightarrow$ Chạy giải mê cung $\rightarrow$ Báo chiến thắng khi chạm đích).
   - Menu điều khiển bắt đầu và ngắt khẩn cấp qua Serial.
2. **`_config.h`:**
   - Chứa toàn bộ định nghĩa chân chuẩn theo `MICROMOUSE_MASTER_PROMPT.md` (với chân XSHUT Phải là GPIO 5).
   - Các thông số hiệu chuẩn chuyển động (`TICKS_PER_CELL`, `TICKS_TURN_90`, tốc độ chạy, độ nhạy PID).
3. **`_motors.ino`:**
   - Điều khiển động cơ qua PWM và DIR, dừng khẩn cấp và cờ đảo chiều.
4. **`_encoders.ino`:**
   - Giải mã xung encoder bằng ngắt tốc độ cao `IRAM_ATTR`.
5. **`_sensors.ino`:**
   - Khởi tạo tuần tự 3 cảm biến ToF, đọc khoảng cách chính xác theo milimét (mm) và xác định sự hiện diện của tường.
6. **`_motion.ino`:**
   - Điều khiển chạy thẳng từng ô hoặc cụm ô với **đường cong tăng/giảm tốc (Trapezoidal profile)** để chống trượt bánh.
   - **PID kép:** Vừa ép 2 bánh chạy bằng số xung nhau, vừa tự động uốn lái giữ robot nằm chính giữa làn cách đều 2 tường.
   - Điều khiển quay góc tại chỗ $90^\circ$ và $180^\circ$.
7. **`_floodfill.ino`:**
   - Quản lý bản đồ mê cung 256 ô ($16 \times 16$).
   - Thuật toán loang trọng số khoảng cách Manhattan về 4 ô trung tâm $(7,7), (7,8), (8,7), (8,8)$.
   - Chọn hướng đi tối ưu (ưu tiên đi thẳng khi bằng điểm để hạn chế quay xe).
   - Tối ưu đường hầm (`isTunnel`): Tự động gom nhiều ô thẳng liên tiếp để bứt tốc thay vì dừng giật cục từng ô.

---

## 2. Các thông số cần hiệu chuẩn trước khi chạy thật (`_config.h`)

Khi lắp ráp xong phần cứng và bánh xe thực tế, bạn chỉ cần tinh chỉnh 2 thông số này trong file `_config.h`:

1. **`TICKS_PER_CELL = 1500;`** (Số xung để robot đi hết 1 ô mê cung 18cm):
   - Đặt robot ở vạch xuất phát, cho chạy thử 1 ô.
   - Nếu robot chạy chưa tới tâm ô tiếp theo $\rightarrow$ tăng số này lên.
   - Nếu robot chạy lố qua tâm ô $\rightarrow$ giảm số này xuống.
2. **`TICKS_TURN_90 = 420;`** (Số xung để robot quay đúng một góc vuông $90^\circ$):
   - Cho robot quay thử $90^\circ$.
   - Nếu quay chưa đủ $90^\circ$ $\rightarrow$ tăng số này lên.
   - Nếu quay quá $90^\circ$ $\rightarrow$ giảm số này xuống.

---

## 3. Cách sử dụng tại Hackathon

1. Mở file `Micromouse_ESP32C6_Autonomous.ino` trong Arduino IDE.
2. Nạp code vào ESP32-C6 (nhớ chọn `Tools` $\rightarrow$ `USB CDC On Boot: Enabled`).
3. Đặt robot vào ô xuất phát (ô 0) quay đầu về hướng Bắc (North).
4. Mở **Serial Monitor** (115200 baud).
5. Gõ phím **`g`** hoặc **`1`** $\rightarrow$ Enter: Robot sẽ đếm ngược 3, 2, 1 rồi tự động tìm đường vào trung tâm mê cung!
6. Bấm phím **`s`** hoặc phím Cách (Space) bất kỳ lúc nào để phanh khẩn cấp.
