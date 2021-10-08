from PyQt5.QtCore import (Qt, QTime, QObject, QThread, pyqtSignal, pyqtSlot, QTimer, QMutex)
from PyQt5.QtWidgets import (QGridLayout, QHBoxLayout, QLabel, QLineEdit,
		QMessageBox, QPushButton, QTextEdit, QVBoxLayout, QWidget, QGroupBox,
		QTimeEdit, QComboBox, QSpinBox, QCheckBox, QApplication, QStackedWidget, QMainWindow)

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg, NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

from screeninfo import get_monitors

import os
import sys
import datetime
import random
import string
import io
import pressure_grabber as pg
import time
import csv
import numpy as np
import glob
import psutil


class MplCanvas(FigureCanvasQTAgg):
		
	def __init__(self, parent=None, width=5, height=4, dpi=100):
		self.fig = Figure(figsize=(width, height), dpi=dpi)
		self.axes = self.fig.add_subplot(111)
		self.axes2 = self.axes.twinx()
		super(MplCanvas, self).__init__(self.fig)

# Step 1: Create a worker class
class Worker(QObject):

	finished = pyqtSignal()
	progress = pyqtSignal(int)


	def __init__(self,parent=None):
		super(Worker, self).__init__(parent)

		self.data = []
		self.grabber = pg.Grabber("COM4",9600,5)
		self._mutex = QMutex()
		self.go_on = True

	@pyqtSlot()
	def run(self):
		self.data = self.grabber.getOutputStream()
		while(self.go_on):
			#print("while",self.go_on)
			self.grabber.get_transmission()
			#print("running")

	def saveData(self,name):
		self.grabber.writeToFile(name)

	def stop(self):
		print("stopping...")
		self.grabber.stopWriteToFile()

class Popup(QWidget):
	def __init__(self):
		super().__init__()

		self.initUI()

	def initUI(self):
		lblName = QLabel("Enter file name:", self)
		lblExample = QLabel("Example : Plane07_ArCO2_108_88_127_90_130_82_9-30-21", self)
		self.fileName = QLineEdit()
		layout = QGridLayout()
		layout.addWidget(lblName,0,0)
		layout.addWidget(self.fileName,1,0)
		layout.addWidget(lblExample,2,0)

		self.setLayout(layout)
		self.setWindowTitle("Saving Data")


