#!/bin/sh

logpath_src=/mnt/app/log
logpath_dst=/mnt/extsd/log
staging=${logpath_src}/.staging

function log_backup()
{
        src=$1
        dst=$2
        let index=-1

        if [ ! -d "${dst}" ]
        then
		echo "mkdir ${dst}"
                mkdir ${dst}
        fi

		#get the max index value
        for file in ${dst}/*.log.*
        do
                if [ -f "$file" ]
                then
                        #only pure-decimal suffixes are backup indices; a
                        #non-numeric suffix would abort ash arithmetic (.2x)
                        #or evaluate as 0/subtraction and corrupt the index
                        sfx=${file##*.}
                        case "${sfx}" in
                                ''|*[!0-9]*) continue ;;
                        esac

                        if [ "${sfx}" -gt "${index}" ]
                        then
                                index=${sfx}
                        fi
                fi
        done

		let index+=1
        echo "index_next = ${index}"

        for file in ${src}/*.log
        do
                if [ -f "$file" ]
                then
                        name=${file##*/}
                        cp ${file} ${dst}/${name}.${index}
                        echo "cp ${file} ${dst}/${name}.${index}"
                        rm -f ${file}
                        let index+=1
                fi
        done

        #retention: drop backups older than the newest ~50 indices so the
        #directory (and the index scan above) stays bounded over device life
        let prune_below=index-50
        if [ "${prune_below}" -gt 0 ]
        then
                for file in ${dst}/*.log.*
                do
                        if [ -f "$file" ]
                        then
                                #same numeric guard: never delete user files
                                #that merely match *.log.* (e.g. app.log.bak)
                                sfx=${file##*.}
                                case "${sfx}" in
                                        ''|*[!0-9]*) continue ;;
                                esac
                                if [ "${sfx}" -lt "${prune_below}" ]
                                then
                                        rm -f "$file"
                                fi
                        fi
                done
        fi
}

#snapshot the previous boot's logs first: mv within the same filesystem is
#atomic and instant, so the slow SD-card copy below can never race the
#just-started record/app processes writing fresh logs into /mnt/app/log.
#Staged files accumulate (and survive) across boots without an SD card.
mkdir -p ${staging}
mv ${logpath_src}/*.log ${staging}/ 2> /dev/null

#if grep -qs '/mnt/extsd' /proc/mounts; then
if df | grep -qs '/mnt/extsd'; then
    echo "sdcard mounted."
else
    echo "sdcard not mounted."
    exit
fi

#copy staged logs to /mnt/extsd/log (log_backup removes each after copy)
log_backup ${staging} ${logpath_dst}
