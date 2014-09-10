#!/usr/bin/env bash

cd $(dirname $0)

TEMPLATE_DIR=templates

for TEMPLATE in $(cd "$TEMPLATE_DIR"; ls *.p|sed s/.p$//); do
	./build_template.sh $TEMPLATE
done