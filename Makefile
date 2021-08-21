input=

test:
	cmake --build build --target Tests && build/bin/Tests ${input}

calculator:
	cmake --build build --target Calculator && cd examples/calculator && ../../build/bin/Calculator ${input}

json:
	cmake --build build --target Json && cd examples/json && ../../build/bin/Json ${input}

kaleidoscope:
	cmake --build build --target Kaleidoscope && cd examples/kaleidoscope && ../../build/bin/Kaleidoscope ${input}


clear:
	rm -rf build/

init: clear
	cmake -Bbuild .

.PHONY : test init clear