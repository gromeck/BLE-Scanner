#!/bin/bash
#
#	script to download the MAC vendors database from macaddress.io
#
#	the format is:
#	oui,isPrivate,companyName,companyAddress,countryCode,assignmentBlockSize,dateCreated,dateUpdated
#
#

URL="https://macaddress.io/database/macaddress.io-db.csv"
TARGET="../BLE-Scanner/macaddr-list.h"

wget -q -O- $URL | \
	awk '
		BEGIN {
		  FS = ",";
		}
		/^..:..:..,0,/ {
			# mac address ($1) and vendor ($3)
			# print $1,$3;
			split($1,mac,":"); 
			vendor = substr($3,1,32);
			gsub(/"/, "", vendor);
			printf "\t{ \"\\x%s\\x%s\\x%s\", \"%s\" },\n", mac[1], mac[2], mac[3], vendor;
		}
		END {
		}' | \
	sort >$TARGET
