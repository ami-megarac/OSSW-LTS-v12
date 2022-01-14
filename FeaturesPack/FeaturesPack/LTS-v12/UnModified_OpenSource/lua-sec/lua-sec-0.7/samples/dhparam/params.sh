#!/bin/sh

openssl dhparam -2 -out dh-512.pem  -outform PEM 512
openssl dhparam -2 -out dh-2048.pem -outform PEM 2048
