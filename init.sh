#!/usr/bin/bash
wget "https://github.com/dropbox/dbxcli/releases/download/v3.0.0/dbxcli-linux-amd64"
mv dbxcli-linux-amd64 dbxcli
chmod +x dbxcli
mv dbxcli /usr/local/bin
dbxcli account
sudo apt update
sudo apt install fuse libfuse3-dev
sudo apt update
sudo apt install fuse3


