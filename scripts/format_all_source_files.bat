@echo off

set rootDir=%~dp0

dotnet script "%rootDir%SourceFileFormatter.csx" --filter **/*.h --filter **/*.cpp "%rootDir%../src" "%rootDir%../utils"
