#include <unistd.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <string.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <fcntl.h>

using namespace std;

void outhostname(){
		char host[32];
		if(gethostname(host, 32) == -1){
			perror("gethostname error");
			exit(1);
		}	
		else{ 
			char* login;
			if((login = getlogin()) == NULL){
				perror("getlogin error");
				exit(1);
			}
			cout << login << '@' << host << "$ ";
		}
}

string trim_space(string const& str){
	if(str.empty()) return str;

	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, last-first+1);
}

string trim_tab(string const& str){
	if(str.empty()) return str;

	size_t first = str.find_first_not_of('\t');
	size_t last = str.find_last_not_of('\t');
	return str.substr(first, last-first+1);
}

string trim(string const& str){
	string temp = trim_space(str);
	temp = trim_tab(temp);
	return temp;
}

void exec_go(char** path){
	int pid = fork();
	if(pid == 0){
		if(-1 == execvp(path[0], path)){
			cerr << "11" << path[0] << "11\n" << flush;
			perror("Error with exec_go execvp");
		}
		exit(1);
	}
	else if(pid > 0){
		if(-1 == wait(0)){
			perror("Error with exec_go wait");
			exit(1);
		}
	}
	else {
		perror("Error with exec_go fork");
		exit(1);
	}
}

vector<char*> tokenize_go(char* uinput, string inputBlock){
	//tokenize here	
	vector<char*> tokenlist;				
	char *token;

	strcpy(uinput, inputBlock.c_str());

	token = strtok(uinput, " \t");				//tokenize and add to vector of char*
	while(token != NULL){
		tokenlist.push_back(token);
		token = strtok(NULL, " \t");
	}

	tokenlist.push_back('\0');
	
	return tokenlist;
}

void pipe_go(string outputLeft, string outputRight){
	int orig, orig2;
	if(-1 == (orig = dup(1))) {
		perror("Error with pipe dup");
		exit(1);
	}
	if(-1 == (orig2 = dup(0))) {
		perror("Error with pipe dup");
		exit(1);
	}
	int fd[2];
	if(-1 == pipe(fd)) {
		perror(__FILE__ ": pipe");
	}
//cerr << "fd0 = " << fd[0] << endl;
//cerr << "fd1 = " << fd[1] << endl;
	
	int pid = fork();
	if(pid < 0){
		perror("Error with pipe fork");
		exit(1);
	}
	else if(pid == 0) {
		char *uinput = new char[outputLeft.length() +1];	//turns string into c*
		if(-1 == close(fd[0])) perror("Error with close2");
		if(-1 == close(1)) perror("Error with close");
		if(-1 == dup(fd[1])) perror("Error with dup");
		if(-1 == close(fd[1])) perror("Error with close2");

		vector<char*> prog1 = tokenize_go(uinput, outputLeft);
		char **argg = &prog1[0];
		exec_go(argg);
	//	if(-1 == close(fd[1])) perror("Error with close3");
	//	if(-1 == close(fd[0])) perror("Error with close2");
		if(-1 == dup2(orig, 1)){
			perror("Error with redirect_out dup2");
			exit(1);
		}
		if(-1 == close(orig)) {
			perror("Error with redirect_out close1");
			exit(1);
		}
		delete [] uinput;
		exit(0);
	}
	else{ 
		if(fork() == 0){
			char *uinput = new char[outputRight.length() +1];	//turns string into c*
			if(-1 == close(fd[1])) perror("Error with close2");
			if(-1 == close(0)) perror("Error with close");
			if(-1 == dup(fd[0])) perror("Error with dup");
			if(-1 == close(fd[0])) perror("Error with close2");

			vector<char*> prog2 = tokenize_go(uinput, outputRight);
			char **argg = &prog2[0];
			exec_go(argg);
		//	close(fd[0]);
		//	if(-1 == close(fd[1])) perror("Error with close3");
			if(-1 == dup2(orig2, 0)){
				perror("Error with redirect_out dup2");
				exit(1);
			}
			if(-1 == close(orig2)) {
				perror("Error with redirect_out close1");
				exit(1);
			}
			delete [] uinput;
			exit(1);
		}
		if(-1 == close(fd[1])) perror("Error with close4");
		if(-1 == close(fd[0])) perror("Error with close4");
		if(-1 == wait(0)){
			perror("Error with pipe_go wait");
			exit(1);
		}
		if(-1 == wait(0)){
			perror("Error with pipe_go wait");
			exit(1);
		}
	}

	return;
}

