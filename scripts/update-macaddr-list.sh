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
#	script to download the MAC vendors database from macaddress.io
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

URL="https://macaddress.io/database/macaddress.io-db.csv"
TARGET="../BLE-Scanner/macaddr-list.h"

wget -q -O- $URL | \
	awk '
		BEGIN {
		  FS = ",";
		}
		/^..:..:..,/ {
			# mac address ($1) and vendor ($3)
			# print $1,$3;
			split($1,mac,":"); 
			vendor = substr($3,1,32);
			gsub(/"/, "", vendor);
			printf "\t{ { 0x%s, 0x%s, 0x%s }, \"%s\" },\n", mac[1], mac[2], mac[3], vendor;
		}
		END {
		}' | \
	sort >$TARGET
