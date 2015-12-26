# pirflink
Driver for linx pi card

pirflink handle my linx pi card listening and sending RF commands (https://www.linxtechnologies.com/en/products/modules/lt-rf-transceiver).

It emulate a RFLink hardware (but only support a few protocols for the devices I have ...) using TCP.

build_pirflinx is a simple command compiling and linking final executable (sorry no makefile yet ...) named pirflinx. pirflinx.sh a based script to be used as a daemon service script (/etc/init.d)

Get the code: git clone https://github.com/rimram31/pirflinx.git pirflinx
cd pirflinx
./build_pirflinx
mkdir ~/pirflinx
cp pirflinx ~/pirflinx
cp pirfinx.sh ~/etc/init.d
- Adapt as needed pirfinx.sh and add pirflinx as a service
sudo /etc/init.d/pirflinx # By default listen to 9999 port

You can now add a new hardware to domoticz work as a RFLink TCP hardware.

Nota: This program require pigpio library (http://abyz.co.uk/rpi/pigpio/)
