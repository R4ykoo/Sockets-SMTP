#!/bin/bash
./servidor
./cliente localhost TCP ./ordenes/ordenes.txt &
./cliente localhost TCP ./ordenes/ordenes1.txt &
./cliente localhost TCP ./ordenes/ordenes2.txt &
./cliente localhost UDP ./ordenes/ordenes.txt &
./cliente localhost UDP ./ordenes/ordenes1.txt &
./cliente localhost UDP ./ordenes/ordenes2.txt &
