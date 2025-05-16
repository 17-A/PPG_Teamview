import pandas as pd
import matplotlib.pyplot as plt

# # # Đọc file txt
# # df = pd.read_csv('ppg_data.txt', header=None, encoding='latin1')
# # df.columns = ['RED', 'IR']  # đặt tên cột

# # # Ép kiểu dữ liệu và loại bỏ lỗi
# # df = df.apply(pd.to_numeric, errors='coerce')  # chuyển sang float
# # df = df.dropna()  # loại bỏ dòng lỗi

# # Tần số lấy mẫu
# freq_sampling = 500  # Hz (500 mẫu/giây)

# # Đọc và làm sạch dữ liệu
# with open('ppg_data.txt', 'r', encoding='utf-16') as f:
#     lines = [line.strip() for line in f if line.strip()]  # bỏ dòng trống

# # Bỏ dòng đầu nếu là 0,0
# if lines[0] == '0,0':
#     lines = lines[1:]

# # Tách từng dòng thành 2 giá trị
# data = [line.split(',') for line in lines]

# # Tạo DataFrame từ dữ liệu đã tách
# df = pd.DataFrame(data, columns=['RED', 'IR']).astype(int)

# # Vẽ biểu đồ
# plt.figure(figsize=(14, 6))
# plt.plot(df['IR'], label='IR signal', color='red', alpha=0.7)
# plt.plot(df['RED'], label='RED signal', color='blue', alpha=0.7)

# plt.title('PPG Signal from MAX30102')
# plt.xlabel('Sample index')
# plt.ylabel('Sensor reading (ADC value)')
# plt.legend()
# plt.grid(True)
# plt.tight_layout()
# plt.show()


import pandas as pd
import matplotlib.pyplot as plt

# 1. Tần số lấy mẫu
freq_sampling = 500  # Hz (500 mẫu/giây)

# 2. Đọc và xử lý dữ liệu
with open('ppg_data.txt', 'r', encoding='utf-16') as f:
    lines = [line.strip() for line in f if line.strip()]

if lines[0] == '0,0':
    lines = lines[1:]

data = [line.split(',') for line in lines]
df = pd.DataFrame(data, columns=['RED', 'IR']).astype(int)

# 3. Tạo trục thời gian tương ứng
time = [i / freq_sampling for i in range(len(df))]

# 4. Vẽ đồ thị (hiển thị 2 giây đầu tiên = 1000 mẫu)
plt.figure(figsize=(14, 6))
plt.plot(time[:1000], df['IR'][:1000], label='IR', color='red', alpha=0.6)
plt.plot(time[:1000], df['RED'][:1000], label='RED', color='blue', alpha=0.6)

# 5. (Tuỳ chọn) Vẽ đường lọc mượt
df['IR_smoothed'] = df['IR'].rolling(window=5).mean()
plt.plot(time[:1000], df['IR_smoothed'][:1000], label='IR (smoothed)', color='green')

# 6. Trang trí biểu đồ
plt.title('PPG Signal (2 seconds at 500Hz)', fontsize=16)
plt.xlabel('Time (seconds)', fontsize=12)
plt.ylabel('Sensor Reading (ADC value)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend(fontsize=12)
plt.tight_layout()
plt.show()
