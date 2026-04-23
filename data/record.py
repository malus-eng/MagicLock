import serial
import time
import os

# ================= 配置区 =================
COM_PORT = 'COM16'   # ⚠️ 修改为你的 Arduino 端口号（在刚才的截图里我看到你是 COM17）
BAUD_RATE = 115200
LABEL = 'idle'        # ⚠️ 当前采集的手势，比如 'one' 或 'zero'
SAVE_DIR = './dataset' # 保存 CSV 文件的子文件夹
# =========================================

# 自动创建数据文件夹
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)

# 自动寻找下一个文件序号，防止覆盖前面的数据
def get_next_filename(label):
    i = 1
    while os.path.exists(f"{SAVE_DIR}/{label}_{i:02d}.csv"):
        i += 1
    return f"{SAVE_DIR}/{label}_{i:02d}.csv"

try:
    print(f"正在连接 {COM_PORT}...")
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) # 等待 Arduino 重启
    print(f"✅ 连接成功！准备采集手势 '{LABEL}'")
    print("-" * 40)

    while True:
        next_file = get_next_filename(LABEL)
        index = next_file.split('_')[-1].split('.')[0]
        
        # 等待用户按下回车键
        user_input = input(f"\n[按下回车键] 开始录制第 {index} 个样本 (或输入 q 退出): ")
        if user_input.lower() == 'q':
            break
            
        # 1. 发送触发指令 'r' 给 Arduino
        ser.write(b'r')
        print("=> 滴！已发送录制指令。请盯紧板子，黄灯亮起时开始动作！")
        
        data_lines = []
        recording = False
        
        # 2. 监听 Arduino 传回来的数据
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue
            
            # 当读到表头时，开始真正的记录
            if "timestamp,accX,accY,accZ" in line:
                recording = True
                data_lines.append(line)
                continue
            
            # 当读到结束语时，停止记录
            if "录制结束" in line:
                break
                
            if recording:
                data_lines.append(line)
        
        # 3. 自动打包成 CSV 文件
        if len(data_lines) > 50: 
            with open(next_file, 'w', encoding='utf-8') as f:
                f.write('\n'.join(data_lines))
            print(f"🎉 搞定！已自动保存: {next_file} (共 {len(data_lines)-1} 行数据)")
        else:
            print("❌ 数据太短或录制出错，请重试。")

except serial.SerialException:
    print("❌ 串口被占用或找不到！")
    print("💡 必杀排错指南：请确认你已经关闭了 Arduino IDE 里的【串口监视器】！因为一个串口只能被一个程序占用。")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()