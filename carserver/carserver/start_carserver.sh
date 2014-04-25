#/bin/sh
ifconfig eth0 192.168.0.1
python client.py >/dev/null 2>clienterror.log &
./carserver >/dev/null 2>servererror.log &