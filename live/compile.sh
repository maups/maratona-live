#!/bin/bash

g++ -O3 main.cpp boca.cpp `pkg-config --libs --cflags opencv` -lpthread -o live
