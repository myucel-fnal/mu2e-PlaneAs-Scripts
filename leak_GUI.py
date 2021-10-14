###################################################
# Plane assembly leak GUI - M.Yucel 10/8/21       #
# Watch serial output of pressure and temp sensors#
# from the plane as leak setup. Display, plot and #
# save sensor data.                               #
################################################### 
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
		self.scRef = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.refPressure = []
		self.refTemp = []
		self.time = []
		self.data, = self.scRef.axes.plot([], [],"g-", label="Ref. pressure")
		self.dataTemp, = self.scRef.axes2.plot([], [], "b-", label="Ref. temperature")
		self.scRef.axes.tick_params(axis='y', labelcolor='green')
		self.scRef.axes2.tick_params(axis='y', labelcolor='blue')
		self.scRef.axes.set_xlabel("time(s)")
		self.scRef.axes.set_ylabel("pressure(psia)",color='green')
		self.scRef.axes2.set_ylabel("temp(C)",color='blue')

		#set up diff pressure plot
		self.scP0 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.scP1 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.scP2 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.scP3 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.scP4 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.scP5 = MplCanvas(self, width=3, height=2.5, dpi=100)
		self.p0Pressure = []
		self.p1Pressure = []
		self.p2Pressure = []
		self.p3Pressure = []
		self.p4Pressure = []
		self.p5Pressure = []
		self.dataP0, = self.scP0.axes.plot([], [],"y-", label="Panel-0")
		self.dataP1, = self.scP1.axes.plot([], [], "r-", label="Panel-1")
		self.dataP2, = self.scP2.axes.plot([], [], "c-", label="Panel-2")
		self.dataP3, = self.scP3.axes.plot([], [], "b-", label="Panel-3")
		self.dataP4, = self.scP4.axes.plot([], [], "m-", label="Panel-4")
		self.dataP5, = self.scP5.axes.plot([], [], "k-", label="Panel-5")
		self.scP0.axes.set_xlabel("time(s)")
		self.scP0.axes.set_ylabel("diff. pressure(mbar)")
		self.scP1.axes.set_xlabel("time(s)")
		self.scP1.axes.set_ylabel("diff. pressure(mbar)")
		self.scP2.axes.set_xlabel("time(s)")
		self.scP2.axes.set_ylabel("diff. pressure(mbar)")
		self.scP3.axes.set_xlabel("time(s)")
		self.scP3.axes.set_ylabel("diff. pressure(mbar)")
		self.scP4.axes.set_xlabel("time(s)")
		self.scP4.axes.set_ylabel("diff. pressure(mbar)")
		self.scP5.axes.set_xlabel("time(s)")
		self.scP5.axes.set_ylabel("diff. pressure(mbar)")
		#Little toolbar
		self.toolbar = NavigationToolbar(self.scRef, self)
		# Widget for displaying the log.
		fs = 16
		self.displayRef = QLabel()
		self.displayP0 = QLabel()
		self.displayP1 = QLabel()
		self.displayP2 = QLabel()
		self.displayP3 = QLabel()
		self.displayP4 = QLabel()
		self.displayP5 = QLabel()
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
		mainLayout.addWidget(self.scRef,0,2,1,3)
		mainLayout.addWidget(self.displayRef,2,2,1,3,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.scP0,3,0,2,2)
		mainLayout.addWidget(self.scP1,3,2,2,2)
		mainLayout.addWidget(self.scP2,3,4,2,2)
		mainLayout.addWidget(self.displayP0,5,0,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.displayP1,5,2,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.displayP2,5,4,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.scP3,6,0,2,2)
		mainLayout.addWidget(self.scP4,6,2,2,2)
		mainLayout.addWidget(self.scP5,6,4,2,2)
		mainLayout.addWidget(self.displayP3,8,0,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.displayP4,8,2,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.displayP5,8,4,1,2,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.startButton,0,0,alignment=Qt.AlignCenter)
		mainLayout.addWidget(self.stopButton,0,1,alignment=Qt.AlignCenter)

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
				self.displayP0.setText(str(row[2]))
				self.displayP1.setText(str(row[3]))
				self.displayP2.setText(str(row[4]))
				self.displayP3.setText(str(row[5]))
				self.displayP4.setText(str(row[6]))
				self.displayP5.setText(str(row[7]))
				self.displayRef.setText(str(row[8]+"\t"+ row[9]))
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
			self.scRef.axes.set_ylim([min(self.refPressure)-1.,max(self.refPressure)+1.])
			self.scRef.axes.set_xlim([0.,len(self.refPressure)+1.])
			self.scRef.axes2.set_ylim([18,22])
			self.scP0.axes.set_ylim([min(self.p0Pressure)-2.,max(self.p0Pressure)+2.])
			self.scP0.axes.set_xlim([0.,len(self.p0Pressure)+1.])
			self.scP1.axes.set_ylim([min(self.p1Pressure)-2.,max(self.p1Pressure)+2.])
			self.scP1.axes.set_xlim([0.,len(self.p1Pressure)+1.])
			self.scP2.axes.set_ylim([min(self.p2Pressure)-2.,max(self.p2Pressure)+2.])
			self.scP2.axes.set_xlim([0.,len(self.p2Pressure)+1.])
			self.scP3.axes.set_ylim([min(self.p3Pressure)-2.,max(self.p3Pressure)+2.])
			self.scP3.axes.set_xlim([0.,len(self.p3Pressure)+1.])
			self.scP4.axes.set_ylim([min(self.p4Pressure)-2.,max(self.p4Pressure)+2.])
			self.scP4.axes.set_xlim([0.,len(self.p4Pressure)+1.])
			self.scP5.axes.set_ylim([min(self.p5Pressure)-2.,max(self.p5Pressure)+2.])
			self.scP5.axes.set_xlim([0.,len(self.p5Pressure)+1.])
			self.data.set_ydata(self.refPressure)
			self.dataTemp.set_ydata(self.refTemp)
			self.dataP0.set_ydata(self.p0Pressure)
			self.dataP1.set_ydata(self.p1Pressure)
			self.dataP2.set_ydata(self.p2Pressure)
			self.dataP3.set_ydata(self.p3Pressure)
			self.dataP4.set_ydata(self.p4Pressure)
			self.dataP5.set_ydata(self.p5Pressure)
			self.scP0.axes.legend(loc="upper right")
			self.scP1.axes.legend(loc="upper right")
			self.scP2.axes.legend(loc="upper right")
			self.scP3.axes.legend(loc="upper right")
			self.scP4.axes.legend(loc="upper right")
			self.scP5.axes.legend(loc="upper right")
			self.scRef.fig.tight_layout()
			self.scRef.fig.canvas.draw()
			self.scRef.fig.canvas.flush_events()
			self.scP0.fig.tight_layout()
			self.scP0.fig.canvas.draw()
			self.scP0.fig.canvas.flush_events()
			self.scP1.fig.tight_layout()
			self.scP1.fig.canvas.draw()
			self.scP1.fig.canvas.flush_events()
			self.scP2.fig.tight_layout()
			self.scP2.fig.canvas.draw()
			self.scP2.fig.canvas.flush_events()
			self.scP3.fig.tight_layout()
			self.scP3.fig.canvas.draw()
			self.scP3.fig.canvas.flush_events()
			self.scP4.fig.tight_layout()
			self.scP4.fig.canvas.draw()
			self.scP4.fig.canvas.flush_events()
			self.scP5.fig.tight_layout()
			self.scP5.fig.canvas.draw()
			self.scP5.fig.canvas.flush_events()

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


