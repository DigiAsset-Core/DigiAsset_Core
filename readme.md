# DigiAsset Core

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

DigiAsset Core is a DigiAsset processing node for the DigiByte blockchain.  It decodes and
tracks all DigiAsset transactions, serves asset data over JSON-RPC, and (when connected to a
wallet enabled DigiByte Core) can create, send and track DigiAssets.

Building this project produces up to 4 binaries (all placed in `bin/`):

| Binary | What it is |
|---|---|
| `digiasset_core` | The main daemon.  Syncs against DigiByte Core and serves the JSON-RPC api (default port 14024) |
| `digiasset_core-cli` | Command line interface to the daemon.  Any RPC method (including all DigiByte wallet methods) can be called: `./digiasset_core-cli getwalletbalances` |
| `digiasset_core-web` | Documentation web server (default port 8090) |
| `digiasset_core-qt` | Graphical interface (sync status, balances, sending assets, creating assets with optional royalty rules, burning/reissuing, wallet history) |

## Table of Contents
1. [Requirements](#requirements)
2. [Install on Ubuntu](#install-on-ubuntu)
3. [Install on macOS](#install-on-macos)
4. [Install on Windows](#install-on-windows)
5. [Configure DigiAsset Core](#configure-digiasset-core)
6. [Set DigiAsset Core to Run at Boot](#set-digiasset-core-to-run-at-boot)
7. [Upgrading DigiAsset Core](#upgrading-digiasset-core)
8. [Documentation](#documentation)
9. [Event Stream](#event-stream)
10. [Other Notes](#other-notes)
11. [Special Thanks](#special-thanks)

## Requirements

- **DigiByte Core v8.22.2** with `txindex=1`.  Wallet support must be enabled (it is in the
  official release binaries) if you want to create or send assets — the
  `issueasset`/`sendasset`/`getwalletbalances` methods and the wallet RPC passthrough need it.
- **IPFS (kubo)** running on the same machine (asset metadata storage).
- **cmake 3.24+** and a C++14 capable compiler.
- Roughly 100GB of disk space for the DigiByte chain plus the DigiAsset database.

## Install on Ubuntu

Tested on Ubuntu 22.04 LTS.  Ubuntu 20.04 works for the main app but the google tests don't
compile there.

### Increase swap size (low RAM machines only)

DigiByte Core can crash during sync on machines with little RAM.  If your machine has less
than 8GB, increase swap to 8GB:

```bash
sudo swapoff /swap.img
sudo dd if=/dev/zero bs=1M count=8192 oflag=append conv=notrunc of=/swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
sudo swapon --show
sudo nano /etc/fstab
```

place the following at the end (if swap.img is already there replace it)

```
/swapfile       none    swap    sw      0       0
```

### Install DigiByte

```bash
wget https://github.com/DigiByte-Core/digibyte/releases/download/v8.22.2/digibyte-8.22.2-x86_64-linux-gnu.tar.gz
tar -xf digibyte-8.22.2-x86_64-linux-gnu.tar.gz
rm digibyte-8.22.2-x86_64-linux-gnu.tar.gz
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
User=<your-username>
Group=<your-username>

Type=forking
PIDFile=/home/<your-username>/.digibyte/digibyted.pid
ExecStart=/home/<your-username>/digibyte-8.22.2/bin/digibyted -daemon -pid=/home/<your-username>/.digibyte/digibyted.pid \
-conf=/home/<your-username>/.digibyte/digibyte.conf -datadir=/home/<your-username>/.digibyte

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
```

replace <your-username>

Enable and start the service

```bash
sudo systemctl enable digibyted.service
sudo systemctl start digibyted.service
```

### Install dependencies

```bash
sudo apt update
sudo apt upgrade
sudo apt-get install cmake libcurl4-openssl-dev libjsoncpp-dev golang-go libjsonrpccpp-dev libjsonrpccpp-tools libsqlite3-dev build-essential pkg-config zip unzip libssl-dev
sudo apt install libboost-all-dev
```

If you want to build the GUI also install Qt:

```bash
sudo apt-get install qtbase5-dev libqt5charts5-dev
```

### Update cmake

DigiAsset Core needs cmake 3.24 or newer.  Ubuntu 22.04 ships 3.22 so a manual install is
needed:

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-x86_64.sh
chmod +x cmake-3.27.7-linux-x86_64.sh
sudo ./cmake-3.27.7-linux-x86_64.sh --prefix=/usr/local
export PATH=/usr/local/cmake-3.27.7-linux-x86_64/bin:$PATH
nano ~/.bashrc
```

at the end of the file add

```
export PATH=/usr/local/cmake-3.27.7-linux-x86_64/bin:$PATH
```

### Install IPFS

```bash
wget https://dist.ipfs.tech/kubo/v0.22.0/kubo_v0.22.0_linux-amd64.tar.gz
tar -xvzf kubo_v0.22.0_linux-amd64.tar.gz
cd kubo
sudo bash install.sh
ipfs init
ipfs daemon
```

this step will list out a lot of data of importance is the line that says "RPC API server listening on" it is usually
port 5001 note it down if it is not. You can now see IPFS usage at localhost:5001/webui in your web browser(if not
headless).
Press Ctrl+C to stop the daemon

To set IPFS to run on boot:

```bash
cd ~
sudo nano /etc/systemd/system/ipfs.service
```

edit the file to look like this

```
[Unit]
Description=IPFS Daemon
After=network.target

[Service]
ExecStart=/usr/local/bin/ipfs daemon
User=<your-username>
Restart=always

[Install]
WantedBy=multi-user.target
```

replace <your-username>

```bash
sudo systemctl daemon-reload
sudo systemctl enable ipfs.service
sudo systemctl start ipfs.service
```

### Build DigiAsset Core

```bash
git clone -b master --recursive https://github.com/DigiAsset-Core/DigiAsset_Core.git
cd DigiAsset_Core
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build . -j$(nproc)
cd ../bin
```

Notes:
* Binaries are placed in `DigiAsset_Core/bin/` automatically.
* If you don't want the GUI (or don't have Qt installed) add `-DBUILD_QT=OFF` to the first cmake command.
* To also build the test suite add `-DBUILD_TEST=ON`.
* Other options: `-DBUILD_CLI=OFF`, `-DBUILD_WEB=OFF`.

## Install on macOS

Tested on macOS (Intel and Apple Silicon) with [Homebrew](https://brew.sh).

### Install DigiByte

Download and install [digibyte-8.22.2-osx.dmg](https://github.com/DigiByte-Core/digibyte/releases/download/v8.22.2/digibyte-8.22.2-osx.dmg),
then create `~/Library/Application Support/DigiByte/digibyte.conf` with the same settings as
the Ubuntu section above (rpcuser, rpcpassword, rpcport=14022, txindex=1, server=1).

### Install dependencies

```bash
brew install cmake jsoncpp libjson-rpc-cpp openssl@3 curl sqlite boost
```

If you want to build the GUI also install Qt (Qt5 and Qt6 both work):

```bash
brew install qt
```

### Install IPFS

```bash
brew install ipfs
ipfs init
brew services start ipfs
```

### Build DigiAsset Core

```bash
git clone -b master --recursive https://github.com/DigiAsset-Core/DigiAsset_Core.git
cd DigiAsset_Core
git submodule update --init --recursive
mkdir build
cd build
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build . -j$(sysctl -n hw.ncpu)
cd ../bin
```

The same `-DBUILD_QT=OFF`/`-DBUILD_TEST=ON` options as the Ubuntu section apply.

## Install on Windows

The daemon (`digiasset_core.exe`) builds natively on Windows with Visual Studio and
vcpkg.  The CLI, web server, and Qt GUI are not yet built natively — if you need those
on Windows, use [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install) with
Ubuntu 22.04 and follow the [Ubuntu instructions](#install-on-ubuntu) inside WSL.

Prebuilt Windows binaries are published on the GitHub Releases page for tagged versions.

To build natively you need [Visual Studio 2022](https://visualstudio.microsoft.com/)
(with the "Desktop development with C++" workload), [git](https://git-scm.com/), and
[CMake](https://cmake.org/).  Then from a "x64 Native Tools Command Prompt":

```bat
git clone -b master --recursive https://github.com/DigiAsset-Core/DigiAsset_Core.git
cd DigiAsset_Core

rem Install dependencies with vcpkg (uses vcpkg.json manifest)
git clone https://github.com/microsoft/vcpkg.git
vcpkg\bootstrap-vcpkg.bat

rem Configure and build the daemon
cmake -B build -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_TEST=OFF -DBUILD_CLI=OFF -DBUILD_WEB=OFF -DBUILD_QT=OFF
cmake --build build --config Release
```

The binary is written to `build\Release\digiasset_core.exe`.  You will also need a
DigiByte Core node (v8.22.2) and an [IPFS (kubo)](https://docs.ipfs.tech/install/) node
running, the same as on other platforms.

## Configure DigiAsset Core

The first time you run DigiAsset Core it will ask you several questions to set up your config file.  Run DigiAsset Core using

```bash
./digiasset_core
```

This will create bin/config.cfg the wizard creates only the basic config for a full list of config options see example.cfg

If DigiByte Core has **more than one wallet loaded** it cannot tell which one you mean, and every
wallet command(issuing, sending, the PSP payout address) fails.  Name the one to use in config.cfg:

```
rpcwallet=<wallet name>
```

This is the same option name `digibyte-cli` uses, and it is only needed for multi wallet nodes -
with a single wallet loaded DigiAsset Core finds it on its own.

Make sure DigiAsset Core is running correctly and then press ctrl+c to stop it and continue with instructions.

---

## Set DigiAsset Core to run at boot

(Linux only)

```bash
sudo nano /etc/systemd/system/digiasset_core.service
```

```
[Unit]
Description=DigiAsset Core
After=network.target digibyted.service

[Service]
User=<your-username>
Group=<your-username>

Type=simple
ExecStart=/home/<your-username>/DigiAsset_Core/bin/digiasset_core
WorkingDirectory=/home/<your-username>/DigiAsset_Core/bin

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
```

replace <your-username>

Enable and start the service

```bash
sudo systemctl enable digiasset_core.service
sudo systemctl start digiasset_core.service
```

## Upgrading DigiAsset Core

When a new version is available you can upgrade by running the following commands

```bash
cd DigiAsset_Core/bin
./digiasset_core-cli shutdown
sudo systemctl stop digiasset_core.service
cd ..
git pull
git submodule update --init --recursive
cd build
cmake ..
cmake --build . -j$(nproc)
cd ../bin
sudo systemctl start digiasset_core.service
```

---

## Documentation

To access documentation run the digiasset_core-web application then go to http://127.0.0.1:8090/

Highlights:
- Every RPC method has its own documentation page, including the DigiAsset specific
  methods (`issueasset`, `reissueasset`, `sendasset`, `sendmanyassets`, `burnasset`,
  `getwalletbalances`, `getassetdata`, `listassets`, ...).
- `issueasset` supports the full DigiAsset v3 rule set (royalties, KYC, geofencing,
  voting, expiry, deflation, signer approval) via its `rules` parameter.
- `issueasset`, `reissueasset`, `sendasset` and `burnasset` all take a `dryrun` option
  that builds the transaction and returns a full cost breakdown (outputs, storage pool
  fee, estimated miner fee) without broadcasting anything.

For how the rules are enforced and every situation in which assets are destroyed, see
[docs/asset-rules-and-burns.md](docs/asset-rules-and-burns.md).  Worth reading before
touching the transfer path: a rule violation burns every asset in the transaction, the
sender's change included.
- Any method the daemon doesn't recognize is transparently forwarded to the DigiByte Core
  wallet, so the standard DigiByte/Bitcoin RPC api is available through the same port too.

## Event Stream

The daemon pushes events to any TCP client as newline delimited JSON (default port
14025, config keys `eventport`/`eventbind`, see `example.cfg`).  Try it with
`nc 127.0.0.1 14025`:

```
{"event":"newBlock","height":23843354,"blocksBehind":1}
{"event":"assetTransfer","assetIds":["La6xk..."],"txid":"d44d...","height":23843354}
{"event":"balanceChanged","addresses":["dgb1q..."],"txid":"d44d...","height":23843354}
```

Event types: `newBlock` (near chain tip only), `assetIssued`, `assetTransfer`,
`assetBurn` and `balanceChanged`.  Writes never block the daemon — clients that fall
behind are disconnected, so treat the stream as a wake-up signal and re-query the RPC
api for authoritative state.  The stream has no authentication and therefore only
listens on localhost unless you explicitly set `eventbind`.

The daemon shuts down cleanly on `ctrl+c`, SIGTERM or the `shutdown` RPC method
(database flushed and closed before exit).

## Other Notes

- If submitting pull requests please utilize the .clang-format file to keep things standardized.
- Run the test suite with `cmake .. -DBUILD_TEST=ON`, build target `Google_Tests_run`, then
  run `./Google_Tests_run` from the `bin/` folder.  See `tests/` for details — some tests
  need a running IPFS node and a reachable DigiByte Core.

---

# Special Thanks

### Major Financial Support:

RevGenetics [Longevity Supplements](https://www.RevGenetics.com)
