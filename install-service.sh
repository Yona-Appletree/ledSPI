#!/usr/bin/env bash

cd $(dirname $0)

if [[ -f ledspi.service ]]; then
	echo "Enabling Service..."
	systemctl enable $(pwd)/ledspi.service || exit -1

	echo "Starting Service..."
	systemctl start ledspi.service
	systemctl status ledspi.service
else
	echo "Could not find ledspi.service. Please run make first"
	exit -1
fi