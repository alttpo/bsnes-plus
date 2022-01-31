set -e

#tinygo build -gc=none -wasm-abi=generic -target=wasm -no-debug -opt=s -o main.wasm test2.go
tinygo build -gc=none -wasm-abi=generic -target=wasm -scheduler=none -panic=trap -o main.wasm test2.go
/c/Program\ Files/7-Zip/7z.exe u main.zip main.wasm kakwafont-12-n.pcf
