#!/bin/bash
gcc -Wall -shared -fPIC -lreadline -o luaclie_c.so src/luaclie.c
