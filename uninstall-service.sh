#!/usr/bin/env bash

cd $(dirname $0)

if [[ -f ledspi.service ]]; then
	echo "Stopping Service..."
	systemctl stop ledspi.service

	echo "Disabling Service..."
	systemctl disable $(pwd)/ledspi.service || exit -1
else
	echo "Could not find ledspi.service. Please run make first"
	exit -1
fi