input=

test:
	cmake --build build --target Tests && build/bin/Tests ${input}

doc:
	cmake --build build --target Doc

benchmark:
	cmake --build build --target Benchmarks && build/bin/Benchmarks ${input}

calculator:
	cmake --build build --target Calculator && cd examples/calculator && ../../build/bin/Calculator ${input}

graphs:
	cmake --build build --target Graphs && cd examples/graphs && ../../build/bin/Graphs ${input}

json:
	cmake --build build --target Json && cd examples/json && ../../build/bin/Json ${input}

kaleidoscope:
	cmake --build build --target Kaleidoscope && cd examples/kaleidoscope && ../../build/bin/Kaleidoscope ${input}


clear:
	cmake --build build --target clean

init:
	rm -rf build/ && cmake -Bbuild .

.PHONY : test init clear