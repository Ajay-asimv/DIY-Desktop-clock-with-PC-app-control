import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import datetime
import time

class ClockControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Arduino Clock Controller")
        self.root.geometry("440x730")

        self.ser = None

        # --- 1. COM Port Selection & Connection ---
        ttk.Label(root, text="1. Select Arduino COM Port:", font=("Arial", 10, "bold")).pack(pady=(10,3))
        com_frame = ttk.Frame(root)
        com_frame.pack()
        
        self.port_combo = ttk.Combobox(com_frame, values=[port.device for port in serial.tools.list_ports.comports()], width=15)
        self.port_combo.pack(side=tk.LEFT, padx=5)
        if self.port_combo['values']:
            self.port_combo.current(0)
            
        self.connect_btn = ttk.Button(com_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.pack(side=tk.LEFT)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 2. Quick Sync PC Time ---
        ttk.Label(root, text="2. Quick Sync:", font=("Arial", 10, "bold")).pack()
        ttk.Button(root, text="Sync PC Time to Clock", command=self.sync_time).pack(pady=3)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 3. Manual Time Setting ---
        ttk.Label(root, text="3. Set Custom Time:", font=("Arial", 10, "bold")).pack()
        
        date_frame = ttk.Frame(root)
        date_frame.pack(pady=2)
        ttk.Label(date_frame, text="Date (YYYY/MM/DD):").pack(side=tk.LEFT, padx=(0,5))
        self.man_yr = ttk.Spinbox(date_frame, from_=2000, to=2099, width=5)
        self.man_yr.pack(side=tk.LEFT, padx=2)
        self.man_mo = ttk.Spinbox(date_frame, from_=1, to=12, width=3)
        self.man_mo.pack(side=tk.LEFT, padx=2)
        self.man_dy = ttk.Spinbox(date_frame, from_=1, to=31, width=3)
        self.man_dy.pack(side=tk.LEFT, padx=2)
        
        time_frame = ttk.Frame(root)
        time_frame.pack(pady=2)
        ttk.Label(time_frame, text="Time (HH:MM:SS):    ").pack(side=tk.LEFT, padx=(0,5))
        self.man_hr = ttk.Spinbox(time_frame, from_=0, to=23, width=3)
        self.man_hr.pack(side=tk.LEFT, padx=2)
        self.man_mn = ttk.Spinbox(time_frame, from_=0, to=59, width=3)
        self.man_mn.pack(side=tk.LEFT, padx=2)
        self.man_sc = ttk.Spinbox(time_frame, from_=0, to=59, width=3)
        self.man_sc.pack(side=tk.LEFT, padx=2)

        now = datetime.datetime.now()
        self.man_yr.set(now.year)
        self.man_mo.set(now.month)
        self.man_dy.set(now.day)
        self.man_hr.set(now.hour)
        self.man_mn.set(now.minute)
        self.man_sc.set(0)

        ttk.Button(root, text="Send Custom Time", command=self.set_manual_time).pack(pady=3)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 4. Reminder System ---
        ttk.Label(root, text="4. Set Daily Reminder (Scrolling Text):", font=("Arial", 10, "bold")).pack()
        rem_frame = ttk.Frame(root)
        rem_frame.pack(pady=2)
        
        ttk.Label(rem_frame, text="Time (24hr): ").pack(side=tk.LEFT)
        self.rem_hr = ttk.Spinbox(rem_frame, from_=0, to=23, width=3)
        self.rem_hr.set("14")
        self.rem_hr.pack(side=tk.LEFT)
        ttk.Label(rem_frame, text=":").pack(side=tk.LEFT)
        self.rem_mn = ttk.Spinbox(rem_frame, from_=0, to=59, width=3)
        self.rem_mn.set("30")
        self.rem_mn.pack(side=tk.LEFT)
        
        self.msg_var = tk.StringVar(value="Take your medicine and drink water!")
        ttk.Entry(root, textvariable=self.msg_var, width=40).pack(pady=2)
        
        rem_btn_frame = ttk.Frame(root)
        rem_btn_frame.pack(pady=2)
        ttk.Button(rem_btn_frame, text="Send Reminder", command=self.send_reminder).pack(side=tk.LEFT, padx=5)
        ttk.Button(rem_btn_frame, text="Clear Reminder", command=self.clear_reminder).pack(side=tk.LEFT, padx=5)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 5. Daily Alarm (NEW) ---
        ttk.Label(root, text="5. Set Daily Sound Alarm:", font=("Arial", 10, "bold")).pack()
        alm_frame = ttk.Frame(root)
        alm_frame.pack(pady=2)
        
        ttk.Label(alm_frame, text="Alarm Time (24hr): ").pack(side=tk.LEFT)
        self.alm_hr = ttk.Spinbox(alm_frame, from_=0, to=23, width=3)
        self.alm_hr.set("07")
        self.alm_hr.pack(side=tk.LEFT)
        ttk.Label(alm_frame, text=":").pack(side=tk.LEFT)
        self.alm_mn = ttk.Spinbox(alm_frame, from_=0, to=59, width=3)
        self.alm_mn.set("00")
        self.alm_mn.pack(side=tk.LEFT)

        alm_btn_frame = ttk.Frame(root)
        alm_btn_frame.pack(pady=3)
        ttk.Button(alm_btn_frame, text="Set Alarm", command=self.set_alarm).pack(side=tk.LEFT, padx=5)
        ttk.Button(alm_btn_frame, text="Stop / Clear Alarm", command=self.clear_alarm).pack(side=tk.LEFT, padx=5)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 6. Hourly Westminster Chime Toggle (NEW) ---
        ttk.Label(root, text="6. Hourly Westminster Chime:", font=("Arial", 10, "bold")).pack()
        chime_frame = ttk.Frame(root)
        chime_frame.pack(pady=3)
        ttk.Button(chime_frame, text="Turn Chime ON", command=lambda: self.set_chime(True)).pack(side=tk.LEFT, padx=5)
        ttk.Button(chime_frame, text="Turn Chime OFF", command=lambda: self.set_chime(False)).pack(side=tk.LEFT, padx=5)

        ttk.Separator(root, orient='horizontal').pack(fill='x', padx=20, pady=8)

        # --- 7. Temporary Custom Message ---
        ttk.Label(root, text="7. Display Custom Message (Temporary):", font=("Arial", 10, "bold")).pack()
        custom_frame = ttk.Frame(root)
        custom_frame.pack(pady=2)
        
        ttk.Label(custom_frame, text="Show for (seconds): ").pack(side=tk.LEFT)
        self.custom_sec = ttk.Spinbox(custom_frame, from_=1, to=999, width=5)
        self.custom_sec.set("15")
        self.custom_sec.pack(side=tk.LEFT)

        self.custom_var = tk.StringVar(value="Hello! I am a scrolling message.")
        ttk.Entry(root, textvariable=self.custom_var, width=40).pack(pady=2)
        ttk.Button(root, text="Send Temporary Message", command=self.send_custom_message).pack(pady=3)
        
    def toggle_connection(self):
        if self.ser is None or not self.ser.is_open:
            port = self.port_combo.get()
            if not port:
                messagebox.showerror("Error", "Please select a port first.")
                return
            try:
                self.ser = serial.Serial(port, 9600, timeout=2)
                time.sleep(2)
                self.connect_btn.config(text="Disconnect")
                messagebox.showinfo("Connected", f"Successfully connected to {port}!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to connect: {e}")
        else:
            self.ser.close()
            self.connect_btn.config(text="Connect")
            messagebox.showinfo("Disconnected", "Disconnected from Arduino.")

    def send_command(self, cmd_str):
        if self.ser is None or not self.ser.is_open:
            messagebox.showerror("Error", "You must click 'Connect' first!")
            return False
        try:
            self.ser.reset_input_buffer()
            self.ser.write('\n'.encode('utf-8'))
            time.sleep(0.1)
            
            self.ser.write((cmd_str + '\n').encode('utf-8'))
            self.ser.flush()
            
            timeout = time.time() + 2.0
            response_text = ""
            
            while time.time() < timeout:
                if self.ser.in_waiting > 0:
                    chunk = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    response_text += chunk
                    if "OK" in response_text:
                        return True
                time.sleep(0.1)
                
            response_text = response_text.strip()
            messagebox.showwarning("Warning", f"Arduino didn't say OK. It said:\n\n'{response_text}'")
            return False
            
        except Exception as e:
            messagebox.showerror("Error", f"Communication error: {e}")
            return False

    def sync_time(self):
        now = datetime.datetime.now()
        dow = (now.weekday() + 1) % 7 + 1
        cmd = f"T,{now.year},{now.month},{now.day},{dow},{now.hour},{now.minute},{now.second}"
        if self.send_command(cmd):
            messagebox.showinfo("Success", "PC Time synchronized successfully!")

    def set_manual_time(self):
        try:
            y, m, d = int(self.man_yr.get()), int(self.man_mo.get()), int(self.man_dy.get())
            h, mn, s = int(self.man_hr.get()), int(self.man_mn.get()), int(self.man_sc.get())
            dt = datetime.datetime(y, m, d)
            dow = (dt.weekday() + 1) % 7 + 1 
            cmd = f"T,{y},{m},{d},{dow},{h},{mn},{s}"
            if self.send_command(cmd):
                messagebox.showinfo("Success", "Custom Time set successfully!")
        except ValueError:
            messagebox.showerror("Error", "Please ensure all time and date fields are valid numbers.")

    def send_reminder(self):
        hr = self.rem_hr.get().zfill(2)
        mn = self.rem_mn.get().zfill(2)
        msg = self.msg_var.get()[:60]
        cmd = f"R,{hr},{mn},{msg}"
        if self.send_command(cmd):
            messagebox.showinfo("Success", "Reminder set on the clock!")

    def clear_reminder(self):
        if self.send_command("CLEAR_REM"):
            messagebox.showinfo("Success", "Reminder cleared from the clock!")

    def set_alarm(self):
        hr = self.alm_hr.get().zfill(2)
        mn = self.alm_mn.get().zfill(2)
        cmd = f"A,{hr},{mn}"
        if self.send_command(cmd):
            messagebox.showinfo("Success", f"Alarm set for {hr}:{mn}!")

    def clear_alarm(self):
        if self.send_command("CLEAR_ALM"):
            messagebox.showinfo("Success", "Alarm stopped and cleared!")

    def set_chime(self, enabled):
        state = "1" if enabled else "0"
        status_word = "ENABLED" if enabled else "DISABLED"
        if self.send_command(f"C,{state}"):
            messagebox.showinfo("Success", f"Hourly Westminster Chime is now {status_word}!")

    def send_custom_message(self):
        try:
            sec = int(self.custom_sec.get())
            msg = self.custom_var.get()[:60]
            cmd = f"M,{sec},{msg}"
            if self.send_command(cmd):
                messagebox.showinfo("Success", f"Custom message sent to display for {sec} seconds!")
        except ValueError:
            messagebox.showerror("Error", "Duration must be a valid number.")

if __name__ == "__main__":
    root = tk.Tk()
    app = ClockControllerApp(root)
    root.mainloop()