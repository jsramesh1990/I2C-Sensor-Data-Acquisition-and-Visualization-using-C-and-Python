#!/usr/bin/env python3
import socket, json, threading
from PyQt5 import QtCore, QtWidgets
import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg

SOCKET_PATH = "/tmp/sensor_backend.sock"

class SocketReader(QtCore.QObject):
    new_sample = QtCore.pyqtSignal(dict)
    def __init__(self):
        super().__init__()
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.running = True
    def start(self): self.thread.start()
    def run(self):
        while self.running:
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.connect(SOCKET_PATH)
                with s:
                    buf = b""
                    while self.running:
                        data = s.recv(1024)
                        if not data: break
                        buf += data
                        while b"\n" in buf:
                            line, buf = buf.split(b"\n", 1)
                            try: self.new_sample.emit(json.loads(line))
                            except: pass
            except Exception: 
                import time; time.sleep(1)
    def stop(self): self.running = False

class Plot(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.fig = Figure()
        self.canvas = FigureCanvasQTAgg(self.fig)
        self.ax = self.fig.add_subplot(111)
        self.temps, self.hums = [], []
        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.canvas)
        self.setLayout(layout)
    def update(self, s):
        self.temps.append(s['temp']); self.hums.append(s['hum'])
        self.ax.clear()
        self.ax.plot(self.temps, label='Temp')
        self.ax.plot(self.hums, label='Hum')
        self.ax.legend()
        self.canvas.draw()

class Main(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sensor GUI")
        self.plot = Plot()
        self.setCentralWidget(self.plot)
        self.reader = SocketReader()
        self.reader.new_sample.connect(self.plot.update)
        self.reader.start()
    def closeEvent(self, e):
        self.reader.stop(); super().closeEvent(e)

if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    win = Main()
    win.show()
    app.exec_()