class GUI(QWidget):

	def __init__(self, parent=None):
		super(GUI, self).__init__(parent)

		for m in get_monitors():
			if(m.is_primary == True):
				w = m.width
				h = m.height

		#set up Reference pressure & temp vs time plot
		self.sc = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.refPressure = []
		self.refTemp = []
		self.time = []
		self.data, = self.sc.axes.plot([], [],"g-")
		self.dataTemp, = self.sc.axes2.plot([], [], "b-")
		self.sc.axes.tick_params(axis='y', labelcolor='green')
		self.sc.axes2.tick_params(axis='y', labelcolor='blue')
		self.sc.axes.set_xlabel("time(s)")
		self.sc.axes.set_ylabel("pressure(psia)",color='green')
		self.sc.axes2.set_ylabel("temp(C)",color='blue')

		#set up diff pressure plot
		self.sc2 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.p0Pressure = []
		self.p1Pressure = []
		self.p2Pressure = []
		self.p3Pressure = []
		self.p4Pressure = []
		self.p5Pressure = []
		self.dataP0, = self.sc2.axes.plot([], [],"y-", label="Panel-0")
		self.dataP1, = self.sc2.axes.plot([], [], "r-", label="Panel-1")
		self.dataP2, = self.sc2.axes.plot([], [], "c-", label="Panel-2")
		self.dataP3, = self.sc2.axes.plot([], [], "b-", label="Panel-3")
		self.dataP4, = self.sc2.axes.plot([], [], "m-", label="Panel-4")
		self.dataP5, = self.sc2.axes.plot([], [], "k-", label="Panel-5")
		self.sc2.axes.set_xlabel("time(s)")
		self.sc2.axes.set_ylabel("pressure(psid)")


		#Little toolbar
		self.toolbar = NavigationToolbar(self.sc, self)
		# Widget for displaying the log.
		self.display = QTextEdit()
		self.display.setFontPointSize(16)
		self.display.setReadOnly(True)
		self.startButton = QPushButton("Start Taking Data")
		self.stopButton = QPushButton("Stop Taking Data")

		self.thread = QThread()
		self.worker = Worker()
		self.startButton.clicked.connect(self.buildPopup)
		self.stopButton.clicked.connect(self.worker.stop)
		self.startButton.clicked.connect(lambda: self.startButton.setEnabled(False))
		self.stopButton.clicked.connect(lambda: self.startButton.setEnabled(True))

		# The main layout is a grid (currently 2x2) with the various QGroupBoxes
		mainLayout = QGridLayout()
		mainLayout.addWidget(self.sc,0,3,2,2)
		mainLayout.addWidget(self.sc2,0,1,2,2)
		mainLayout.addWidget(self.startButton,0,0,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.stopButton,1,0,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.display,2,0,4,5)

		self.qTimer = QTimer()
		self.qTimer.setInterval(1000) # 1000 ms = 1 s
		self.qTimer.timeout.connect(self.reportProgress)
		self.qTimer.start()


		self.setLayout(mainLayout)
		self.setWindowTitle("Plane Leak Test")

		self.startDataTaking()

	def reportProgress(self):

		with open('buffer.dat',"r") as f:
			ss = f.readline()
			row = ss.split('\t')
			fields = len(row)
			if(fields < 10):
				return
			if(fields == 10):
				#print(ss)
				self.display.append(ss)
				self.p0Pressure.append(float(str(row[2])))
				self.p1Pressure.append(float(str(row[3])))
				self.p2Pressure.append(float(str(row[4])))
				self.p3Pressure.append(float(str(row[5])))
				self.p4Pressure.append(float(str(row[6])))
				self.p5Pressure.append(float(str(row[7])))
				self.refPressure.append(float(str(row[8])))
				self.refTemp.append(float(str(row[9])))
			if(len(self.refPressure) > 3600):
				self.refPressure.pop(0)
				self.p0Pressure.pop(0)
				self.p1Pressure.pop(0)
				self.p2Pressure.pop(0)
				self.p3Pressure.pop(0)
				self.p4Pressure.pop(0)
				self.p5Pressure.pop(0)
				self.refTemp.pop(0)
			elif(len(self.refPressure) <= 3600):
				xaxis = range(len(self.refPressure))
				self.data.set_xdata(xaxis)
				self.dataTemp.set_xdata(xaxis)
				self.dataP0.set_xdata(xaxis)
				self.dataP1.set_xdata(xaxis)
				self.dataP2.set_xdata(xaxis)
				self.dataP3.set_xdata(xaxis)
				self.dataP4.set_xdata(xaxis)
				self.dataP5.set_xdata(xaxis)
			self.sc.axes.set_ylim([min(self.refPressure)-1.,max(self.refPressure)+1.])
			self.sc.axes.set_xlim([0.,len(self.refPressure)+1.])
			self.sc.axes2.set_ylim([18,22])
			self.sc2.axes.set_ylim([min(self.p0Pressure)-2.,max(self.p0Pressure)+2.])
			self.sc2.axes.set_xlim([0.,len(self.p0Pressure)+1.])
			self.data.set_ydata(self.refPressure)
			self.dataTemp.set_ydata(self.refTemp)
			self.dataP0.set_ydata(self.p0Pressure)
			self.dataP1.set_ydata(self.p1Pressure)
			self.dataP2.set_ydata(self.p2Pressure)
			self.dataP3.set_ydata(self.p3Pressure)
			self.dataP4.set_ydata(self.p4Pressure)
			self.dataP5.set_ydata(self.p5Pressure)
			self.sc2.axes.legend(loc="upper right")
			self.sc.fig.tight_layout()
			self.sc.fig.canvas.draw()
			self.sc.fig.canvas.flush_events()
			self.sc2.fig.tight_layout()
			self.sc2.fig.canvas.draw()
			self.sc2.fig.canvas.flush_events()

	def setFileName(self):
		fn = self.PopupWindow.fileName.text() + str(".txt")
		existingFilenames = glob.glob("*.txt")
		random.seed(datetime.datetime.now())
		for n in existingFilenames:
			if n == fn:
				print("File with same name exists")
				rndmTrail = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(3))
				fn = fn.strip(".txt") + rndmTrail + ".txt"
		print("Saving data to : ", fn)
		self.worker.saveData(fn)
		self.PopupWindow.fileName.clear()
		self.PopupWindow.close()
		del self.PopupWindow

	def buildPopup(self):
		self.PopupWindow = Popup()
		self.PopupWindow.setGeometry(100, 200, 500, 100)
		self.PopupWindow.fileName.returnPressed.connect(self.setFileName)
		self.PopupWindow.show()

	def startDataTaking(self):

		#Worker 1 to run pressure grabber
		self.worker.moveToThread(self.thread)
		self.thread.started.connect(self.worker.run)
		self.worker.finished.connect(self.thread.quit)
		self.worker.finished.connect(self.worker.deleteLater)
		self.thread.finished.connect(self.thread.deleteLater)
		self.thread.start()


import sys

from PyQt5.QtWidgets import QApplication

app = QApplication(sys.argv)
gui = GUI()
gui.show()

sys.exit(app.exec_())   


