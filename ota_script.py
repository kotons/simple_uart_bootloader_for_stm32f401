import serial
import os
import math
import binascii
from time import sleep
import datetime

try:
    ser = serial.Serial('COM28',baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS)
except:
    print("Serial Port not available")
    exit()

import binascii

def CRC32_from_file(filename):
    buf = open(filename,'rb').read()
   


# Packet_Frame_Structure = {'SF':1,"EF":2}
SF = 1
EF = 2
Response_codes = {"CALLBACK_TIMEOUT_OCCURED" : 10, 
                    "CALLBACK_RECEVIED_FOR_WRONG_CMD": 11,
                    "CALLBACK_RECEVIED_WRONG_FRAME_STRUCTURE": 12 }

DATA_FILE = "sample_application.bin"
FILE_SIZE = int(os.stat(DATA_FILE).st_size)
MAX_BYTES_TO_SEND = 8
data = []
def make_data_frame_n_send(cmd,data,length):
    s_frame = SF.to_bytes(2, 'little')
    e_frame = EF.to_bytes(2, 'little')
    cmd_frame = cmd.to_bytes(2, 'little')
    chunk_size_frame = length.to_bytes(2,'little')
    data_frame = data.to_bytes(2, 'little')
    frame = s_frame+cmd_frame+data_frame+chunk_size_frame+e_frame
    print(len(frame))
    print(frame)
    ser.write(frame)
    return frame

def waiting_for_callback(waiting_callback_for_cmd):
    process_start_time =  datetime.datetime.now()
    while(1):
        # Reading 4 bytes for callback
        callback_data_packet = ser.read(4)
        # Start timeout timer
        timedelta = datetime.datetime.now() - process_start_time
        # Verify if any data received in callback packet
        if(callback_data_packet[0] != 0x00):
            #Process the UART callback 
            # Check frame structure

            if((callback_data_packet[0] == SF) and (callback_data_packet[3] == EF)):

                #If packet is correct then start futher proccessing
                recevied_callback_for_cmd = callback_data_packet[1]
                callback_status = callback_data_packet[2]

                # Compare recevied callback command must be equal to expected callback for sent command
                if(recevied_callback_for_cmd == waiting_callback_for_cmd):
                    print(f"Callback status-:{callback_status}")
                    return callback_status
                else:
                    # 
                    print("Callback recevide for different command")
                    return Response_codes["CALLBACK_RECEVIED_FOR_WRONG_CMD"]
            else:
                # If Start frame and End frame is not correct then throw the wrong frame structure
                print("Wrong frame structure")
                return Response_codes["CALLBACK_RECEVIED_WRONG_FRAME_STRUCTURE"]

        # If no data is received on the uart for 5 seconds then throw the timeout error
        elif(timedelta.total_seconds() >= 5):
            print("Timeout occure issue in UART communication")
            return Response_codes["CALLBACK_TIMEOUT_OCCURRED"]

with open(DATA_FILE, "rb") as f:
    NO_OF_PARTS = int(math.ceil(FILE_SIZE / float(MAX_BYTES_TO_SEND)))
    while(len(data) < FILE_SIZE):
        byte = f.read(1)  # Reading binary from file
        # print(byte)
        buf = (binascii.crc32(byte) & 0xFFFFFFFF)
        byte = binascii.hexlify(byte) # change the binary to hex
        data.append(byte)  # Add that value to data list
    print(buf)

Current_sent_part  = 1
current_packet = 0

print(" ** OTA Script Started **\n Select Option Below \n \
         1 - Settiing OTA Mode \n \
         2 - Start Updating Firmware \n \
         3 - Start the application or Press the user button on the board")

while(1):
    temp = input("Press enter to start ota- ")
    if(temp == '1'):
        print('setting OTA mode')
        make_data_frame_n_send(1,NO_OF_PARTS,0)
        if(waiting_for_callback(1) == 0):
            pass
        else:
            print("Stpped OTA process due to error")
            break
    elif (temp == '2'):
        print("Start to updating firmware")
        while(NO_OF_PARTS >= Current_sent_part):
            print(f"Sending new data frame - {Current_sent_part} of {NO_OF_PARTS}.")
            data_to_be_send = data[current_packet:current_packet+MAX_BYTES_TO_SEND]
            data_to_be_send.reverse()
            if(len(data_to_be_send) > 0):
                print(data_to_be_send)
                for i in range (0,len(data_to_be_send)):
                    packet_int = int(data_to_be_send[i], 16)
                    # input("Click to send next packet")
                    make_data_frame_n_send(2,packet_int,len(data_to_be_send))
                    # sleep(0.05)
                # NO error condition
                if(waiting_for_callback(2) == 0):
                    pass
                else:
                    print("Stopped OTA process due to error")
                    break
                current_packet = current_packet+MAX_BYTES_TO_SEND
                Current_sent_part = Current_sent_part +1
        print("Firmware update completed")
        temp = 0
    elif (temp == '3'):
        print("Start the application code")
        command=b"\x04\x02"
        ser.write(command)
    else:
        print("Wrong command")
