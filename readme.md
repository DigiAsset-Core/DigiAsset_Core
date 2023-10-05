## install ubuntu

install ubuntu server using default settings(except add a 32GB swap partition).  You can probably get away with 8GB swap if low on storage space.  If someone tests with 8GB please change these instructions.

## Bugs found in js code

- There was a bug in bitio that caused 7 and 8 byte numbers encoded in assets to be wrong.  This only effected the following txids.
93d0407a6bf511a0eefe5a00d56965991b0a22df2fdcb74bbf025cae14549123
- This didn't indirectly effect other txs so it has been fixed.

## install DigiByte

```bash
wget https://github.com/digibyte/digibyte/releases/download/v7.17.2/digibyte-7.17.2-x86_64-linux-gnu.tar.gz
tar -xf digibyte-7.17.2-x86_64-linux-gnu.tar.gz
rm digibyte-7.17.2-x86_64-linux-gnu.tar.gz
mkdir .digibyte
nano .digibyte/digibyte.conf
```

```
rpcuser=user
rpcpassword=pass11
rpcbind=127.0.0.1
rpcport=14022
whitelist=127.0.0.1
rpcallowip=127.0.0.1
listen=1
server=1
txindex=1
addnode=191.81.59.115
addnode=175.45.182.173
addnode=45.76.235.153
addnode=24.74.186.115
addnode=24.101.88.154
addnode=8.214.25.169
addnode=47.75.38.245
```

to get digibyte to run on boot do the following

```bash
sudo nano /etc/systemd/system/digibyted.service
```

```
[Unit]
Description=DigiByte's distributed currency daemon
After=network.target

[Service]
User=digibyte
Group=digibyte

Type=forking
PIDFile=/home/digibyte/.digibyte/digibyted.pid
ExecStart=/home/digibyte/digibyte-7.17.2/bin/digibyted -daemon -pid=/home/digibyte/.digibyte/digibyted.pid \
-conf=/home/digibyte/.digibyte/digibyte.conf -datadir=/home/digibyte/.digibyte -disablewallet

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
```


Enable the service on boot

```bash
sudo systemctl enable digibyted.service
```

Start the serivice

```bash
sudo systemctl start digibyted.service
```

## Install Dependencies

```bash
sudo apt update
sudo apt upgrade
sudo apt-get install cmake libcurl4-openssl-dev libjsoncpp-dev golang-go libjsonrpccpp-dev libjsonrpccpp-tools libsqlite3-dev build-essential pkg-config zip unzip
sudo apt install libboost-all-dev
```

## Install VCPKG
```bash
wget -qO vcpkg.tar.gz https://github.com/microsoft/vcpkg/archive/master.tar.gz
sudo mkdir /opt/vcpkg
sudo tar xf vcpkg.tar.gz --strip-components=1 -C /opt/vcpkg
rm vcpkg.tar.gz
sudo /opt/vcpkg/bootstrap-vcpkg.sh
sudo ln -s /opt/vcpkg/vcpkg /usr/local/bin/vcpkg
```

## Install Standard C++ Dependencies
Warning: The following steps build a lot of code and can take a long time to complete
```bash
sudo vcpkg install cryptopp
# sudo cp /opt/vcpkg/packages/cryptopp_x64-linux/lib/libcryptopp.a /usr/bin/
sudo mkdir /usr/local/include/cryptopp870
sudo cp /opt/vcpkg/packages/cryptopp_x64-linux/include/cryptopp/* /usr/local/include/cryptopp870/
sudo vcpkg install sqlite3
```

## Install IPFS Desktop
```bash
wget https://dist.ipfs.tech/kubo/v0.22.0/kubo_v0.22.0_linux-amd64.tar.gz
tar -xvzf kubo_v0.22.0_linux-amd64.tar.gz
cd kubo
sudo bash install.sh
ipfs init
ipfs daemon
```
this step will list out a lot of data of importance is the line that says "RPC API server listening on" it is usually port 5001 note it down if it is not.  You can now see IPFS usage at localhost:5001/webui in your web browser(if not headless)


## Build DigiAsset Core
download code and get in to directory
```bash
mkdir build
cd build
cmake -B . -S .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
mv src/digiasset_core ../../bin
```

---

### Other Notes

- If submitting pull requests please utilize the .clang-format file to keep things standardized.
- example.cfg has a write up of what the different config settings are.
- The first 12066400 blocks database file can be downloaded using ipfs
 ```bash 
ipfs get QmWPQ3msDsWbR4YB9PvePtNcPJtTum8hsQGjYa2GnPxc7F -o bin/chain.db
```


---
# Special Thanks
### Major Financial Support:
RevGenetics [Longevity Supplements](https://www.RevGenetics.com)