#!/bin/bash

g++ -O3 stats.cpp `pkg-config --libs --cflags opencv` -lpthread -o stats