void redirect_out(int flag, char** outputLeft, char* inputRight){
	int orig;
	if(-1 == (orig = dup(1))){
		perror("Error with redirect_out dup");
		exit(1);
	}
	if(-1 == close(1)) {
		perror("Error with redirect_out close1");
		exit(1);
	}
	if(flag == 5) {
		if (open(inputRight, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU) == -1) perror("Error with redirect_out open1");
		else{
			//output to inputRight
			exec_go(outputLeft);
			if(-1 == close(1)) {
			perror("Error with redirect_out close2");
			exit(1);
			}
		}
	}
	else if(flag == 6) {
		if(open(inputRight, O_WRONLY|O_CREAT|O_APPEND, S_IRWXU) == -1) perror("Error with redirect_out open2");
		else{
			//output to inputRight
			exec_go(outputLeft);		
			if(-1 == close(1)) {
			perror("Error with redirect_out close2");
			exit(1);
			}
		}
	}
	if(-1 == dup2(orig, 1)){
		perror("Error with redirect_out dup2");
		exit(1);
	}
	if(-1 == close(orig)) {
		perror("Error with redirect_out close1");
		exit(1);
	}
	return;
}

void redirect_in(char** outputLeft, char* inputRight){
	int orig;
	if(-1 == (orig = dup(0))){
		perror("Error with redirect_out dup");
		exit(1);
	}
	if(-1 == close(0)){
		perror("Error with redirect_in close");
		exit(1);
	}
	if(open(inputRight, O_RDONLY)  == -1) perror("Error with redirect_in open");
	else{
		exec_go(outputLeft);
		if(-1 == close(0)) {
			perror("Error with redirect_out close2");
			exit(1);
		}
	}
		if(-1 == dup2(orig, 0)){
			perror("Error with redirect_out dup2");
			exit(1);
		}
		if(-1 == close(orig)) {
			perror("Error with redirect_out close1");
			exit(1);
		}
	return;
}


