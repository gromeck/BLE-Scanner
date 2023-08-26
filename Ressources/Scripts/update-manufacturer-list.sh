#!/bin/bash
#
#	BLE-Scanner
#
#	(c) 2020 Christian.Lorenz@gromeck.de
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
#	if the script doesn't work anymore, visit 
#		https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
#	and follow the instruction to find the right URL.
#
#	the output format is:
#	{ 0xXXXX, "vendor name" },
#	:
#
#	NOTE: only the first 2 octets from the MAC address will be used -- makes matching easier.
#	NOTE: the vendor name is limited to 32 characters in the output
#

URL="https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/company_identifiers/company_identifiers.yaml"
TARGET="../../BLE-Scanner/ble-manufacturer-list.h"

#
#	write a header to the file
#
cat >$TARGET <<EOM
/*
	updated with $0
	
	$( date )
*/
EOM

#
#	get the manufacturer db into the target file
#
echo "Getting manufacture IDs from $URL ..."
ITEMS=$( wget -q -O- "$URL" | \
	sed -e "s/company_identifiers://g" | \
	sed -e "s/^[ \t-]*//g" | \
	sed -e "s/'//g" | \
	awk -v RS="value:|name:" \
		'{
			gsub("value:|name:","",$0);
			hex = $1;
			vendor = $2;
			gsub(/0x/,"0000",hex);
			gsub(/"/,"",vendor);
			hex = "0x"tolower(substr(hex,length(hex) - 3));
			printf "\t{ %s, \"%s\" },\n", hex, vendor;
		}' \
		RS='' | \
	sort | tee --append $TARGET | wc -l )

echo "Written $ITEMS entries to $TARGET -- recompile the sketch."
