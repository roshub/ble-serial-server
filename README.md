# ble-serial-server

## Installation
To install, run `sudo snap install roshub-ble-serial-server`

Then, run:
```
sudo snap connect roshub-ble-serial-server:bluetooth-control :bluetooth-control
sudo snap connect roshub-ble-serial-server:bluez :bluez
sudo snap connect roshub-ble-serial-server:network-observe :network-observe
```

## Running
To run the application, do:
```
sudo roshub-ble-serial-server.ble-serial-server
```
