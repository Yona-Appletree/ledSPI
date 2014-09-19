#!/usr/bin/env bash

cd $(dirname $0)

if [[ -f ledscape.service ]]; then
	echo "Enabling Service..."
	systemctl enable $(pwd)/ledscape.service || exit -1

	echo "Starting Service..."
	systemctl start ledscape.service
	systemctl status ledscape.service
else
	echo "Could not find ledscape.service. Please run make first"
	exit -1
fi