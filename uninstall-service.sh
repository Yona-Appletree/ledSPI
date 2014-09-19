#!/usr/bin/env bash

cd $(dirname $0)

if [[ -f ledscape.service ]]; then
	echo "Stopping Service..."
	systemctl stop ledscape.service

	echo "Disabling Service..."
	systemctl disable $(pwd)/ledscape.service || exit -1
else
	echo "Could not find ledscape.service. Please run make first"
	exit -1
fi