//gcc wish.c -Wall -g -o wish
//未来可以进行的改进工作
//将分割合并成一个函数
//增加信号处理
//资源释放处理
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<unistd.h>
#include <stdbool.h>
       #include <errno.h>
#define NODEBUG
#define MAX_SIZE 1024//一条命令最多字符数
#define MAX_NUMS_ARGS 10//一条命令最多参数数
#define NUMS_BUILT_IN_CMDS 10//最大命令数
#define NUMS_PATH 10//最大路径数
char*built_in_cmd[NUMS_BUILT_IN_CMDS];
char*path[NUMS_PATH];
int NOW_PATH=1;
int MAX_LINE=1024;
int num_of_arg=0;
char error_message[30] = "An error has occurred\n";
char*together_args[MAX_NUMS_ARGS]={0};
void init(){
    init_path();
    init_cmd_list();
}
void init_path(){
    path[0]="/bin";
    path[1]=".";
    NOW_PATH=2;
}
void init_cmd_list(){
    FILE* fp;
    fp=fopen("./config","r");
    if(fp==NULL){
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(0);
    }
    else{
        char line[MAX_SIZE]={0};
        int count=0;
        while(fscanf(fp,"%s",line)!=-1){
            if(strlen(line)!=1){
                built_in_cmd[count]=malloc(strlen(line)+1);
                strcpy(built_in_cmd[count],line);
                count++;
            }
        }
    }
    fclose(fp);
}
bool built_in(char*cmd){
    for(int i=0;i<NUMS_BUILT_IN_CMDS;i++){
        if(built_in_cmd[i]==NULL)
            break;
        if(strcmp(cmd,built_in_cmd[i])==0)
            return true;
    }
    return false;
}

int withpath_execvp(char*cmd,char **args){
    for(int i=0;i<NUMS_PATH;i++){
        if(path[i]==NULL){
            break;
        }
        else{
            char temp_dir[MAX_SIZE]={0};
            strcat(temp_dir,path[i]);
            strcat(temp_dir,"/");
            strcat(temp_dir,cmd);
            if(access(temp_dir,X_OK)==0){
                pid_t now;
                int status;
                now=fork();
                int result=1;
                if(now==0){
                    // close(STDERR_FILENO);
                    int result=execvp(temp_dir,args);//这里的返回错误不靠谱！执行期间发生的错误不算！出错：文件不存在，无法执行，内存不够。。。
                if (result == -1) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    }           
                    exit(0);
                }
                else{
                    waitpid(now,&status,0);
                    // if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    //     write(STDERR_FILENO, error_message, strlen(error_message));
                    // }
                    return 1;
                }
            }
        }
    }
    return 0;
}
void try_execvp(char*cmd,char **args){
    //遍历每条路径，拼接，然后执行

    if(withpath_execvp(cmd,args)==1){

    }
    else if(strcmp(cmd,"exit")==0){
        if(args[1]!=NULL){
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return;
        }
        exit(0);
    }
    else if(strcmp(cmd,"cd")==0){
        if(args[1]==NULL || args[2]!=NULL){
           write(STDERR_FILENO, error_message, strlen(error_message)); 
            return;
        }    
        int result=chdir(args[1]);
        if(result==-1){
            exit(-1);
        }
    }
    else if(strcmp(cmd,"path")==0){
        if(args[1]==NULL){
            memset(path,0,sizeof(path));
            NOW_PATH=1;
            path[0]=".";
        }
        else{
            for(int i=1;i<MAX_NUMS_ARGS;i++){
                if(args[i]==NULL){
                    break;
                }
                else{
                    path[NOW_PATH]=malloc(strlen(args[i])+1);
                    strcpy(path[NOW_PATH++],args[i]);
                }
            }
        }
    }
    else if(strcmp(cmd,"pathprint")==0){
        printf("Path:\n");
        for(int i=0;i<NOW_PATH;i++){
            printf("%s\n",path[i]);
        }
    }
    else{
        write(STDERR_FILENO, error_message, strlen(error_message)); 
    }

}
bool and_check(char**args){
    for(int i=0;i<num_of_arg;i++)
        if(strcmp(args[i],"&")==0)
            return true;
    return false;
}
//for two sides ! (>)
bool character_check(char character,char**args,char**target_file,bool* is_legal ){
    bool flag1=false;//找到符号
    char for_check[2]={character,0};
    *is_legal=true;
    for(int i=0;i<num_of_arg;i++){
        if(!flag1&&strcmp(args[i],for_check)==0){
            *is_legal=false;
            flag1=true;
            if(args[i+1]!=NULL&&args[i+2]==NULL){
                *target_file=args[i+1];
                *is_legal=true;
                return true;
            }
        }
        
    }
    return false;
}
bool is_legal_args(char**args){
    if(args==NULL||args[0]==NULL){
        return false;
    }
    return true;
}
void together_eval(char** args) {
    char *cmd = args[0]; // remember to free
    char* target_file = NULL;
    bool is_legal = true;
    bool together = and_check(args);

    if (together) {
        int loc = 1;
        int start_point = 0;
        memset(together_args, 0, sizeof(together_args));
        
        while (args[loc] != NULL) {
            if (strcmp(args[loc], "&") == 0) {
                // Copy the args to together_args
                for (int i = start_point; i < loc; i++) {
                    together_args[i-start_point] = args[i];
                }
                together_args[loc-start_point] = NULL; // Ensure NULL termination

                int pid = fork();
                if (pid == 0) {
                    // 在子进程中，执行命令
                    printf("子进程 %d 尝试运行命令: %s\n", getpid(), together_args[0]);
                    fflush(stdout);
                    eval(together_args);
                    exit(0);
                } else if (pid < 0) {
                    // 错误处理：fork 失败
                    perror("fork 失败");
                    exit(1);
                }

                start_point = loc + 1;
            }
            loc++;
        }

        // Handle last command after last '&'
        for (int i = start_point; i < loc; i++) {
            together_args[i-start_point] = args[i];
        }
        together_args[loc-start_point] = NULL; // Ensure NULL termination

        int pid = fork();
        if (pid == 0) {
            // 在子进程中，执行命令
            printf("子进程 %d 尝试运行命令: %s\n", getpid(), together_args[0]);
            fflush(stdout);
            eval(together_args);
            exit(0);
        } else if (pid < 0) {
            // 错误处理：fork 失败
            perror("fork 失败");
            exit(1);
        }

        // Wait for all child processes
        while (waitpid(-1, NULL, 0) > 0);
    }
}

