#!/usr/bin/bash

### Assemble and execute a list of programs in ../examples and record their output ###

if [[ $# -lt 3 ]]; then
	echo "Usage:: ./record.sh /PathToAsmExamples/ /PathToObjExamples/ /PathToGoldenFiles/"
	exit 1
fi

# Delete old records
find $3 -type f -name "*.output" -delete
find $3 -type f -name "*.hex" -delete

programs=('hello' 'counter' 'hello2')
input=('n' 'abcdefgz\nn' 'n' 'test')

execute_and_record_output(){
	#echo "Executing $1"
	
	printf $2 | ./lc3 $1 >> "$3"
}

declare -i index=0
for file in "${programs[@]}";
do
	echo "Recording --> $1/$file.asm"
	./lcasm -o $2/$file.obj $1/$file.asm 

	if [[ $? -ne 0 ]]; then
		echo "Couldn't assemble $1/$file.asm"
		echo "Aborting recording ..."
		exit 1
	fi
	
	# Save machine code output
	cp "$2/$file.obj" "$3/$file.expected.hex"

	execute_and_record_output "$2/$file.obj" "${input[$index]}" "$3/$file.expected.output"

	index+=1
done


