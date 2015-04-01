#! /bin/bash

clear

echo "Now starts running nodes"

./vec_router 0 testinitcosts0 logfile0 &
./vec_router 1 testinitcosts1 logfile1 &
./vec_router 2 testinitcosts2 logfile2 &

./vec_router 200 testinitcosts200 logfile200 &




