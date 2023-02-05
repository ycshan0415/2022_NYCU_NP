#!/bin/bash
sudo mount -t efs -o tls fs-0968137b9cdd48c14:/ efs

make

mkdir build

mv server build