#!/bin/bash

START=0
END=`bitcoind getblockcount`
INTERVAL=2500
#START=$(($END-1))
echo "Total: " $START "-" $END
for (( j = $START ; j <= $END ; j += $INTERVAL ))
do
	echo "[" > djp3Dump_$j.json
	NEW_END=$(( $j + $INTERVAL))
	echo "  Current :" $j "-" $NEW_END
	for (( i = $j ; i < $NEW_END ; i ++))
	do
		./bitcoind getblockbycount $i+0 >> djp3Dump_$j.json
		if [[ "$i" == "$(( $NEW_END - 1 ))" ]]
			then echo "]" >> djp3Dump_$j.json
			else echo "," >> djp3Dump_$j.json
		fi
	done
done
