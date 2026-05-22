all:
	cmake -B build -S .
	cmake --build build

run:
	./build/CommandsApp