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
        self.window.title("CyBot Dual-Radar Control")
        self.window.geometry("1200x750")

        # --- UI LAYOUT ---
        self.left_frame = tk.Frame(window)
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, padx=10, pady=10)

        # Console
        self.console = scrolledtext.ScrolledText(self.left_frame, state='disabled', height=25, width=50)
        self.console.pack(pady=(0, 10), fill=tk.BOTH, expand=True)

        # Manual Calculator Button
        self.calc_btn = tk.Button(self.left_frame, text="Open Distance Calculator", command=self.open_calculator, bg="#e1e1e1")
        self.calc_btn.pack(fill=tk.X, pady=5)

        self.input_label = tk.Label(self.left_frame, text="Command (n=front, b=back, quit):")
        self.input_label.pack(anchor="w")
        
        self.entry = tk.Entry(self.left_frame)
        self.entry.pack(fill=tk.X, pady=5)
        self.entry.bind("<Return>", self.process_input)
        self.entry.focus_set()

        # Right Frame: Plots
        self.right_frame = tk.Frame(window)
        self.right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.fig = Figure(figsize=(10, 6), dpi=100)
        
        self.ax_front = self.fig.add_subplot(121, projection='polar')
        self.setup_polar_axis(self.ax_front, "FRONT RADAR")
        
        self.ax_back = self.fig.add_subplot(122, projection='polar')
        self.setup_polar_axis(self.ax_back, "BACK RADAR")
        
        self.fig.tight_layout()

        self.canvas = FigureCanvasTkAgg(self.fig, master=self.right_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # --- STATE ---
        self.gui_send_message = "wait\n"
        self.absolute_path = os.path.dirname(__file__)
        self.full_path = os.path.join(self.absolute_path, FILENAME)

        self.socket_thread = threading.Thread(target=self.run_socket_logic, daemon=True)
        self.socket_thread.start()

    def open_calculator(self):
        """Creates a pop-up window for manual distance calculations."""
        calc_win = tk.Toplevel(self.window)
        calc_win.title("Manual Point Calculator")
        calc_win.geometry("350x550") # Made slightly larger to ensure visibility
        
        # Object 1 Inputs
        tk.Label(calc_win, text="Object 1", font=('Arial', 11, 'bold')).pack(pady=5)
        tk.Label(calc_win, text="Angle (deg):").pack()
        ent_a1 = tk.Entry(calc_win); ent_a1.pack()
        tk.Label(calc_win, text="Distance (cm):").pack()
        ent_d1 = tk.Entry(calc_win); ent_d1.pack()
        
        # Object 2 Inputs
        tk.Label(calc_win, text="Object 2", font=('Arial', 11, 'bold')).pack(pady=5)
        tk.Label(calc_win, text="Angle (deg):").pack()
        ent_a2 = tk.Entry(calc_win); ent_a2.pack()
        tk.Label(calc_win, text="Distance (cm):").pack()
        ent_d2 = tk.Entry(calc_win); ent_d2.pack()
        
        # Result Display
        lbl_res = tk.Label(calc_win, text="Result: -- cm", font=('Arial', 12,), fg="blue", pady=15)
        lbl_res.pack()

        def do_calc():
            try:
                d1 = float(ent_d1.get())
                a1 = np.deg2rad(float(ent_a1.get()))
                d2 = float(ent_d2.get())
                a2 = np.deg2rad(float(ent_a2.get()))
                
                # Law of Cosines
                d_sq = d1**2 + d2**2 - (2 * d1 * d2 * np.cos(a1 - a2))
                res = np.sqrt(d_sq)
                
                lbl_res.config(text=f"Result: {res:.2f} cm")
                
                # Highlight if near the 56cm pillar target
                if 51.0 <= res <= 61.0:
                    lbl_res.config(foreground="green", text=f"MATCH: {res:.2f} cm")
                else:
                    lbl_res.config(foreground="blue")
            except ValueError:
                messagebox.showerror("Input Error", "Please enter valid numbers.")

        # THE MISSING BUTTON
        # Added pady=20 to make it stand out at the bottom
        btn_trigger = tk.Button(calc_win, text="Calculate Distance", 
                                command=do_calc, bg="#4CAF50", fg="white", 
                                font=('Arial', 10, 'bold'), padx=10, pady=5)
        btn_trigger.pack(pady=20)

    def setup_polar_axis(self, ax, title):
        """Standard horizontal polar view (0 on right, 180 on left)."""
        ax.set_thetalim(0, np.pi)  # Show only the 0-180 semi-circle
        ax.set_rmax(MAX_PLOT_DIST)
        ax.set_title(title, pad=15, weight='bold')

    def log(self, message):
        self.console.configure(state='normal')
        self.console.insert(tk.END, f"{message}\n")
        self.console.see(tk.END)
        self.console.configure(state='disabled')

    def process_input(self, event):
        cmd = self.entry.get().strip()
        if cmd:
            self.gui_send_message = cmd + "\n"
            self.log(f">> Sent: {cmd}")
            self.entry.delete(0, tk.END)
            if cmd.lower() == "quit":
                self.window.after(500, self.window.destroy)

    def update_plot(self, is_front):
        raw_angles, raw_distances = [], []
        try:
            if not os.path.exists(self.full_path): return
            with open(self.full_path, 'r') as f:
                next(f) # Skip header
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        raw_angles.append(float(parts[0]))
                        raw_distances.append(float(parts[1]))

            if not raw_angles: return

            target_ax = self.ax_front if is_front else self.ax_back
            target_ax.clear()
            self.setup_polar_axis(target_ax, "FRONT" if is_front else "BACK")
            
            # Convert degrees to radians for plotting
            angles_rad = np.deg2rad(raw_angles)
            target_ax.plot(angles_rad, raw_distances, color='r', linewidth=1.5, marker='o', markersize=2)
            
            self.canvas.draw()
        except Exception as e:
            self.log(f"Plotting Error: {e}")

    def run_socket_logic(self):
        try:
            cybot_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            cybot_socket.connect((HOST, PORT))
            cybot = cybot_socket.makefile("rbw", buffering=0)
            self.log("Connected to CyBot.")
        except Exception as e:
            self.log(f"Connection Error: {e}")
            return

        while True:
            while self.gui_send_message == "wait\n":
                time.sleep(0.05)

            current_cmd = self.gui_send_message
            self.gui_send_message = "wait\n"

            try:
                cybot.write(current_cmd.encode())
                if current_cmd in ["n\n", "b\n"]:
                    is_front = (current_cmd == "n\n")
                    self.log(f"Receiving Data...")
                    
                    with open(self.full_path, 'w') as f:
                        f.write("Angle \t Distance\n") 
                        while True:
                            rx_line = cybot.readline().decode().strip()
                            if "END" in rx_line or not rx_line: break
                            if rx_line and rx_line[0].isdigit():
                                f.write(rx_line + "\n")
                    
                    self.window.after(0, lambda: self.update_plot(is_front))
                elif current_cmd == "quit\n":
                    break
                else:
                    rx_message = cybot.readline().decode().strip()
                    if rx_message: self.log(f"CyBot: {rx_message}")
            except Exception as e:
                self.log(f"Socket Error: {e}")
                break
        cybot_socket.close()

if __name__ == "__main__":
    root = tk.Tk()
    app = CyBotGUI(root)
    root.mainloop()
