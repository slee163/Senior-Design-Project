import socket
import pygame

pygame.init()

def serverInit(host, port):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except socket.error as msg:
        print ('Socket Create failed')
        print(msg)
        sock.close()
        pygame.quit()
        exit()
            
    try:
        sock.bind((host,port))
    except socket.error as msg:
        print ('Bind failed')
        print(msg)
        sock.close()
        pygame.quit()
        exit()
    
    return sock
        
class Webcam_Receiver(object):
    
    def __init__(self, host = '', port = 9001):
        self.SERVERHOST = host
        self.SERVERPORT = port
        self.screen = pygame.display.set_mode((640,360),0)
        self.sock = serverInit(self.SERVERHOST, self.SERVERPORT)
        self.sock.listen(2)
        self.done = False
        
        print("Remote Server is active")
        print('Host Address: ', self.SERVERHOST)
        print('Port Number: ', self.SERVERPORT)
    
    def ClassExit(self):
        self.sock.close()
        pygame.quit()
        exit()
    
    def ClassClose(self):
        self.sock.close()
    
    def RunWebcamServer(self):
        try:
            while True:
                connection, addr = self.sock.accept()
                received = []
                
                while True:
                    chunk = connection.recv(691200)
                    if not chunk:
                        break
                    else:
                        received.append(chunk)
                frame_string = b''.join(received)
                frame_image = pygame.image.fromstring(frame_string,(640,360),"RGB")
                self.screen.blit(frame_image,(0,0))
                pygame.display.update()
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        self.ClassExit()
        except KeyboardInterrupt:
            self.ClassExit()
            
    def RunSingleIteration(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.done = True
                
        connection, addr = self.sock.accept()
        received = []
        
        while True:
            chunk = connection.recv(691200)
            if not chunk:
                break;
            else:
                received.append(chunk)
        
        frame_string = b''.join(received)
        frame_image = pygame.image.fromstring(frame_string,(640,360),"RGB")
        self.screen.blit(frame_image,(0,0))
        pygame.display.update()
        
        
        