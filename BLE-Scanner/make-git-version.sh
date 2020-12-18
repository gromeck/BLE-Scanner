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

# logger $0: $*

# get the VERSION/tag from git
VERSION=$( cd "$1" ; git describe --tags --always --dirty ) 2>/dev/null

# fallback to the current date
[ -z "$VERSION" ] && VERSION=$( date -I )

# Save this in git-version.h
echo "GIT version: $VERSION"
echo "#define GIT_VERSION \"$VERSION\"" >$2/sketch/git-version.h
