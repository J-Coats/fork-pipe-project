/*
*  Author: Jared Coats
*  Date: Feb. 20th, 2023
*  Description: Program that is able to take in a command, system or user provided.
*  It's able to handle up to two redirection characters in a single command as well as
*  a single pipe currently.
*
*  
*/


#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <vector>
#include <sys/types.h>
#include <algorithm>
#include <stdio.h>



using namespace std;

const int MAX_LINE = 1024;
const int MAX_ARGS = 20;

// struct where I store most of the information needed when parsing through a command.
struct command_info {
    // command stores arguments when we don't see an input/output redirection
    // input_redirection_file stores whatever aguments come before seeing a '<'
    // output_redirection_file does the same for '>'

    string command;
    string input_redirection_file;
    string output_redirection_file;
    
};


void syserror( const char *s )
{
	fprintf(stderr, "%s", s);
        fprintf(stderr, " (%s)\n", strerror(errno) );
	exit( 1 );
}


// These two helpers are used by removeSpacesFromCommand to remove both leading and trailing spaces
void removeTrailingSpaces (string &str) {
    if (!str.empty()) {
        while (isspace(str[str.length() - 1]))
            str = str.substr(0, str.length() - 1);
    }
}

void removeLeadingSpaces (string & str) {
    if (!str.empty()) {
        while (isspace(str[0]))
            str = str.substr(1, str.length());
    }
}


// Used by parseLine to remove leading/trailing spaces from all strings stored in our command_info structs
void removeSpacesFromCommand(command_info& cmd)
{
    removeLeadingSpaces(cmd.command);
    removeLeadingSpaces(cmd.input_redirection_file);
    removeLeadingSpaces(cmd.output_redirection_file);
    removeTrailingSpaces(cmd.command);
    removeTrailingSpaces(cmd.input_redirection_file);
    removeTrailingSpaces(cmd.output_redirection_file);
}


// builds up a string of arguments in commandStr as long as we're not seeing a '|' or redirect
// as soon as we see a pipe, push commandStr into command string in the struct and then
// reset commandStr to grab the second set of arguments on the other side of the pipe.
// If we see an input or output redirect, store what is currently held in commandStr into
// the structs output_redirection_file/input_redirection_file variables instead of the command string
void parseLine (string sLine, vector<command_info> &commands, bool &pipe) {
    command_info command;
    string *commandStr = &command.command;

    for (int i = 0; i < sLine.length(); i++) {
        if (sLine[i] == '|')
        {
            removeSpacesFromCommand(command);
            pipe = true;
            commands.push_back(command);
            command.output_redirection_file = "";
            command.input_redirection_file = "";
            command.command = "";
            commandStr = &command.command;
        }
        else if (sLine[i] == '>') {
            commandStr = &command.output_redirection_file; 

        }
        else if (sLine[i] == '<') {
            commandStr = &command.input_redirection_file; 

        }
        else if (sLine[i] != '|' || sLine[i] != '>' || sLine[i] != '<') {
            *commandStr += sLine[i];
        }
    }
    removeSpacesFromCommand(command);
    commands.push_back(command);

}


// simply parses the command we pass it and stores it in argsForExec, called during the
// runCommand function before we use execvp
void parseCommand ( string command, char *line, char **argsForExec ) {
    

    string sLine = command;
    int num_args = 0;
    int illen = command.length();

    strcpy(line, sLine.c_str());
    const char *start = sLine.c_str();
    int startIdx;
    for( startIdx = 0; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx )
        ;

    if( line[startIdx] == '\0' ) {
        std::cout << "Empty input line.\n";
        return;
    }

    int endIdx;
    for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ); ++endIdx )
        ;
    line[endIdx] = '\0';   // a c-string that begins with "line[startIdx]" and ends at "line[endIdx]".
    argsForExec[num_args] = line + startIdx;  // argsForExec[0] by convention contains the name of the command.
    num_args++;
    //std::cout << "The first word on the line is: --" << line + startIdx << "--\n";


    while (line[endIdx + 1]) {
        if (endIdx + 1 > sLine.length())
            break;
        for( startIdx = endIdx + 1; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx )
            ;

        for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ); ++endIdx )
            ;
        line[endIdx] = '\0';   // a c-string that begins with "line[startIdx]" and ends at "line[endIdx]".
        argsForExec[num_args] = line + startIdx;  // argsForExec[0] by convention contains the name of the command.
        num_args++;

        //cout << "The word on the line is: --" << line + startIdx << "--\n";


    }

    argsForExec[num_args] = NULL;


}


