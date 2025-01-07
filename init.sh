#!/usr/bin/bash
sudo apt install git
sudo git clone https://github.com/ediardi/DropboxFS
cd DropboxFS
sudo wget "https://github.com/dropbox/dbxcli/releases/download/v3.0.0/dbxcli-linux-amd64"
sudo mv dbxcli-linux-amd64 dbxcli
sudo chmod +x dbxcli
sudo mv dbxcli /usr/local/bin
sudo dbxcli account
sudo apt update
sudo apt install libfuse-dev
sudo apt update
sudo apt install fuse
sudo apt-get install pkg-config
sudo apt install gcc
sudo gcc main.c -o main $(pkg-config fuse --cflags --libs)
read -p "Enter mount path: " mntpath
echo $mntpath
sudo echo RUNNING!
sudo echo open the given path in a new terminal!
sudo echo "dont forget to unmount when finished using $ fusermount -u [specified path]"
sudo ./main -f $mntpath
