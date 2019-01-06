import argparse
import datatools
import os.path
from server import Server

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='OpenMCS recording server. This server can be used to record data to a seperate location or to directly render it via the OpenMCS Blender Plugin.',
    epilog='This program is part of the OpenMCS project. For more information, visit https://github.com/coretool/openmcs')
    parser.add_argument('-n', type=int, help='Number of sensors to connect to', required=True)
    parser.add_argument('-p', '--port', type=int, help='Port to listen on', default=3000)
    parser.add_argument('-db', '--database', help='Database to write to', default=':memory:')
    parser.add_argument('-t', '--tablename', help='Table to write to', default='sensordata')
    parser.add_argument('--format', help='Format of the recording', default='both', choices=['quaternion', 'raw', 'both'])
    args = vars(parser.parse_args())
    
    server = Server(args['n'], args['port'], args['database'], args['tablename'], args['format'])  # TODO: Take other arguments into account
    print('Server is waiting for the sensors to connect ') 
    server.run()
    input('All sensors are now connected, hit enter to continue and start the recording ')
    server.ready()
    try:
        print('Hit Ctrl + C to end the recording')
        server.receive()
    except KeyboardInterrupt:
        pass
    server.switch_off()
    server.terminate()
    
    exit(0)
