import time
import socket
import tkinter as tk
from tkinter import scrolledtext
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
MAX_PLOT_DIST = 100  # Global setting for the 100cm zoom

class CyBotGUI:
    def __init__(self, window):
        self.window = window
        self.window.title("CyBot Dual-Radar (100cm Zoomed)")
        self.window.geometry("1100x600")

        # --- UI LAYOUT ---
        self.left_frame = tk.Frame(window)
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, padx=10, pady=10)

        self.console = scrolledtext.ScrolledText(self.left_frame, state='disabled', height=25, width=45)
        self.console.pack(pady=(0, 10), fill=tk.BOTH, expand=True)

        self.input_label = tk.Label(self.left_frame, text="Command (n=front, b=back, quit):")
        self.input_label.pack(anchor="w")
        
        self.entry = tk.Entry(self.left_frame)
        self.entry.pack(fill=tk.X, pady=5)
        self.entry.bind("<Return>", self.process_input)
        self.entry.focus_set()

        # Right Frame: Dual Live Plots
        self.right_frame = tk.Frame(window)
        self.right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.fig = Figure(figsize=(10, 5), dpi=100)
        
        # Setup Front Plot
        self.ax_front = self.fig.add_subplot(121, projection='polar')
        self.setup_polar_axis(self.ax_front, "FRONT RADAR")
        
        # Setup Back Plot
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

    def setup_polar_axis(self, ax, title):
        """Initializes the polar view with 180 degree span and 100cm zoom."""
        ax.set_thetamax(180)
        ax.set_rmax(MAX_PLOT_DIST)  # Forces the zoom to 100cm
        ax.set_title(title)

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

    def filter_and_clip(self, distances):
        """Removes spikes and clips data to 100cm for the zoomed view."""
        if len(distances) < 3:
            return distances
            
        filtered = list(distances)
        threshold = 50.0 

        for i in range(1, len(distances) - 1):
            prev_d, curr_d, next_d = distances[i-1], distances[i], distances[i+1]
            
            # Spike removal
            if abs(curr_d - prev_d) > threshold and abs(prev_d - next_d) < 10.0:
                filtered[i] = (prev_d + next_d) / 2.0
        
        # Clipping: Ensure Matplotlib doesn't auto-resize if data is > 100
        return [d if d <= MAX_PLOT_DIST else np.nan for d in filtered]

    def update_plot(self, is_front):
        raw_angles = []
        raw_distances = []
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

            clean_distances = self.filter_and_clip(raw_distances)
            target_ax = self.ax_front if is_front else self.ax_back
            title = "FRONT SIDE" if is_front else "BACK SIDE"

            target_ax.clear()
            self.setup_polar_axis(target_ax, title) # Re-apply 100cm limit after clear
            
            angles_rad = np.deg2rad(raw_angles)
            target_ax.plot(angles_rad, clean_distances, color='r', linewidth=2, marker='o', markersize=2)
            
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
                    self.log(f"Receiving {'Front' if is_front else 'Back'} Data...")
                    
                    with open(self.full_path, 'w') as f:
                        f.write("Angle \t Distance\n") 
                        while True:
                            rx_line = cybot.readline().decode().strip()
                            if "END" in rx_line or not rx_line:
                                break
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

        cybot.close()
        cybot_socket.close()

if __name__ == "__main__":
    root = tk.Tk()
    app = CyBotGUI(root)
    root.mainloop()