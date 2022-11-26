#!/usr/bin/env bash

rootDir=$(dirname $(readlink -f $0))

dotnet script "$rootDir/SourceFileFormatter.csx" --filter **/*.h --filter **/*.cpp "$rootDir/../src" "$rootDir/../utils"
