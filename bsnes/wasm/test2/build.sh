#tinygo build -gc=none -wasm-abi=generic -target=wasm -no-debug -opt=s -o main.wasm test2.go
tinygo build -gc=none -wasm-abi=generic -target=wasm -scheduler=none -panic=trap -o main.wasm test2.go