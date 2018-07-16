#!/bin/bash
while true ; do
  read
  clear
  make BOARD=BADGE flash_softdevice && \
  make BOARD=BADGE flash && \
  echo 'Flashed successfully!'
done