// Calls parseCommand to fill argsForExec and executes differently depending on whether or not
// we've seen an input/output redirect (or both). Executes execvp.
void runCommand(const command_info& command) {
    char *argsForExec[ MAX_ARGS ];
    char line[ MAX_LINE];
    

    parseCommand(command.command, line, argsForExec);

    if (!command.input_redirection_file.empty()) {
        int readFD;
        const char *inputFile = command.input_redirection_file.c_str();
        if( (readFD = open( inputFile, O_RDONLY ) ) < 0 ) {  // read-only.
                char *buf = (char *) malloc( strlen( inputFile ) + strlen("Failed to open ") + 1 );
                sprintf( buf, "Failed to open %s", inputFile );
                syserror( buf );
            }
        
        if( dup2( readFD, 0 ) == -1 || close( readFD ) )
                syserror( "Failed to run dup2." );

    }

    if (!command.output_redirection_file.empty()) {
        int writeFD;
        const char *outputFile = command.output_redirection_file.c_str();
        if( (writeFD = open( outputFile, O_CREAT | O_WRONLY | O_TRUNC, 0777) ) < 0 ) {  
                char *buf = (char *) malloc( strlen( outputFile ) + strlen("Failed to open ") + 1 );
                sprintf( buf, "Failed to open %s", outputFile );
                syserror( buf );
            }
        
        if( dup2( writeFD, 1 ) == -1 || close( writeFD ) )
                syserror( "Failed to run dup2." );

    }

    if( execvp( argsForExec[0], argsForExec ) == -1 ) {
        perror("Failed to run.");
        exit( 1 );
    }


    
}

// This function gets executed if we don't see a pipe
void noPipesCase (const vector<command_info> &commands) {
    
    
    pid_t pid = fork();
    if( pid == -1 ) {
        std::cout << "fork failed.\n" << std::endl;
        exit( -1 );
    }
    if( pid == 0 ) {
        std::cout << "Child process is about to run execlp...\n";
    
        runCommand(commands[0]);
    }

    int childStatus;
    waitpid( pid, &childStatus, WUNTRACED );
    std::cout << "Parent process waited for the child process to terminate.\n";
    std::cout << "Child's exit status is :" << childStatus << std::endl;;

    return;
}


// this code gets executed if we see a single pipe
void onePipeCase (const vector<command_info> &commands) {
    
    int pfd[2], pid; // fd stands for file descriptor

    if ( pipe (pfd) == -1 )  // create a pipe and store its descriptors in pdf. pipe sets up mechanism to read from and write to.
        syserror( "Could not create a pipe" );
    printf("pid1 = %d\n", pfd[0]);
    switch ( pid = fork() ) {  // Create a process to read from the pipe.
        case -1:
            syserror( "Fork failed." );
            break;

        case  0:    // The child process.

            if ( close( 0 ) == -1 )    // close the child process's read file-descriptor
                syserror( "Could not close stdin I" );

            dup(pfd[0]); // copy the read end of the pipe in place of the process's read file-descriptor.
          

            if ( close (pfd[0]) == -1 || close (pfd[1]) == -1 )
                syserror( "Could not close pfds I" );

            runCommand(commands[1]);

            syserror( "unsuccessfull execlp I!!");
    }

    fprintf(stderr, "The first child's pid is: %d\n", pid);
    switch ( pid = fork() ) { // Create a process to write to the pipe.

        case -1:
            syserror( "Could not fork successfully II" );
            break;

        case  0:
            if ( close( 1 ) == -1 )
                syserror( "Could not close stdout" );
            dup(pfd[1]);

            if ( close (pfd[0]) == -1 || close (pfd[1]) == -1 )  // close pfd since it will be replaced with who
                syserror( "Could not close pfds II" );

            runCommand(commands[0]);

            syserror( "Execlp error" );
    }

    fprintf(stderr, "The second child's pid is: %d\n", pid);
    if ( close (pfd[0]) == -1 || close (pfd[1]) == -1 )   // close parent pfd
        syserror( "parent could not close file descriptors!!");

    while ( (pid = wait( (int *) 0 ) ) != -1) // waits for child processes
        ;    /* fprintf(stderr,"%d\n", pid) */
}




int main(int argc, char *argv[]) {


    string sLine;
    char *argsForExec [ MAX_ARGS];



    while (true) {
        vector<command_info> commands;
        bool pipe = false;
        cout << "% ";
        getline(cin, sLine);

        if (cin.eof()) break;

        parseLine(sLine, commands, pipe);

       
        if (!pipe) {
            noPipesCase(commands);
        }

        // if we saw a single pipe
        else {
            onePipeCase(commands);
        }


    }





    return 0;

}
