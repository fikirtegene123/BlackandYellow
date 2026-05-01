import time
import socket
import tkinter as tk
from tkinter import scrolledtext, messagebox
import threading
import os
import numpy as np

# Matplotlib embedding imports
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

##### GLOBAL CONFIGURATION #####
HOST = "192.168.1.1"
PORT = 288
FILENAME = 'sensor-scan.txt'
MAX_PLOT_DIST = 100 

class CyBotGUI:
    def __init__(self, window):
        self.window = window
        self.window.title("CyBot Control - Movement Tracking & Dual Radar")
        self.window.geometry("1200x800")

        # --- POSITION STATE ---
        self.pos_x = 0.0
        self.pos_y = 0.0
        self.heading = 90.0  # Facing "North" initially
        self.path_x = [0.0]
        self.path_y = [0.0]
        
        self.gui_send_message = "wait\n"
        self.absolute_path = os.path.dirname(__file__)
        self.full_path = os.path.join(self.absolute_path, FILENAME)
        self.is_front_scan = True 

        # --- UI LAYOUT ---
        self.left_frame = tk.Frame(window)
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, padx=10, pady=10)

        self.console = scrolledtext.ScrolledText(self.left_frame, state='disabled', height=20, width=45)
        self.console.pack(pady=(0, 10), fill=tk.BOTH, expand=True)

        self.calc_btn = tk.Button(self.left_frame, text="Distance Calculator", command=self.open_calculator, bg="#f0f0f0")
        self.calc_btn.pack(fill=tk.X, pady=2)
        
        self.map_btn = tk.Button(self.left_frame, text="Open Movement Map", command=self.open_map, bg="#d1e7ff", font=('Arial', 10, 'bold'))
        self.map_btn.pack(fill=tk.X, pady=2)

        self.input_label = tk.Label(self.left_frame, text="Command (w, a, s, d, n, b, quit):")
        self.input_label.pack(anchor="w", pady=(10,0))
        
        self.entry = tk.Entry(self.left_frame)
        self.entry.pack(fill=tk.X, pady=5)
        self.entry.bind("<Return>", self.process_input)
        self.entry.focus_set()

        self.right_frame = tk.Frame(window)
        self.right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.fig = Figure(figsize=(8, 6), dpi=100)
        self.ax_front = self.fig.add_subplot(121, projection='polar')
        self.ax_back = self.fig.add_subplot(122, projection='polar')
        self.setup_polar_axis(self.ax_front, "FRONT RADAR")
        self.setup_polar_axis(self.ax_back, "BACK RADAR")
        self.fig.tight_layout()

        self.canvas = FigureCanvasTkAgg(self.fig, master=self.right_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        self.map_win = None
        self.map_ax = None
        self.map_canvas = None

        # --- SOCKET & THREADING ---
        self.cybot_socket = None
        self.cybot_file = None
        self.socket_thread = threading.Thread(target=self.run_socket_logic, daemon=True)
        self.socket_thread.start()

    def setup_polar_axis(self, ax, title):
        ax.set_thetalim(0, np.pi)
        ax.set_rmax(MAX_PLOT_DIST)
        ax.set_rticks([20, 40, 60, 80, 100]) 
        ax.set_yticklabels(['20', '40', '60', '80', '100'])
        ax.set_title(title, pad=15, weight='bold')

    def log(self, message):
        self.console.configure(state='normal')
        self.console.insert(tk.END, f"{message}\n")
        self.console.see(tk.END)
        self.console.configure(state='disabled')

    def update_position(self, cmd):
        """Calculates tracking with turns."""
        dist = 20.0
        angle_step = 13.2

        if cmd == 'w':
            self.pos_x += dist * np.cos(np.deg2rad(self.heading))
            self.pos_y += dist * np.sin(np.deg2rad(self.heading))
        elif cmd == 'a':
            self.heading += angle_step
        elif cmd == 'd':
            self.heading -= angle_step
        elif cmd == 's':
            self.pos_x -= dist * np.cos(np.deg2rad(self.heading))
            self.pos_y -= dist * np.sin(np.deg2rad(self.heading))

        self.path_x.append(self.pos_x)
        self.path_y.append(self.pos_y)
        
        if self.map_win and self.map_win.winfo_exists():
            self.draw_map()

    def process_input(self, event):
        cmd = self.entry.get().strip().lower()
        if cmd:
            if cmd == "n": self.is_front_scan = True
            if cmd == "b": self.is_front_scan = False
            
            if cmd in ['w', 'a', 's', 'd']:
                self.update_position(cmd)

            self.gui_send_message = cmd + "\n"
            self.entry.delete(0, tk.END)
            if cmd == "quit":
                self.window.after(500, self.window.destroy)

    def open_map(self):
        if self.map_win and self.map_win.winfo_exists():
            self.map_win.lift()
            return

        self.map_win = tk.Toplevel(self.window)
        self.map_win.title("CyBot Movement Map")
        self.map_win.geometry("600x650")

        fig_map = Figure(figsize=(5, 5), dpi=100)
        self.map_ax = fig_map.add_subplot(111)
        self.map_canvas = FigureCanvasTkAgg(fig_map, master=self.map_win)
        self.map_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        tk.Button(self.map_win, text="Reset Path", command=self.reset_path).pack(pady=5)
        
        self.draw_map()

    def reset_path(self):
        self.pos_x, self.pos_y = 0.0, 0.0
        self.heading = 90.0
        self.path_x, self.path_y = [0.0], [0.0]
        self.draw_map()

    def draw_map(self):
        if not self.map_ax: return
        self.map_ax.clear()
        self.map_ax.plot(self.path_x, self.path_y, '-o', markersize=3, alpha=0.6, label="Path")
        
        self.map_ax.plot(self.pos_x, self.pos_y, 'rs', markersize=6)
        
        dx = 8 * np.cos(np.deg2rad(self.heading))
        dy = 8 * np.sin(np.deg2rad(self.heading))
        self.map_ax.arrow(self.pos_x, self.pos_y, dx, dy, head_width=2.5, color='red')
        
        self.map_ax.set_title(f"Position: ({self.pos_x:.1f}, {self.pos_y:.1f}) | Heading: {self.heading:.1f}°")
        self.map_ax.grid(True, linestyle='--')
        self.map_ax.set_aspect('equal', adjustable='datalim')
        self.map_canvas.draw()

    def update_plot(self):
        raw_angles, raw_distances = [], []
        try:
            if not os.path.exists(self.full_path): return
            with open(self.full_path, 'r') as f:
                next(f)
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        angle = float(parts[0])
                        dist = float(parts[1])
                        
                        # --- CAP DISTANCE AT 100 ---
                        if dist > 100:
                            dist = 100
                            
                        raw_angles.append(angle)
                        raw_distances.append(dist)

            target_ax = self.ax_front if self.is_front_scan else self.ax_back
            target_ax.clear()
            self.setup_polar_axis(target_ax, "FRONT" if self.is_front_scan else "BACK")
            if raw_angles:
                angles_rad = np.deg2rad(raw_angles)
                target_ax.plot(angles_rad, raw_distances, color='#00FF00', linewidth=2, marker='o', markersize=3)
            self.canvas.draw()
        except Exception as e:
            self.log(f"Plotting Error: {e}")

    def run_socket_logic(self):
        try:
            self.cybot_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.cybot_socket.connect((HOST, PORT))
            self.cybot_file = self.cybot_socket.makefile("rbw", buffering=0)
            self.log("Connected to CyBot.")
            threading.Thread(target=self.listen_to_cybot, daemon=True).start()
        except Exception as e:
            self.log(f"Connection Error: {e}")
            return

        while True:
            if self.gui_send_message != "wait\n":
                msg = self.gui_send_message
                self.gui_send_message = "wait\n"
                try:
                    self.cybot_file.write(msg.encode())
                except: break
            time.sleep(0.1)

    def listen_to_cybot(self):
        recording_scan = False
        scan_f = None
        while True:
            try:
                line = self.cybot_file.readline().decode().strip()
                if not line: continue
                self.window.after(0, lambda m=line: self.log(f"UART: {m}"))
                if "Angle(Degrees)" in line:
                    recording_scan = True
                    scan_f = open(self.full_path, 'w')
                    scan_f.write("Header\n")
                    continue
                if "END" in line:
                    recording_scan = False
                    if scan_f: scan_f.close()
                    self.window.after(0, self.update_plot)
                    continue
                if recording_scan and line and (line[0].isdigit() or line.startswith('-')):
                    if scan_f: scan_f.write(line + "\n")
            except: break

    def open_calculator(self):
        calc_win = tk.Toplevel(self.window)
        calc_win.title("Point-to-Point Calc")
        calc_win.geometry("300x400")
        tk.Label(calc_win, text="Object 1 (Angle | Dist)").pack(pady=5)
        a1_e = tk.Entry(calc_win); a1_e.pack()
        d1_e = tk.Entry(calc_win); d1_e.pack()
        tk.Label(calc_win, text="Object 2 (Angle | Dist)").pack(pady=5)
        a2_e = tk.Entry(calc_win); a2_e.pack()
        d2_e = tk.Entry(calc_win); d2_e.pack()
        res_l = tk.Label(calc_win, text="Result: -- cm", font=('Arial', 12, 'bold'), pady=20)
        res_l.pack()
        def calc():
            try:
                a1, d1 = np.deg2rad(float(a1_e.get())), float(d1_e.get())
                a2, d2 = np.deg2rad(float(a2_e.get())), float(d2_e.get())
                dist = np.sqrt(d1**2 + d2**2 - 2*d1*d2*np.cos(a1-a2))
                res_l.config(text=f"{dist:.2f} cm")
            except: pass
        tk.Button(calc_win, text="Calculate", command=calc).pack()

if __name__ == "__main__":
    root = tk.Tk()
    app = CyBotGUI(root)
    root.mainloop()