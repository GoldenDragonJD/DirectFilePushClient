#!/bin/bash

flatpak-builder build-dir com.GoldenDragonJD.DirectFilePushClient.yml --force-clean
flatpak-builder --repo=repo --force-clean build-dir com.GoldenDragonJD.DirectFilePushClient.yml
flatpak build-bundle \
  repo \
  DirectFilePushClient.flatpak \
  com.GoldenDragonJD.DirectFilePushClient \
  --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
rm -rf build-dir repo .flatpak-builder/
