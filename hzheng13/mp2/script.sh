#! /bin/bash

clear

echo "Now starts running nodes"



./vec_router 3 testinitcosts3 logfile3 &
./vec_router 0 testinitcosts0 logfile0 &
./vec_router 200 testinitcosts200 logfile200 &
./vec_router 5 testinitcosts5 logfile5 &
./vec_router 2 testinitcosts2 logfile2 &
./vec_router 1 testinitcosts1 logfile1 &
./vec_router 4 testinitcosts4 logfile4 &
./vec_router 6 testinitcosts6 logfile6 &
./vec_router 7 testinitcosts7 logfile7 &
