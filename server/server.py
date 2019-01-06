import datatools
import socket
import sqlite3

# Standalone recorder #
# Part of the OpenMCS project #
# Visit https://www.github.com/coretool/openmcs for more information #

class Server(object):
    def __init__(self, number_of_sensors, port, db=':memory:', db_name='sensordata', type='raw'):
        self.port = port
        self.num_of_sensors = number_of_sensors
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(('0.0.0.0', port))
        self.shutdown = False

        self.db_name = db_name
        self.conn = sqlite3.connect(db)
        self.cursor = self.conn.cursor()

        self.connected_sensors = []
        self.sensor_ids = []

        self.type = type

    def run(self):
        """
        Top-level function to start the whole process.
        """
        if self.type == 'quat':
            self.cursor.execute('CREATE TABLE ' + self.db_name + 
                            ''' (id integer, w real, x real, y real, z real, generation integer)''')

        elif self.type == 'raw':
            self.cursor.execute('CREATE TABLE ' + self.db_name + 
                            ''' (id integer, ax real, ay real, az real, gx real, gy real, gz real, mx real, my real, mz real, generation integer)''')            

        else:  # Create two tables for a combined recording
            self.cursor.execute('CREATE TABLE ' + self.db_name + '_euler' +
                            ''' (id integer, x real, y real, z real, generation integer)''')

            self.cursor.execute('CREATE TABLE ' + self.db_name + '_raw' +
                            ''' (id integer, ax real, ay real, az real, gx real, gy real, gz real, mx real, my real, mz real, generation integer)''') 
        
        self.cursor.execute('CREATE TABLE ' + self.db_name + '_init' + 
                        ''' (id integer, x real, y real, z real) ''')
        
        self.wait_for_sensors()

    def wait_for_sensors(self):
        """
        Method to wait for the sensors to register. Upon successful
        registration of all sensors, Server.ready() will be called.
        """
        while len(self.connected_sensors) < self.num_of_sensors:
            data, addr = self.socket.recvfrom(100)
            values = datatools.unpack_init(data)
            self.connected_sensors.append(addr)
            self.cursor.execute('INSERT INTO ' + self.db_name + '_init VALUES ' + str(values))
            self.conn.commit()
            self.sensor_ids.append(values[0])
            
    def ready(self):
        """
        Tell all sensors that we are ready for transmission. Upon
        recveiving the ready call, the sensor will wait another 
        5 seconds until it sends data.
        """
        for sensor in self.connected_sensors:
            self.socket.sendto(b'ready', sensor)
    
    def receive(self):

        print('The recording has been started')
        while not self.shutdown:
            data, addr = self.socket.recvfrom(100)
            if self.type == 'quat':
                data = str(datatools.unpack_quaternion(data))
                self.cursor.execute('INSERT INTO ' + self.db_name + ' VALUES ' + data)
                self.conn.commit()
        
            elif self.type == 'raw':
                data = str(datatools.unpack_raw(data))
                self.cursor.execute('INSERT INTO ' + self.db_name + ' VALUES ' + data)
                self.conn.commit()
            
            else: 
                data = datatools.unpack_both(data)
                print(data)
                quat = [data[0]] + list(data[10:13]) + [data[13]]
                quat = tuple(quat)
                print(quat)
                self.cursor.execute('INSERT INTO ' + self.db_name + '_euler VALUES ' + str(quat))
                self.conn.commit()
                raw = [data[0]] + list(data[1:10]) + [data[13]]
                print(raw)
                raw = tuple(raw)
                self.cursor.execute('INSERT INTO ' + self.db_name + '_raw VALUES ' + str(raw))
                self.conn.commit()

    def switch_off(self):
        """
        Tell all sensors that we are done. Each sensor checks whether it
        has received a message after it has sent its most recent measurement.
        """
        self.shutdown = True
        for sensor in self.connected_sensors:
            self.socket.sendto(b'end', sensor)
    
    def terminate(self):
        self.conn.close()
