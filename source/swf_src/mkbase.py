import struct,sys
def rect(xmin,xmax,ymin,ymax):
    vals=[xmin,xmax,ymin,ymax]
    n=max(v.bit_length() for v in vals)+1
    bits=format(n,'05b')+''.join(format(v&((1<<n)-1),'0%db'%n) for v in vals)
    bits+= '0'*((8-len(bits)%8)%8)
    return bytes(int(bits[i:i+8],2) for i in range(0,len(bits),8))
def tag(code,data):
    if len(data)<63: return struct.pack('<H',(code<<6)|len(data))+data
    return struct.pack('<H',(code<<6)|63)+struct.pack('<I',len(data))+data
body=rect(0,25600,0,14400)+struct.pack('<HH',30<<8,1)
body+=tag(9,bytes([0,0,0]))+tag(12,b'\x00')+tag(1,b'')+tag(0,b'')
swf=b'FWS'+bytes([8])+struct.pack('<I',8+len(body))+body
open(sys.argv[1],'wb').write(swf)
