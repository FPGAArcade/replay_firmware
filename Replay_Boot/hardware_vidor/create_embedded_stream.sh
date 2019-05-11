#!/bin/bash

# only tested with WSL ;)
cmd.exe /c C:/intelFPGA_lite/18.1/quartus/bin64/quartus_cpf.exe -c loader.sof loader.rbf
cmd.exe /c C:/intelFPGA_lite/18.1/quartus/bin64/quartus_cpf.exe -c loader.sof loader.ttf
go run make_composite_binary.go -i loader.ttf:1:512 -o loader.h > signature.h
