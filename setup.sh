#! /bin/bash


case `uname -s` in
Linux)
  sudo apt install libgoogle-glog-dev libgflags-dev libsfml-dev
  sudo modprobe joydev
  ;;
Darwin)
  brew install glog gflags sfml
  ;;
Unix)
  echo "Not support in Unix now" >&2
  ;;
*)
  echo "Unknown system" >&2
  ;;
esac
