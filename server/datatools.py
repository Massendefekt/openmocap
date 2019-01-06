import math

def quartet_to_int(q):
    """
    Converts the raw data back into integers
    """
    r = 0
    r += q[0] << 24
    r += q[1] << 16
    r += q[2] << 8
    r += q[3]  # reverse of the push function in the sensors firmware
    if q[4] == 1:
        return r * -1
    else:
        return r

def unpack_init(packet):
    id = quartet_to_int(packet[:5])
    x = quartet_to_int(packet[5:10]) / (10 ** 4)
    y = quartet_to_int(packet[10:15]) / (10 ** 4)
    z = quartet_to_int(packet[15:20]) / (10 ** 4)
    return(id, x, y, z)

def unpack_quaternion(packet):
    """
    Unpacks the quaternion packet and returns a sqlite
    friendly tuple of values
    """
    # The first four bytes form the sensor id
    id = quartet_to_int(packet[:5])
    print(packet[5:10])
    w = quartet_to_int(packet[5:10]) / (10 ** 4)
    x = quartet_to_int(packet[10:15]) / (10 ** 4)
    y = quartet_to_int(packet[15:20]) / (10 ** 4)
    z = quartet_to_int(packet[20:25]) / (10 ** 4)
    gen = quartet_to_int(packet[25:30])
    return (id, w, x, y, z, gen)

def unpack_raw(packet):
    """
    Unpacks the raw packet and returns a sqlite 
    friendly tuple of values
    """
    # sensor id
    id = quartet_to_int(packet[:5])
    ax = quartet_to_int(packet[5:10]) / (10 ** 4)
    ay = quartet_to_int(packet[10:15]) / (10 ** 4)
    az = quartet_to_int(packet[15:20]) / (10 ** 4)
    gx = quartet_to_int(packet[20:25]) / (10 ** 4)
    gy = quartet_to_int(packet[25:30]) / (10 ** 4)
    gz = quartet_to_int(packet[30:35]) / (10 ** 4)
    mx = quartet_to_int(packet[35:40]) / (10 ** 4)
    my = quartet_to_int(packet[40:45]) / (10 ** 4)
    mz = quartet_to_int(packet[45:50]) / (10 ** 4)
    gen = quartet_to_int(packet[50:55]) / (10 ** 4)
    return (id, ax, ay, az, gx, gy, gz, mx, my, mz, gen)

def unpack_both(packet):
    """
    Unpacks a packet w/ raw and quaternion data and returns
    a sqlite friendly tuple of values
    """
    # sensor id
    id = quartet_to_int(packet[:5])
    # sensor data
    ax = quartet_to_int(packet[5:10]) / (10 ** 4)
    ay = quartet_to_int(packet[10:15]) / (10 ** 4)
    az = quartet_to_int(packet[15:20]) / (10 ** 4)
    gx = quartet_to_int(packet[20:25]) / (10 ** 4)
    gy = quartet_to_int(packet[25:30]) / (10 ** 4)
    gz = quartet_to_int(packet[30:35]) / (10 ** 4)
    mx = quartet_to_int(packet[35:40]) / (10 ** 4)
    my = quartet_to_int(packet[40:45]) / (10 ** 4)
    mz = quartet_to_int(packet[45:50]) / (10 ** 4)  # 9
    x = quartet_to_int(packet[50:55]) / (10 ** 4)
    y = quartet_to_int(packet[55:60]) / (10 ** 4)
    z = quartet_to_int(packet[60:65]) / (10 ** 4)  # 13
    # packet generation
    gen = quartet_to_int(packet[65:70]) / (10 ** 4)
    return (id, ax, ay, az, gx, gy, gz, mx, my, mz, x, y, z, gen)

def quaternion_to_euler_angles(w, x, y, z):
    """
    Converts quaternions into euler angles (used for bhv output)
    """
    x_0 = 2.0 * (w * x + y * z)
    x_1 = 1.0 - 2.0 * (x ** 2 + y ** 2)
    x = math.atan2(x_0, x_1)
    
    y = 2.0 * (w * y - z * x)
    if y > 1.0:
        y = 1.0
    if y < -1.0:
        y = -1.0
    y = math.asin(y)
    
    z_0 = 2.0 * (w * z + x * y)
    z_1 = 1.0 - 2.0 * (y ** 2 + z ** 2)
    z = math.atan2(z_0, z_1)
    return (x, y, z)
    
def find_all(string, sub):
    start = 0
    while True:
        start = string.find(sub, start)
        if start == -1:
            return
        yield start 
        start += len(sub)