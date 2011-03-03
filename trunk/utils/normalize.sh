#!/bin/bash
# Noramlize folders, that is, folders that look similar are merged into one

DST=$1

ORIGINAL_IFS=$IFS
IFS=$'\n'

function normalize()
{
	total=`ls $1 2>/dev/null | wc -l`
	current=0
	echo "$total files"
	for a in `ls $1`; do
		let "current=$current +1"
		let "pos=($current*100)/$total"
		against=0
		working=0
		dups_count=0
		for b in `ls $1`; do
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
			echo -ne "\r$ch $current $against ($total) $pos%"
			if [ $a == $b ]; then
				continue
			fi
			if echo $a | grep -i "^${b}$" > /dev/null ; then
				let "dups_count=$dups_count+1"
				dups[$dups_count]=$b
			fi
		done
		# now show the options
		if [ ! $dups_count -eq 0 ]; then
			echo -e "\nWe have found the following similar names for '$a':"
			#dups_count=`expr $dups_count - 1`
			echo "[0] $a"
			for d in `seq 1 $dups_count`; do
				echo "[$d] ${dups[$d]}"
			done
			# select the option
			echo "Select the one to choose"
			read option
			case $option in
				[1-${dups_count}])
				# modify all the dups AND the current file
				mv $1/Artist/$a/Files/* $1/Artist/${dups[$option]}/Files/
				;;
				*);;
			esac
			# TODO add the modified values on a list to skip
		fi
	done
}

normalize $DST/Artist
#normalize $DST/Album
#normalize $DST/Title
