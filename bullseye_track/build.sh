#!/bin/bash
clang++ ./main.cpp $(pkg-config --cflags --libs opencv4) -o main
