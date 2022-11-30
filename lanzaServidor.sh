#!/bin/bash
./servidor
./cliente nogal TCP ./ordenes/ordenes.txt &
./cliente nogal TCP ./ordenes/ordenes1.txt &
./cliente nogal TCP ./ordenes/ordenes2.txt &
./cliente nogal UDP ./ordenes/ordenes.txt &
./cliente nogal UDP ./ordenes/ordenes1.txt &
./cliente nogal UDP ./ordenes/ordenes2.txt &