int main(int argc, char* argv[]){
	while(1){
		outhostname();
		//get input
		string userinput, inputBlock, connector, inputRight;
		getline(cin, userinput);
		unsigned int pf = 0;
		bool first = true;
		int fd[2];
		int prevflag = 0;

		while(!userinput.empty()){
			unsigned int here;
			unsigned int flag = 0;		

cout << "userinput: " << userinput << endl << endl; 

			unsigned int first_char = userinput.find_first_not_of(" \t");
		
			if(first_char > userinput.size()) break; 
			userinput = trim(userinput);

			if(userinput.find_first_of("#") != string::npos){
				userinput = userinput.substr(0, userinput.find_first_of("#"));
			}

			userinput.append(" ");	
			

			if(userinput.find_first_of(";&|") != string::npos){
				here = userinput.find_first_of(";&|");
				if(userinput.at(here) == ';'){
					flag = 3;
					inputBlock = userinput.substr(0, here);
					connector = userinput.substr(here, 1);
					string rest = userinput;
					userinput = rest.substr(here+1);
				}	
				else if( ((userinput.at(here) == '&') && (userinput.at(here+1) == '&'))
						|| ( (userinput.at(here) == '|') && (userinput.at(here+1) == '|'))){
					if(userinput.at(here) == '&')	flag = 1;
					if(userinput.at(here) == '|')	flag = 2;
					inputBlock = userinput.substr(0, here);
					connector = userinput.substr(here, 2);
					string rest = userinput;
					userinput = rest.substr(here+2);
				}
			}

		//string test = userinput;
			vector<string> blocks, redirs;
			while(userinput.find_first_of("<>|") != string::npos){
				here = userinput.find_first_of("<>|");
				blocks.push_back(userinput.substr(0, here));
				if(userinput.at(here) == '>' && userinput.at(here+1) == '>'){
					redirs.push_back(userinput.substr(here, 2));
					string hold = userinput;
					userinput = hold.substr(here+2);
				}
				else{	
					redirs.push_back(userinput.substr(here, 1));
					string hold = userinput;
					userinput = hold.substr(here+1);
				}
			}
			blocks.push_back(userinput);
			cout << endl;
			for(unsigned int i = 0; i < blocks.size(); i++){
				blocks.at(i) = trim(blocks.at(i));
				cout << i << blocks.at(i) << "!"  << endl;
			}
			cout << "----------------------" << endl;
			for(unsigned int i = 0; i < redirs.size(); i++){
				cout << i << redirs.at(i) << "!" <<  endl;
				if(redirs.at(i) == "|") flag = 4;
				else if(redirs.at(i) == ">") flag = 5;
				else if(redirs.at(i) == ">>")flag = 6;
				else if(redirs.at(i) == "<") flag = 7;
				else cout << "ERROR with redir flags" << endl;
			}
	cout << endl;
/*
			cout << "inputBlock :" << inputBlock << "1" << endl;
			cout << "connector :" << connector << "2" << endl;
			cout << "rest of userinput :" << userinput << "3" << endl;
			cout << "inputRight :" << inputRight << '4' << endl << endl;
*/
			if(flag == 0){
				inputBlock = userinput;
				userinput = "";
			}

			if(inputBlock.empty() && flag < 4)	{
				cout << "execvp: No such file or directory" << endl;
				break;
			}

		//tokenize here	
			vector<char*> tokenlist;				
			char *uinput = new char[inputBlock.length() +1];	//turns string into c*
			tokenlist = tokenize_go(uinput, inputBlock);
			char **argg = &tokenlist[0]; 

//	cout << "flag begin: " << flag << "  prevflag: " << prevflag << endl << endl;

			if( ((pf == 0) && (prevflag == 1)) || ((pf == 1) && (prevflag == 2)) || (prevflag == 3) || (first && !((flag == 4) ||  (flag == 5) || (flag == 6) || (flag == 7)) ) ){	
				pf = 0;
				if(strcmp(tokenlist.at(0), "exit") == 0)
					exit(0); 	
				if(strcmp(tokenlist.at(0), "false") == 0){
					break;
					pf = 1;
				}
				else {
					if(-1 == pipe(fd)){
						perror("Pipe fail");
						exit(1);
					}
					int pid = fork();	
					if(pid == 0){
						if(-1 == close(fd[0])){
							perror("Close fail");
							exit(1);
						}
						int check = execvp(argg[0], argg);
						if(check == -1){	
							pf = 1;
							if(-1 == write(fd[1], &pf, sizeof(pf))){
								perror("Write fail");
								exit(1);
							}
							if(-1 == close(fd[1])){
								perror("Close fail");
								exit(1);
							}
							perror("execvp");
						}
						else{
							pf = 0;
							if(-1 == write(fd[1], &pf, sizeof(pf))){
								perror("Write fail");
								exit(1);
							}
							if(-1 == close(fd[1])){
								perror("Close fail");
								exit(1);
							}
							perror("execvp");
						}
						exit(0);
					}
					else if(pid > 0){
						if(-1 == waitpid(pid, NULL, 0)){
							perror("Waitpid fail");
							exit(1);
						}
						if(-1 == close(fd[1])){
							perror("Close fail");
							exit(1);
						}
						if(-1 == read(fd[0], &pf, sizeof(pf))){
							perror("Read fail");
							exit(1);
						}
						if(-1 == close(fd[0])){
							perror("Close fail");
							exit(1);
						}
					}
					else {
						perror("fork");
						exit(1);
					}
				}
			}
			else if(flag >= 4){
				int orig_in = 0;
				bool inp = false;
				bool pip = false;
				if(redirs.at(0) == "<"){			//has input redirection, close stdin
					inp = true;
					redirs.erase(redirs.begin());
					if(-1 == (orig_in = dup(0))){
						perror("Error with dup");
						exit(1);
					}
					if(-1 == close(0)){
						perror("Error with close");
						exit(1);
					}
					char *uinputt = new char[blocks.at(1).length() +1];	//turns string into c*
					strcpy(uinputt, blocks.at(1).c_str());
					if(open(uinputt, O_RDONLY) == -1) perror("Error with open");
					delete [] uinputt;
				}
		//if pipe found
		//loop for each pipe some how
				if(find(redirs.begin(), redirs.end(), "|") != redirs.end()){
					pip = true;
					vector<int*> fds;

					vector<string>::iterator iter = redirs.begin();
					while(fds.size() < 5 && find(iter, redirs.end(), "|") != redirs.end()){
						int *fd1 = new int[2];
						fds.push_back(fd1);
						redirs.erase(find(iter, redirs.end(), "|"));
					}

					//pipe function - if > , you can close(1), until the end, then run redirect_out with last block
					//				- if < , already closed(0), just close(1) until end
					//				- if | , close(0) and close(1), follow the paper..

					for(unsigned int i = 0; i < fds.size(); i++){
						//run pipe fundtion?
					}

					for(unsigned int i = 0; i < fds.size(); i++)
						delete [] fds.at(i);
				}
				if(!pip){		//execvp the first block, rest are used already
					vector<char*> toklist;				
					char *uinputi = new char[blocks[0].size() +1];	//turns string into c*
					toklist = tokenize_go(uinputi, blocks[0]);
					char **arggg = &toklist[0]; 

					char *uinputt = new char[blocks.at(blocks.size()-1).size() +1];	//turns string into c*
					strcpy(uinputt, blocks.at(blocks.size()-1).c_str());

					if(!(redirs.empty()) && (redirs.at(redirs.size()-1) == ">" || redirs.at(redirs.size()-1) == ">>")){
						redirect_out(flag, arggg, uinputt);
					}
					else{
						exec_go(arggg);
					}
					if(inp){
						if(-1 == close(0)){
							perror("Error with close");
							exit(1);
						}
						if(-1 == dup2(orig_in, 0)){
							perror("Error with dup2");
							exit(1);
						}
						if(-1 == close(orig_in)){
							perror("Error with close");
							exit(1);
						}
					}
					delete [] uinputi;
					delete [] uinputt;
				}
				
			}

			first = false;
			prevflag = flag;
			delete [] uinput;
		}
	}
	return 0;
}
