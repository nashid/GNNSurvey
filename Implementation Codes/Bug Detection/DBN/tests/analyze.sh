#!/usr/bin/env bash

TOKENPROFILER=../bin/bin/tokenprofiler

llfiles=""

for file in ./ll/**/*.ll
do
    llfiles="$llfiles $file"
done
$TOKENPROFILER $llfiles
mv token_vector.csv ./csv/token_vector.csv
mv token_encoding.csv ./csv/token_encoding.csv