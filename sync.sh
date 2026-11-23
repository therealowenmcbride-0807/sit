#!/usr/bin/bash
SOURCE_DIR="~/src/sit"
BUILD_DIR="~/build/sit"
export PATH="$PATH:${BUILD_DIR}"
echo "Syncing..."
cmake -S ${SOURCE_DIR} -B ${BUILD_DIR}
echo "Building..."
cmake --build ${BUILD_DIR}
while true; do
  read -rp "Do you want to test? (y/n): " yn
  case $yn in
  [Yy]*)
    sit
    break
    ;;
  [Nn]*)
    echo "Build completed, test manually."
    exit
    ;;
  *) echo "Please answer yes or no." ;;
  esac
done
clear
