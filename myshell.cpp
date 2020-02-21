#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <stack>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

void fork_process(string path, string linux_command, string extention, string extention2){
	pid_t pid;
	pid = fork();
	if(pid<0){
		fprintf(stderr,"Fork failed!");
		return;
	}

	else if(pid==0){//child process
		if(extention==" "){
			execlp(path.c_str(), linux_command.c_str(), NULL);
		}
		else if(extention != " " && extention2 == " "){
			execlp(path.c_str(), linux_command.c_str(), extention.c_str(), NULL);
		}
		else if(extention!= " " && extention2 != " "){
			execlp(path.c_str(), linux_command.c_str(), extention.c_str(), extention2.c_str(), NULL);
		}
		else{
			cout<<"Something went wrong!"<<endl;
		}
		exit(0);
	}

	else{
		int status;
		waitpid(pid, &status, 0);
			
	}
	
}
//taking command as a parameter, parse it with deliminator " ", push all the tokens to a queue
queue<string> parse_command(string command){
	queue<string> q;
	std::stringstream ss(command);
	string token;
	char delim = ' ';
	while(std::getline(ss,token,delim)){
		if(token=="grep"){ // in the case that grep argument contains spaces, we didn't parse the token
			q.push(token);
			string rest;
			std::getline(ss, rest);
			q.push(rest);
			break;
		}
		else{
			q.push(token);
		}
		
	}
	return q;
}
/**for piping, I benefit from the given link https://stackoverflow.com/questions/33884291/pipes-dup2-and-exec**/
//used for piping ls and grep commands
void pipe_process(string fpath, string firstcmd, string frsarg, string spath,  string scmd, string secarg){
	
    pid_t pid;                                  
    int fd[2];                                                                                   
    pipe(fd);                                    
    pid = fork();  
                            
    if(pid==0){  			//child process                                          
        dup2(fd[1], STDOUT_FILENO); 	//move the STDOUT of the ls to the read end of the pipe
	close(fd[1]);     		//close the read end of the pipe for ls
        close(fd[0]); 			//close the write end of the pipe for ls
	if(frsarg!=" "){
		execlp(fpath.c_str(), firstcmd.c_str(), frsarg.c_str(), NULL);	//execute ls with -a
	}
        else{
		execlp(fpath.c_str(), firstcmd.c_str(), NULL);			//execute ls with no arguments
	} 
	fprintf(stderr, "Failed to execute");
	exit(1);
    }                                            
    else                                         
    {                                           
        pid=fork();	//fork another process for grep (grandchild)                                                      
        if(pid==0)                              
        {                                        	
            dup2(fd[0], STDIN_FILENO);	//move the STDIN of the grep to the write end of the pipe
	    close(fd[0]);		//close the write end of the pipe for grep
            close(fd[1]);               //close the read end of the pipe for grep

            execlp(spath.c_str(),scmd.c_str(),secarg.c_str(), NULL);	//execute grep with an argument
	    fprintf(stderr, "Failed to execute");
	    exit(1);

        }                                        
	else{
	int status;
        close(fd[0]);                  
        close(fd[1]);                    
        waitpid(pid, &status, 0);
	}                  

    }                  
	
}
/** In printfile command with redirection, I benefit from the given link http://digi-cron.com:8080/programmations/c/lectures/8-dup.html**/
//used for redirecting the output of cat to another file
void redirect_cat_process(string infile, string outfile1){
	pid_t pid;
	int fd;
	pid=fork(); 
	if(pid == 0){
		if((fd = open(outfile1.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600))==-1){ //open the file outfile1
		perror("open outfile");
		exit(2);
		}
		close(STDOUT_FILENO);	//close the stdout of the cat to the terminal
		dup(fd);		//move the STDIN of the cat to the write end of the pipe
		execlp("/bin/cat", "cat", infile.c_str(), NULL);
		
	}
	wait(NULL);
	
	return; 


}

int main() 
{	
	stack<string> command_s;
	string user_name = getenv("USER");
     	cout<<"Welcome to my Shell, "<<user_name<<"!"<<endl;
	
	while(1){
	cout<<user_name<<" >>> ";
	string command;
	getline(cin, command);
	
	//if no command entered, continue to showing the prompt
	if(command==""){
		continue;
	}
	
	command_s.push(command); 	//pushed to be used later in footprint command
	queue<string> tokens = parse_command(command);
	string token = tokens.front();
	
		if(token == "exit"){ 	//if entered command is exit, finish the program
			cout<<"You exitted!"<<endl;
			return 0;
		}

		else{
			tokens.pop();
			string extention=" ";
			if(token=="listdir"){		//execute listdir
				if(tokens.size()<=1){	//listdir with one or non argument
					if(tokens.size()==1){
					extention = tokens.front();
					tokens.pop();
					}	
					
					fork_process("/bin/ls","ls",extention, " ");

				}
				else{	//listdir | grep "arg" or listdir -a | grep "arg"
				
					if(tokens.front()!="|"){ //if it does not enter the if, extention would be " " (listdir | grep "arg")
						extention = tokens.front();
						tokens.pop();
						
					}
					
						tokens.pop(); // | is popped
						tokens.pop(); // grep is popped
						string search = tokens.front();
						search = search.substr(1,search.length()-2);
						tokens.pop(); //search element is popped
						pipe_process("/bin/ls", "ls" ,extention , "/bin/grep",  "grep", search);

					
				}
				
			}
			
			else if(token=="currentpath"){//execute currentpath
				fork_process("/bin/pwd","pwd",extention, " ");
				continue;
			}
			
			else if(token == "printfile"){//execute printfile
				if(tokens.size()==1){//printfile "file.txt"
					string file = tokens.front();
					tokens.pop();
					fork_process("/bin/cat","cat",file, " ");

					
				}
				else if(tokens.size()>1){
					string file1 = tokens.front();
					tokens.pop();//first file name popped
					tokens.pop();//">" popped
					string file2 = tokens.front();
					tokens.pop();//second file name popped
					redirect_cat_process( file1,  file2);
				}
				else{
					cout<<"You should enter a file name."<<endl;
				}
			}
			//pop first 15 element from command_s stack and push them to another stack to get the first command at the top
			else if(token == "footprint"){//execute footprint
				stack<string> old_s;
				while(old_s.size()<15 && !command_s.empty()){
					string temp = command_s.top();
					old_s.push(temp);
					command_s.pop(); 

				}
				int size = old_s.size();
				for(int i = 0; i<size ;i++){ // pop and print the first 15 element 
					string temp = old_s.top();
					old_s.pop();
					command_s.push(temp);
					cout<<i+1<<" "<<temp<<endl;
				}

			}
			else{
				cout<<"No command found : "<<command<<endl;

			}
		

		
		}

	}
	
	return 0;
}
