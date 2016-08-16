import pygame
import socket
import struct
import time

def joystickSetup():
    pygame.joystick.init()
    joystick = pygame.joystick.Joystick(0)
    joystick.init()
    return joystick

def clientSetup():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    except socket.error:
        print("Failed to create socket")
        sock.close()
        pygame.quit()
        exit()
    
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    return sock

def UDPSend(sock, HOST, PORT, packet):
    try:
        sock.sendto(packet,(HOST, PORT))
    except socket.error as msg:
        print(msg)

class Drone_Controller(object):
    
    def __init__(self, host = '', port = 1881):
        self.SERVERHOST = host
        self.SERVERPORT = port
        self.sock = clientSetup()
        self.joystick = joystickSetup()
        self.packer = struct.Struct('f f f f ? ? ?')
        self.done = False
    
    def ClassExit(self):
        self.sock.close()
        pygame.quit()
        exit()
        
    def ClassClose(self):
        self.sock.close()
        
    def TestConnection(self):
        try:
            print("Sending connection Signal...")
            signal = 'Remote Signal'
            UDPSend(self.sock, self.SERVERHOST, self.SERVERPORT, signal.encode('UTF-8'))
        except socket.error as msg:
            print(msg)
            self.ClassExit()
        except KeyboardInterrupt:
            self.ClassExit()
            
        input('Press Enter to Continue')
        
    def RunController(self):
        self.TestConnection()
        print("Connection Successful")
        exit_button = False
        
        while self.done == False:
            
            for event in pygame.event.get(): # User did something
                # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                if event.type == pygame.KEYDOWN: #and event.key == pygame.K_ESCAPE:
                    self.done = True
             
            axis_lx = self.joystick.get_axis(0)
            axis_ly = self.joystick.get_axis(1)
            axis_rx = self.joystick.get_axis(2)
            axis_ry = self.joystick.get_axis(3)
            but_share = self.joystick.get_button(8)
            but_option = self.joystick.get_button(9)
            but_cam_stick = self.joystick.get_button(11)
            but_exit_touch = self.joystick.get_button(13)
        
            if but_share == True and but_option == True and exit_button == False:
                exit_button = True
                print("Ending Connection")
            
            if but_exit_touch == True and exit_button == True:
                self.done = True
                
            axis_pair = (axis_lx, axis_ly, axis_rx, axis_ry, but_cam_stick, exit_button)
            axis_pack = self.packer.pack(*axis_pair)
            
            UDPSend(self.sock, self.SERVERHOST, self.SERVERPORT, axis_pack)
            time.sleep(0.05)
        self.ClassExit()
        
    def RunSingleIteration(self):
        for event in pygame.event.get(): # User did something
            # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
            if event.type == pygame.KEYDOWN: #and event.key == pygame.K_ESCAPE:
                self.done = True
                
        exit_button = False
        axis_lx = self.joystick.get_axis(0)
        axis_ly = self.joystick.get_axis(1)
        axis_rx = self.joystick.get_axis(2)
        axis_ry = self.joystick.get_axis(3)
        but_share = self.joystick.get_button(8)
        but_option = self.joystick.get_button(9)
        but_cam_stick = self.joystick.get_button(11)
        but_toggle = self.joystick.get_button(13)
        
        if but_share == True and but_option == True:
            self.done = True
            exit_button = True
            print("Ending Connection")
        
        axis_pair = (axis_lx, axis_ly, axis_rx, axis_ry, but_cam_stick, exit_button, but_toggle)
        axis_pack = self.packer.pack(*axis_pair)
        
        UDPSend(self.sock, self.SERVERHOST, self.SERVERPORT, axis_pack)