# DirectFilePushClient

## Tutorial

https://github.com/user-attachments/assets/d68224d4-7e96-4ed9-aa26-581911ebb2bd

## About

A Lightweight client application for the server [DirectFilePushServer](https://github.com/GoldenDragonJD/DirectFilePushServer). The client is responsible for allowing clients to send messages, files, and even folders to the other client without a need to open any ports or worry about exposing themselves to a public connection. The client includes a very effective encryption method for end to end encryption that allows the user to encrypt messages, file names, folder names, as well as file data to send to another client at the cost of speed. With my personal tests the speed on local host went from 1.8GB/s to around 60MB/s. Though it will prevent anyone but your target client from knowing what you are sending other the internet.

I host a server on my raspberry pi, for LAN transfers, and a server on a linode instance for public transfers. I have successfully transfered a file from the U.S to a friend in mainland china at a speed of 5MB/s. I will in the future open a discussion page to be able to share what speeds you got yourself, I felt like I was too limited by my home internet of 40Mb/s upload speed. I have also compiled a flatpak for easy installs to the steamdeck, I have successfully transfered the entire Red Dead Redemption 2 game folder to the steam deck and it ran perfectly, after also sending the proton prefix.

# Key Features
- Messaging
- Sending Files
- Sending Folders
- Lightwright Resource Usage
- End to End Encryption on:
  - Messages
  - File names
  - File Data
  - Folder names

## Want to compile it yourself?
### Prepare packages needed
- cmake
- g++ or a different c++ compiler
- qt6 sdk

## Distros
### Arch / Cachyos
```bash
sudo pacman -S git base-devel qt6-base cmake vulkan-headers
git clone https://github.com/GoldenDragonJD/DirectFilePushClient.git
cd DirectFilePushClient
cmake .
cmake --build
```

### Debian / Ubuntu / Linux Mint
```bash
sudo apt update
sudo apt install git build-essential cmake qt6-base-dev libvulkan-dev
git clone https://github.com/GoldenDragonJD/DirectFilePushClient.git
cd DirectFilePushClient
cmake .
cmake --build .
```

### Fedora / RHEL / CentOS
```bash
sudo dnf install git gcc-c++ make cmake qt6-qtbase-devel vulkan-headers
git clone https://github.com/GoldenDragonJD/DirectFilePushClient.git
cd DirectFilePushClient
cmake .
cmake --build .
```

### openSUSE (Tumbleweed or Leap)
```bash
sudo zypper install git-core devel_basis cmake qt6-base-devel vulkan-devel
git clone https://github.com/GoldenDragonJD/DirectFilePushClient.git
cd DirectFilePushClient
cmake .
cmake --build .
```

## Flatpak
1. Download flatpak file from [Releases](https://github.com/GoldenDragonJD/DirectFilePushClient/releases)
2. Locate DireFilePushClient.flatpak file
3. run ```flatpak install --user ./DirectFilePushClient.flatpak```

# Windows
Please download the .exe file from the latest [Releases](https://github.com/GoldenDragonJD/DirectFilePushClient/releases) page, it comes with all the things compiled to run the app on just about any system. If you want to compile it, I wish you luck because I don't really have the time to go through all those steps again, I have the system already set up to use wine.
