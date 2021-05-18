#!/bin/bash

KEY_DIR="/tmp/asuswrt_keys"
SOURCE_DIR="release/src/router/bwdpi_source/prebuild"
OUTPUT_FILE="keys.tgz"

rm -rf "${KEY_DIR}"
mkdir "${KEY_DIR}"

cp ${SOURCE_DIR}/*.enc ${KEY_DIR}
cp ${SOURCE_DIR}/*.cert ${KEY_DIR}
cp ${SOURCE_DIR}/*.pem ${KEY_DIR}

echo "* Creating tarball"
rm -rf ${OUTPUT_FILE}
cd ${KEY_DIR}
tar cvzf ${OUTPUT_FILE} *

echo "* tarball content"
tar -tvf ${OUTPUT_FILE}
cd -

echo "* Base64 of tarball is availabe: ${OUTPUT_FILE}.b64"
cat ${KEY_DIR}/${OUTPUT_FILE} | base64 -w 0  > ${OUTPUT_FILE}.b64

