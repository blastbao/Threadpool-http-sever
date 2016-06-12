Author : Yoad Shiran,


A Simple Webserver Written in C
===============================

This is a simple webserver written in the C programming language.

It uses a threadpoolpool to serve multiple requests concurrently and it keeps track of how much data it has output, printing it to the standard output stream.


Compiling and Using the System
==============================

On a Linux system system simply use the terminal "gcc server.c threadpool.c -lpthread -o server"

