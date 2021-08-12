test:
	cmake --build build --target Tests && build/bin/Tests

calculator:
	cmake --build build --target Calculator && build/bin/Calculator

init: 
	cmake -Bbuild .

clear:
	rm -rf build/

.PHONY : test init clear