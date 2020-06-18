#!/bin/bash
rm -r pic;
rm output.txt;
./server &> output.txt
mkdir pic;
mv *_received.jpg pic/;
