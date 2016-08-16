import pygame
import Server_Controller_Module
import Server_Webcam_Module
import time
import threading

server_quit = False

def ControllerThreadLoop(Controller):
    global server_quit
    
    print("Controller Module Now Running\n")
    while Controller.done == False  and server_quit == False:
        try:
            Controller.RunSingleIteration()
            time.sleep(0.1)
        except KeyboardInterrupt:
            server_quit = True
            exit()
    
    server_quit = True
    exit()
    
def CameraThreadLoop(Camera):
    global server_quit
    
    while Camera.done == False and server_quit == False:
        try:
            Camera.RunSingleIteration()
            time.sleep(0.03)
        except KeyboardInterrupt:
            server_quit = True
            exit()
            
    server_quit = True
    exit()
    
pygame.init()

Controller = Server_Controller_Module.Drone_Controller('192.168.0.205',1991)
Server_Cam = Server_Webcam_Module.Webcam_Receiver('',9001)


Control_thread = threading.Thread(target=ControllerThreadLoop,args=(Controller,))

Control_thread.start()
CameraThreadLoop(Server_Cam)

Control_thread.join()

Controller.ClassClose()
Server_Cam.ClassClose()

pygame.quit()
exit()