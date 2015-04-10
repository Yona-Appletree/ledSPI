#!/usr/bin/env bash

function fail() {
	echo "FAILED $1"
	exit -1
}

DTB_FNAME=$1

if [[ -z $DTB_FNAME ]]; then
	echo "Scanning for DTB file..."
	DTB_FNAME=$(find /boot -name "*boneblack*dtb" | head -n 1)

	if [[ -z $DTB_FNAME ]]; then
		echo "No boneblack DTB file found in /boot"
		exit -1
	else
		echo "Found $DTB_FNAME."
		echo "Continue? (y/n) "
		read response

		if [[ $response != 'y' ]]; then
			echo "Exiting."
			exit -1
		fi
	fi
	
fi

DTS_FNAME=linux-`uname -r`.dts
DTS_PATCHED_FNAME=$(basename $DTS_FNAME .dts).pru.dts
DTB_PATCHED_FNAME=$(basename $DTS_FNAME .dts).pru.dtb

echo "Decompiling DTB..."
dtc -I dtb -O dts -o $DTS_FNAME $DTB_FNAME || fail

echo "Patching DTS..."
START_LN=$(grep -n 'pruss@.*{' $DTS_FNAME | cut -d ':' -f 1)

head -n $START_LN $DTS_FNAME > $DTS_PATCHED_FNAME || fail
echo $START_LN
tail -n +$(expr 1 + $START_LN) $DTS_FNAME | sed -E '0,/status = "disabled"|}/s//status = "okay"/' >> $DTS_PATCHED_FNAME || fail

echo "Compiling DTB..."
dtc -I dts -O dtb -o $DTB_PATCHED_FNAME $DTS_PATCHED_FNAME || fail "Could not compile DTB. Likely cause is that the PRU is already enabled."

echo "Backing up your dtb..."
cp -v $DTB_FNAME{,.pre-pru.bk} || fail

echo "Copying patched DTB in..."
cp -v $DTB_PATCHED_FNAME $DTB_FNAME
