	#HCG Serial readout

import serial
import time
import numpy as np
import csv
import datetime as dt
import sys
import threading

port = 'COM4'
baudrate = 9600
timeout = 5

class Grabber():

	def __init__(self,p,b,t):

		self.port = p
		self.baud = b
		self.timeout = t
		self.ser = serial.Serial(port=self.port,
		     		            baudrate=self.baud,
		             		    timeout=self.timeout,
		                 		bytesize=serial.EIGHTBITS,
		                 		parity=serial.PARITY_NONE,
		                 		stopbits=serial.STOPBITS_ONE)

		self.time_ = [time.strftime("%H:%M:%S", time.localtime())]
		self.datastring = ""
		self.write = False
		self.fn = ""
		self.lock = threading.Lock()

	def getOutputStream(self):
		return self.datastring

	def writeToFile(self,name):
		self.f = open(name,'w+')
		self.write = True

	def stopWriteToFile(self):
		self.write = False


	def get_transmission(self):

		#close this for now
		sensor_data = self.ser.readline();
		self.datastring = sensor_data.decode("utf-8");
		#print(self.datastring)
		#fake it
		# check = np.random.random_integers(190)
		# p0 = np.random.rand(1)
		# p1 = np.random.rand(1)
		# p2 = np.random.rand(1)
		# p3 = np.random.rand(1)
		# p4 = np.random.rand(1)
		# p5 = np.random.rand(1)
		# pRef = 30.0 + np.random.rand(1)
		# tRef = 21.0 + np.random.rand(1)

		#self.datastring = str(check) + "\t" + str(p0).strip("[]") + "\t" + str(p1).strip("[]") + "\t" + str(p2).strip("[]") + "\t" + str(p3).strip("[]") + "\t" + str(p4).strip("[]") + "\t" + str(p5).strip("[]") + "\t" + str(pRef).strip("[]") + "\t" + str(tRef).strip("[]") + "\n"
		if self.datastring != "":

			self.buffer = open('buffer.dat','w')

			timestamp = time.strftime("%m-%d-%Y %H:%M:%S", time.localtime());
			readout = timestamp + "\t" + self.datastring
			self.lock.acquire()
			self.buffer.write(readout)
			self.buffer.flush()
			self.buffer.close()
			self.lock.release()
			if(self.write):
				self.f.write(readout)
				self.f.flush()
			#add sleep for faking the data stream
			#time.sleep(1)
