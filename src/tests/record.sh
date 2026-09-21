#!/usr/bin/bash

### Assemble and execute a list of programs in ../examples and record their output ###

source ./tests/golden.sh

if [[ $# -lt 2 ]]; then
	echo "Usage:: ./record.sh /PathToAsmExamples/ /PathToGoldenFiles/"
	exit 1
fi

# Delete old records
find $2 -type f -name "*.output" -delete
find $2 -type f -name "*.obj" -delete

execute_and_record_output(){
	printf $2 | ./lc3 $1 >> "$3"
}

declare -i index=0
for file in "${programs[@]}";
do
	echo "Recording --> $1/$file.asm"
	./lcasm -o $2/$file.expected.obj $1/$file.asm 

	if [[ $? -ne 0 ]]; then
		echo "Couldn't assemble $1/$file.asm"
		echo "Aborting recording ..."
		exit 1
	fi

	execute_and_record_output "$2/$file.expected.obj" "${input[$index]}" "$2/$file.expected.output"

	index+=1
done


