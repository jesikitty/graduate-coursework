#! /bin/bash

split "-a 3" "-n 16" "-d" "/mnt/my-data/wiki10gb" "/mnt/my-data/data";
swift wdcount.swift;
python merge.py;

