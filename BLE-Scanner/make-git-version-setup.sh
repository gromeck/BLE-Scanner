#!/bin/bash
#
#	this script generates ~/sketch/git-version.h file in the
#	Arduino build environment (not in the source tree)
#
#	To configure the automatic update of git-version.h add
#	the following line
#
#	recipe.hooks.sketch.prebuild.1.pattern={build.source.path}/make-git-version.sh "{build.source.path}" "{build.path}"
#
#	to <Arduino Install Directory>/arduino/hardware/arduino/avr/platform.local.txt
#
#	or to the local configuration
#
#	~/.arduino*/packages/<HW type>/hardware/<HW type>/<version>/platform.local.txt
#
#	Thanks to https://arduino.stackexchange.com/a/51488
#

ARDUINO_PLATTFORM_LOCAL=~/Arduino/arduino/hardware/arduino/avr/platform.local.txt

echo "Setting up $ARDUINO_PLATTFORM_LOCAL"
cat >$ARDUINO_PLATTFORM_LOCAL <<EOM
recipe.hooks.sketch.prebuild.1.pattern={build.source.path}/make-git-version.sh "{build.source.path}" "{build.path}"
EOM
cat $ARDUINO_PLATTFORM_LOCAL

find ~/.arduino*/packages/*/hardware/*/*/ -maxdepth 0 -type d | while read HWDIR; do
	echo "Symlinking $ARDUINO_PLATTFORM_LOCAL to $HWDIR"
	ln -s $ARDUINO_PLATTFORM_LOCAL $HWDIR
done
