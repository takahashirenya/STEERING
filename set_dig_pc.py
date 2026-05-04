import tkinter as tk
from serial import Serial

ser = Serial("COM6", 115200, timeout=1)
ser.reset_input_buffer()

root = tk.Tk()
root.title("Pico サーボ制御")
root.geometry("400x400")

dig_value = tk.StringVar(value="---")


def send_cmd(cmd):
    ser.write((cmd + "\n").encode())


def move(cmd):
    send_cmd(cmd)


def update_loop():
    send_cmd("GET")
    try:
        while ser.in_waiting:
            line = ser.readline().decode(errors="ignore").strip()
            print(line)  # デバッグ用

            if line.startswith("D:"):
                value = line[2:]  # "D:" を除く
                dig_value.set(value)

    except Exception as e:
        print(f"受信エラー: {e}")

    root.after(200, update_loop)


tk.Label(root, text="現在の Duty 比:").pack(pady=(15, 0))
tk.Label(root, textvariable=dig_value, font=("Arial", 16), fg="blue").pack()

btn_frame = tk.Frame(root)
btn_frame.pack(pady=15)

tk.Button(btn_frame, text="← 遅", width=10, command=lambda: move("LEFT_SLOW")).grid(
    row=0, column=0, padx=10
)
tk.Button(btn_frame, text="→ 遅", width=10, command=lambda: move("RIGHT_SLOW")).grid(
    row=0, column=1, padx=10
)

tk.Button(btn_frame, text="← 速", width=10, command=lambda: move("LEFT_FAST")).grid(
    row=1, column=0, padx=10, pady=5
)
tk.Button(btn_frame, text="→ 速", width=10, command=lambda: move("RIGHT_FAST")).grid(
    row=1, column=1, padx=10, pady=5
)

tk.Button(root, text="中央へ戻す", width=25, command=lambda: move("NEUTRAL")).pack(
    pady=10
)

root.after(200, update_loop)
root.mainloop()
