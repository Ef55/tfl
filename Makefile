test:
	cmake --build build --target Tests && build/bin/Tests

init: 
	cmake -Bbuild .

clear:
	rm -rf build/

.PHONY : test init clear