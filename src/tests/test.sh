#!/usr/bin/bash

# Execute and assemble a list of programs in ../examples and compare it to their golden files"

source ./tests/golden.sh

if [[ $# -lt 2 ]]; then
	echo "Usage:: ./test.sh /PathToAsmExamples/" /PathToGoldenFiles/
	exit 1
fi

declare -i index passes failures
index=0
passes=0
failures=0

for file in "${programs[@]}"; do
	echo -n "Testing --> $1/$file.asm"

	./lcasm -o "$1/bin/$file.obj" "$1/$file.asm"
	if [[ $? != 0 ]]; then
		# Failure at assembling no need to test the rest
		echo  " --> Failure: Couldn't assemble"
		failures+=1
		continue
	fi
	
	diff -q "$1/bin/$file.obj" "$2/$file.expected.obj"
	if [[ $? != 0 ]]; then
		# Failure at machine code comparison
		echo "--> Failure: Machine code output differ"
		failures+=1
		continue
	fi
	
	echo " --> Pass"
	# TODO: execute and compare output

	index+=1
done
