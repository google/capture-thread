#!/bin/bash

cd "$(dirname "$0")" || exit

date=${1-now}

filename="$(basename $(pwd))-$(date +%Y%m%d -d "$date").tar.gz"

git ls-files | tar -cvvzf "$filename" -T -
