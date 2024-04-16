#!/bin/bash

patch_dir=rb_patches

if [ ! -d ${patch_dir} ] || [ -z "$(ls -A $patch_dir)" ]; then
	echo nothing to patch
	exit 
fi

changelists=`ls ${patch_dir} | sort -n -t b -k 2 `

for ch in $changelists
do
	echo "applying patch file...${ch}"
	patch -f -p0 < ${patch_dir}/${ch} >& patch${ch}.out
	cat patch${ch}.out >> patch.out
	rm -f patch${ch}.out
done

echo "completed!"
