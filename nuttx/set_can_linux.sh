echo '#!/bin/bash
sudo killall slcand 2>/dev/null
sudo slcand -o -c -s4 /dev/ttyACM1 can0
sudo ip link set can0 up' > ~/can_setup.sh
chmod +x ~/can_setup.sh
