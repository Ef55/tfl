#!/bin/sh

# Clone repo
git clone https://github.com/nst/JSONTestSuite.git suite

# Build parser and copy it
cd ../../.. 
cmake --build build --target Json 
cd examples/json/testing
cp ../../../build/bin/Json suite/parsers/test_tfl_json

# Clean repo
cd suite
git checkout -- .

# Add the parser to the list
sed -i -z 's/programs = {\n/programs = {\n    "C++ TFL based":\n        {\n            "url":"",\n            "commands": [os.path.join(PARSERS_DIR, "test_tfl_json")],\n        },\n/' run_tests.py

# Run tests
echo '["C++ TFL based"]' > tfl_only.json
echo 'Running tests...'
python3 run_tests.py --filter=tfl_only.json >/dev/null
echo 'Done !'
