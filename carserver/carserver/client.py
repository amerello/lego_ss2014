import sys
import struct
import threading
import socket
import imp
import os,os.path
import SimpleHTTPServer
import SocketServer
import json
import math


PORT = 80

class MyHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    #def __init__(self,req,client_addr,server):
    #   SimpleHTTPRequestHandler.__init__(self,req,client_addr,server)      
    def do_POST(self):
        global sock
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        print (post_data)
        # read received values and make left/right out of dx/dy:
        l = []
        for t in post_data.split():
            try:
                l.append(int(t))
            except ValueError:
                pass
        [dx,dy] = l
        phi = math.atan2(dy,dx)
        v = math.sqrt(dx**2+dy**2)
        if v > 200 :
          v = 200
        vr = math.cos(phi+3*math.pi/4)*v/200
        vl = math.cos(phi+math.pi/4)*v/200
        print('motor left: ',vl,' motor right: ',vr)
        L = vl
        R = vr
        r = """{"distance_front":0,"distance_rear":0,"ubat_power":0,"ubat_compute":0,"v_left":0,"v_right":0,"board_status":[0,0,0,0],"acc":[0,0,0]}"""
        try:
          sock.sendall(json.dumps((L,R))+'\0')
          r = sock.recv(1024)
        except socket.error:
          if os.path.exists( "carserver_socket" ):
            sock.close()
            sock = socket.socket(socket.AF_UNIX,socket.SOCK_SEQPACKET)
            sock.connect("carserver_socket")
            try:
              sock.sendall(json.dumps((L,R))+'\0')
              r = sock.recv(1024)
            except socket.error:
              r = "Could not connect to Carserver.c"
        
        #r = json.dumps(boards[2].fromBoard)
        
        self.send_response(200)
        self.send_header("Content-type", "application/json")

        self.send_header("Content-length", len(r))
        self.end_headers()
        self.wfile.write(r.encode("utf-8"))
        self.wfile.flush()
        print(r)
        
    
httpd = SocketServer.TCPServer(("11.0.0.1", PORT), MyHandler)

sock = socket.socket(socket.AF_UNIX,socket.SOCK_SEQPACKET)
if os.path.exists( "carserver_socket" ):
    try:
     sock.connect("carserver_socket");
    except socket.error:
      os.remove("carserver_socket");
      print "carserver_socket does exist, but could not connect to it.\n delete it..." 
else:
    print "did not find carserver_socket."
print("serving at port", PORT)
try:
    httpd.serve_forever()
finally:
    sock.close()
