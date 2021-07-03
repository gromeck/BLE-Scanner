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

URL="https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/"
FILE="CompanyIdentfiers - CSV.csv"
TARGET="../../BLE-Scanner/ble-manufacturer-list.h"

cat <<EOM
As the offical file with all assigned manufacurer IDs is prevented
to be automatically downloaded, this has to be done manually. So,
please download the CSV file from

  $URL

and store the CVS as

  $FILE

Hit return when done, otherwise cancel with <Ctrl-C>
EOM
read

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
ITEMS=$( cat "$FILE" | \
	awk 'BEGIN {
		  FS = ",";
		}
		/^"[0-9]+","0[xX][0-9a-fA-F]+",".*"/ {
			# Hexadecimal ($2) and Company ($3)
			# print $2,$3;
			hex = $2;
			gsub(/"/, "", hex);
			vendor = substr($3,1,24);
			gsub(/"/, "", vendor);
			printf "\t{ %s, \"%s\" },\n", hex, vendor;
		}' | \
	sort | tee --append $TARGET | wc -l )

echo "Written $ITEMS entries to $TARGET -- recompile the scetch."
