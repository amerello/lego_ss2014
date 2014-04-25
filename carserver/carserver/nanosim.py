import sys
import struct
import threading
import socket
import imp
import time



def read(x, bytes):
  (format,names) = x
  return dict(zip(names,struct.unpack(format,bytes)))

def write(x, dict):
  (format,names) = x
  y = list()
  for x in names:
   y.append(dict[x])
  return struct.pack(format,*y)

def parse_Variables(strs):
  def f(str):
    (dt,name)=str.split(' ',1)
    d = dict([('padding','x'),('char','b'),('alt_8','b'),('alt_u8','B'),('bool','?'),('short','h'),('ushort','H'),
      ('alt_u16','H'),('int','i'),('uint','I'),('long','l'),('ulong','L'),('longlong','q'),('ulonglong','Q'),
      ('float','f'),('double','d')])
    return (d[dt],name)
  x = map(f,strs)
  x=[y for y in x] # do this to make sure that mapping object get a list, otherwise zip(*x) won't work.
  [dts,names] = zip(*x)
  dts1 = ""
  for x in dts:
    dts1= dts1+x
  return (dts1,names)
  
def get_in_out(str):
      strs = str.split('\n')
      n1 = 0
      n2 = 0
      n3 = len(strs);
      for i in range(len(strs)):
        if strs[i] == 'OUTPUT:':
          n1 = i
        if strs[i] == 'INPUT:':
          n2 = i
        if strs[i] == '':
          n3 = i
          break
      return(strs[n1+1:n2],strs[n2+1:n3])
  
HOST = ''
PORT = 23

string = """FRONT LEFT SIDE
OUTPUT:
bool connBroken
bool enMotorController
ushort ubat
short accX
short accY
short accZ
short accXsum
short accYsum
short accZsum
uint duty
uint network_timeout
int X
int W
int P
int I
int D
int FF
float distance
INPUT:
alt_u8 LED
bool enConnBrokenCtr
bool setEnMotorController
int W

"""
# Create a socket (SOCK_STREAM means a TCP socket)
class Nano_Board(threading.Thread):
    def __init__(self,port):
      threading.Thread.__init__(self)
      self.port = port
      self.input = dict()
      self.output = dict([('connBroken',False),('enMotorController',False),('ubat',1203),('accX',0),('accY',0),('accZ',980),('duty',123),('network_timeout',23),
                          ('X',21),('W',32),('P',33),('I',3),('D',0),('FF',0),('distance',1.23),
                          ('accXsum',1),('accYsum',2),('accZsum',3)])
      self.running = True
    
    
    def run(self):  
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            (output,input)= get_in_out(string)
            #print("input:",input," output: ",output)
            input_p = parse_Variables(input)
            output_p = parse_Variables(output)
            (struct_str,_) = input_p
            insize = struct.calcsize(struct_str)
            insize = insize +1
            
            s.bind((HOST,self.port))
            s.listen(1)
            
            conn, addr = s.accept()
            print('Connected by', addr)
            m = b''
            while m.decode('utf-8') != 'I':
             m = conn.recv(1)
             print('received data:',m)
            print('send Data:',"Nano Board Version 1.0\n");
            conn.sendall("Nano Board Version 1.0\n")
            
            while True:
                indat =b''
                while len(indat) != insize:
                  indat = indat + conn.recv(insize- len(indat))
                  self.input = read(input_p,indat[1:])
                  self.output['W'] = self.input['W']
                  time.sleep(0.1)
                  conn.sendall('R'+ write(output_p,self.output))
            
        finally:
            s.close()
            self.active = False
def main():
  b1 = Nano_Board(2012)
  b2 = Nano_Board(2013)
  b3 = Nano_Board(2014)
  b4 = Nano_Board(2015)
  b1.start()
  b2.start()
  b3.start()
  b4.start()
