#!/bin/bash
#
#	BLE-Scanner
#
#	(c) 2020 Christian.Lorenz@gromeck.de
#
#	module to handle the MQTT stuff
#
#
#	This file is part of BLE-Scanner.
#
#	BLE-Scanner is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	BLE-Scanner is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with BLE-Scanner.  If not, see <https://www.gnu.org/licenses/>.
#
#
#
#	script to update the manufacturer list from bluetooth.com
#
#	the input format from macaddress.io is:
#	oui,isPrivate,companyName,companyAddress,countryCode,assignmentBlockSize,dateCreated,dateUpdated
#
#	the output format is:
#	{ { 0xXX, 0xXX, 0xXX }, "vendor name" }
#
#	NOTE: only the first 3 octets from the MAC address will be used -- makes matching easier.
#	NOTE: the vendor name is limited to 32 characters in the output
#

CHARACTERISTICS="
appearance,https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.gap.appearance.xml
"

for CHARACTERISTIC in $CHARACTERISTICS; do
	URL=$( echo $CHARACTERISTIC | cut -f2 -d, )
	CHARACTERISTIC=$( echo $CHARACTERISTIC | cut -f1 -d, )
	XSLT="${CHARACTERISTIC}2list.xslt"
	LIST="../BLE-Scanner/characteristic-$CHARACTERISTIC.h"

	echo "Transforming $URL to $LIST with $XSLT ..."

	#
	#	write a header to the file
	#
	cat >$LIST <<EOM
/*
	updated with $0
	
	$( date )
*/
EOM

	#
	#	do the transformation
	#
	wget -q -O- $URL | xmlstarlet tr $XSLT >>$LIST
done
