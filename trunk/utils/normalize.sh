#!/bin/sh
# Noramlize folders, that is, folders that look similar are merged into one

DST=$1
ORIGINAL_IFS=$IFS
IFS=$'\n'

total=`ls -l $DST/Artist 2>/dev/null | wc -l`
current=0
echo "$total files"
for a in `ls $DST/Artist`; do
	let "current=$current +1"
	let "pos=(current*100)/total"
	against=0
	working=0
	for b in `ls $DST/Artist`; do
		ch="-"
		# - \ | /
		case $working in
			0) ch="-";;
			1) ch="\\";;
			2) ch="|";;
			3) ch="/";;
		esac
		let "against=$against +1"
		let "working=(working+1)%4"
		echo -ne "\r$current $against ($total) $pos% $ch"
		if [ $a == $b ]; then
			continue
		fi
		#stra=`echo $a | tr -s '[:upper:]' '[:lower:]'`
		#strb=`echo $b | tr -s '[:upper:]' '[:lower:]'`
		#if [ $stra == $strb ]; then
		if echo $a | grep -i "^${b}$" > /dev/null ; then
			echo 
			echo "Are equal '$a' '$b'"
		fi
	done
done

# test $(echo "string" | /bin/tr -s '[:upper:]' '[:lower:]') = $(echo "String" | /bin/tr -s '[:upper:]' '[:lower:]') && echo same || echo different

#var1=match 
#var2=MATCH 
#if echo $var1 | grep -i "^${var2}$" > /dev/null ; then
#  echo "MATCH"
#fi
