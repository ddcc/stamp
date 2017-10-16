#!/bin/bash

for i in *.log; do
    echo $i
    grep "abort: " $i | sort | uniq -c | sort -n > $(basename $i).blame
done
