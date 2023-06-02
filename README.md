# fork-pipe-project

This is an application that is able to run successive commands similar to the following: 
wc -l someInputFile

wc -l < someInputFile

wc -l < someInputFile > someOutputFile

ls -ltr /usr/bin

ls -ltr /usr/bin > someOutputFile

ls -ltr /usr/bin | grep grep

 
 
The commands can be any system- or user-provided commands. Each command could take any number of arguments. Additionally, you should be able to parse and apply input- and output-redirection characters. Finally, program is able to handle commands that involve one pipe.

Program reads the commands from standard input and stop when it detects EOF.

To test code, use g++ -std=c++17 main.cpp
Program will output a '%' before prompting user to replicate using command line.
