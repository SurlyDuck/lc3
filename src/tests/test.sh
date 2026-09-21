#!/usr/bin/bash

# Execute and assemble a list of programs in ../examples and compare it to their golden files"

source ./tests/golden.sh

if [[ $# -lt 2 ]]; then
	echo "Usage:: ./test.sh /PathToAsmExamples/ /PathToGoldenFiles/"
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
	
	diff -q "$1/bin/$file.obj" "$2/$file.expected.obj" > /dev/null
	if [[ $? != 0 ]]; then
		# Failure at machine code comparison
		echo " --> Failure: Machine code output differ"
		failures+=1
		index+=1
		continue
	fi

	printf "${input[$index]}" | ./lc3 "$1/bin/$file.obj" > temp_out.txt
	diff -q "$2/$file.expected.output" temp_out.txt > /dev/null
	# We won't be using the current index beyond this point
	index+=1

	if [[ $? != 0 ]]; then
		# Failure at program output comparison
		echo "--> Failure: Program output differ"
		failures+=1
		continue
	fi
	
	echo " --> Pass"
	passes+=1
done

echo "----------------------------"
echo "Failures = $failures"
echo "Passes   = $passes"
