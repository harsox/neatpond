#!/bin/bash
cd ./src &&
g++ -L/usr/local/lib -I/usr/local/include -std=c++14 -lSDL2 -lSDL2_image main.cc -o ../evolve &&
cd ../ && ./evolve
