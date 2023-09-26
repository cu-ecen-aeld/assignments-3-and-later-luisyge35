#!/bin/sh

case "$1" in
  start)
  echo "Starting server"
  start-stop-daemon -S -n server -a /usr/bin/aesdsocket -d
  ;;
  stop)
  echo "Stopping server"
  start-stop-daemon -K -n server
  ;;
  *)
  exit -1
  ;;
esac