void eval(char**args){
    char *cmd=args[0];//remeber to free
    char*target_file=NULL;
    bool is_legal=true;
    bool relocate=character_check('>',args,&target_file,&is_legal);
    if(!is_legal){
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        return;
    }
    int stdout_fd = -1;
    if(relocate){
        stdout_fd = dup(STDOUT_FILENO);
        freopen(target_file,"w",stdout);
        args[num_of_arg-1]=NULL;
        args[num_of_arg-2]=NULL;
    }
   
    if(built_in(cmd)){
        printf("内置命令执行！");
        try_execvp(cmd,args);
    }
    else{
        printf("外部命令执行！");
          int result=withpath_execvp(cmd,args);
          if(result==0){
            write(STDERR_FILENO, error_message, strlen(error_message)); 
          }
    }
    if(relocate){
        fflush(stdout);
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd); 
    }    
}
void free_resources(){
    //释放泄露内存
};
char _MySplit_str[10];
char* MySplit(char* input, const char* delimiter, char** save_ptr) {
	memset(_MySplit_str, 0, sizeof(_MySplit_str));
	if (input == NULL) {
		*save_ptr = NULL;
		return NULL;
	}
	char* ptr = strpbrk(input, delimiter);
	if (ptr == NULL) {

		*save_ptr = NULL;
		if (*input!=0) {
			return input;
		}
		return NULL;
	}
	_MySplit_str[0] = ptr[0];
	_MySplit_str[1] = NULL;
	if (strstr(delimiter, _MySplit_str)) {
		if (ptr != input) {
			*save_ptr = ptr;
			strncpy(_MySplit_str, input, ptr - input);
			return _MySplit_str;
		}
		*save_ptr = ptr+1;
		return  _MySplit_str;
	}
	else {
		*save_ptr = NULL;
		return input;
	}
}



int main(int argc, char *argv[]){
    bool together=false;
    char *input=NULL;//空地址传给getline即可，它会为你分配，注意不要用定长数组
    char *args[MAX_NUMS_ARGS]={0};
    init();
    if(argc==1){
        //Normal Mode
        while(1){
            together=false;
            num_of_arg=0;
            printf("wish> ");
            getline(&input,&MAX_LINE,stdin);
            //消除最后带入的空格
            input[strlen(input)-1]='\0';

            char*Arg;
            char*arg;
            //seperate input
            while(Arg=strsep(&input," \t")){
                if(strcmp(Arg,"")==0){
                    continue;
                }
                if(strcmp(Arg,"\t")==0){
                    continue;
                }
                while(arg=MySplit(Arg,">&",&Arg)){                
                // add args
                if(strcmp(arg,"&")==0){
                    together=true;
                }
                    args[num_of_arg++]=malloc(strlen(arg)+1);//注意加1
                    strcpy(args[num_of_arg-1],arg);
            
                }
            }
            args[num_of_arg]=NULL;
            if(!is_legal_args(args)){
                continue;
            }
            if(together){
                together_eval(args);
            }
            else
                eval(args);
            free(input);
            memset(args,0,sizeof(args));
            }
        
    }
    else if(argc==2){
        FILE*fp=fopen(argv[1],"r");
        if(fp==NULL){
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            exit(1);
        }
        while(getline(&input,&MAX_LINE,fp)!=-1){//注意是-1
            together=false;
            num_of_arg=0;
            //消除最后带入的空格
            input[strlen(input)-1]='\0';
            char*Arg;
            char*arg;
            //seperate input
            while(Arg=strsep(&input," ")){
                if(strcmp(Arg,"")==0){
                    continue;
                }
                if(strcmp(Arg,"\t")==0){
                    continue;
                }
                while(arg=MySplit(Arg,">&",&Arg)){                
                // add args
                if(strcmp(arg,"&")==0){
                    together=true;
                }
                args[num_of_arg++]=malloc(strlen(arg)+1);//注意加1
                strcpy(args[num_of_arg-1],arg);

                }
            }
            args[num_of_arg]=NULL;
            if(!is_legal_args(args)){
                continue;
            }
            if(together){
                together_eval(args);
            }
            else
                eval(args);
            free(input);
            memset(args,0,sizeof(args));
        }
    }
    else{
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }
    return 0;
}