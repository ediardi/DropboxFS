#!/usr/bin/bash
sudo wget "https://github.com/dropbox/dbxcli/releases/download/v3.0.0/dbxcli-linux-amd64"
sudo mv dbxcli-linux-amd64 dbxcli
sudo chmod +x dbxcli
sudo mv dbxcli /usr/local/bin
sudo dbxcli account
sudo apt update
sudo apt install libfuse3-dev
sudo apt install libfuse-dev
sudo apt update
sudo apt install fuse3
sudo apt install fuse
sudo apt-get install pkg-config


