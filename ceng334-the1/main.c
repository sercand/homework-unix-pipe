//
//  main.c
//  ceng334-the1
//
//  Created by Sercan Değirmenci on 27/03/16.
//  Copyright © 2016 Otsimo. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "list.h"

//#define LOGLEVEL 1

#ifdef LOGLEVEL
#define DEBUGLOG(y, ...) fprintf(stderr, (y), __VA_ARGS__); fflush(stderr);
#else
#define DEBUGLOG(...)
#endif

typedef struct exe_node{
    char* command;
    int in_pipe;
    int* out_pipes;
    int out_size;
    char * origin_in;
    char * origin_out;
} exe_node;

typedef struct rename_pipeline{
    int old_id;
    int new_id;
} rename_pipeline;

int node_size = 0;
exe_node* nodes = NULL;

char ** parseCommand(char * str, int* size){
    char * pch;
    char ** cmds = (char **)malloc(0);
    int n = 0;
    pch = strtok (str," ");
    while (pch != NULL)
    {
        cmds = (char **) realloc(cmds, sizeof(char*) * (n + 1));
        cmds[n] = (char*) malloc(sizeof(char*));
        strcpy(cmds[n], pch);
        pch = strtok (NULL, " ");
        n= n+1;
    }
    cmds = (char **) realloc(cmds, sizeof(char*) * (n + 1));
    cmds[n] = 0;
    *size = (n+1);
    return cmds;
}

void add_node(char* cmd, char* in, char* out){
    //  int k;
    if(node_size == 0){
        nodes = (exe_node*) malloc(sizeof(exe_node));
    }else{
        nodes = (exe_node*)realloc(nodes, sizeof(exe_node) * (node_size + 1));
    }
    nodes[node_size].command = (char*)malloc(sizeof(char*));
    nodes[node_size].origin_in = (char*)malloc(sizeof(char*));
    nodes[node_size].origin_out = (char*)malloc(sizeof(char*));
    
    strcpy(nodes[node_size].command, cmd);
    
    if(in!=NULL){
        strcpy(nodes[node_size].origin_in, in);
        nodes[node_size].in_pipe = atoi(in);
    }else{
        nodes[node_size].in_pipe = -1;
    }
    nodes[node_size].out_size = 0;
    
    if(out != NULL){
        int i,j,k;
        char *end, *str = out;
        
        strcpy(nodes[node_size].origin_out, out);
        
        for(i = 0; *str; ++i)
        {
            j = (int)strtol(str, &end, 10);
            str = end;
            
            k = nodes[node_size].out_size + 1;
            
            if(k == 1){
                nodes[node_size].out_pipes =(int*) malloc(sizeof(int));
                nodes[node_size].out_pipes[0] = j;
            }else{
                nodes[node_size].out_pipes =(int*) realloc(nodes[node_size].out_pipes, sizeof(int) * k);
                nodes[node_size].out_pipes[k - 1] = j;
            }
            
            nodes[node_size].out_size = k;
        }
    }
    node_size = node_size + 1;
}

int is_executable(int* size){
    int i,j;
    llclear();
    
    for(i = 0; i < node_size; ++i){
        exe_node n = nodes[i];
        
        if(n.in_pipe != -1){
            lladd(n.in_pipe, 1, 0);
        }
        
        for(j = 0; j < n.out_size; ++j){
            lladd(n.out_pipes[j], 0, 1);
        }
    }
    return llvalid(size);
}

char* substring(const char* str, size_t begin, size_t len){
    if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
        return 0;
    
    return strndup(str + begin, len);
}

int read_line(){
    char * line = NULL;
    size_t len = 0;
    ssize_t read= getline(&line, &len, stdin);
    if(read != -1) {
        if(strcmp(substring(line, 0, strlen(line) - 1), "quit") == 0){
            return 1;
        }
        
#if LOGLEVEL
        if(strcmp(substring(line, 0, strlen(line) - 1), "debug") == 0){
            int i = 0;
            printf("number of nodes= %d\n",node_size);
            for(i=0;i<node_size;i++){
                printf("%d node: cmd='%s' in='%d'\n",i,nodes[i].command,nodes[i].in_pipe);
            }
            return 2;
        }
#endif
        
        char * inp = strstr(line, "<|");
        if(inp != NULL){
            long lposition = inp - line;
            char * outp = strstr(inp, ">|");
            if(outp != NULL){
                //CASE 3
                long rposition = outp - line;
                long srl = strlen(line) - rposition;
                long sll = rposition - lposition;
                
                char* exe = substring(line, 0, lposition -1 );
                char* in = substring(line, lposition + 3, sll - 4);
                char* out = substring(line, rposition + 3, srl - 4);
                add_node(exe, in, out);
            }else{
                //CASE 4
                long sl = strlen(line) - lposition;
                char* in = substring(line, lposition + 3, sl - 4);
                char* exe = substring(line, 0, lposition - 1);
                
                add_node(exe, in, NULL);
            }
        }else{
            char * outp = strstr(line, ">|");
            if( outp != NULL){
                //CASE 2
                long lposition = outp - line;
                long sl = strlen(line) - lposition;
                char* out = substring(line, lposition + 3, sl - 4);
                char* exe = substring(line, 0, lposition - 1);
                
                add_node(exe, NULL, out);
            }else{
                //CASE 1
                char* exe = substring(line, 0, strlen(line) - 1);
                add_node(exe, NULL, NULL);
            }
        }
    }
    return 0;
}

void clear_nodes(){
    free(nodes);
    node_size = 0;
    nodes = (exe_node*) malloc(sizeof(exe_node) * node_size);
}

void renamePipeIds(){
    int maxId = 0,i = 0,j = 0,k=0;
    int numPipe = 0;
    rename_pipeline* pipes =(rename_pipeline*) malloc(0);
    for(i = 0 ; i < node_size ; i++){
        if(nodes[i].in_pipe > -1){
            int id = nodes[i].in_pipe;
            int founded_index = -1;
            for(j = 0 ; j < numPipe ; j++){
                if(pipes[j].old_id == id){
                    founded_index = j;
                    break;
                }
            }
            if(founded_index < 0){
                pipes = (rename_pipeline*) realloc(pipes, sizeof(rename_pipeline) * (numPipe + 1));
                pipes[numPipe].old_id = id;
                maxId = maxId + 1;
                pipes[numPipe].new_id = maxId;
                nodes[i].in_pipe = maxId;
                numPipe = numPipe + 1;
            }else{
                nodes[i].in_pipe = pipes[founded_index].new_id;
            }
        }
        for(k = 0; k < nodes[i].out_size; k++){
            int id = nodes[i].out_pipes[k];
            int founded_index = -1;
            for(j = 0 ; j < numPipe ; j++){
                if(pipes[j].old_id == id){
                    founded_index = j;
                    break;
                }
            }
            if(founded_index < 0){
                maxId = maxId + 1;
                pipes = (rename_pipeline*) realloc(pipes, sizeof(rename_pipeline) * (numPipe + 1));
                pipes[numPipe].old_id = id;
                pipes[numPipe].new_id = maxId;
                nodes[i].out_pipes[k] = maxId;
                numPipe = numPipe + 1;
            }else{
                nodes[i].out_pipes[k] = pipes[founded_index].new_id;
            }
        }
    }
}

void run_command(char ** cmd, int readIndex, int outn, int * outIndex, int nop, int * pipes,int cmdIndex,int proxyOffset){
    int     pid;
    int     k;
    char    readbuffer[1025];
    ssize_t read_bytes;
    int     i;
    int     p = nop * 2;
    
    if(outn > 1) {
        int fd[2] = {pipes[(proxyOffset + cmdIndex) * 2], pipes[(proxyOffset + cmdIndex) * 2 + 1]};
        
        DEBUGLOG("%s: new pipes for '%s' %d:%d are %d:%d\n",__FUNCTION__, cmd[0],proxyOffset,cmdIndex, fd[0], fd[1]);
        
        switch (pid = fork()) {
            case 0:
                readIndex = readIndex - 1;
                if(readIndex >= 0){
                    if( dup2(pipes[readIndex * 2], 0) < 0){
                        DEBUGLOG("%s failed",__FUNCTION__);
                        perror("dup2 inside run_command and multiplex");
                        exit(EXIT_FAILURE);
                    }
                }
                while ((dup2(fd[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
                /*
                 if(dup2(fd[1], 1) < 0){
                 DEBUGLOG("%s failed",__FUNCTION__);
                 perror("dup2 failed to for multiplex output");
                 exit(EXIT_FAILURE);
                 }
                 */
                for(i = 0 ; i < p ; i++){
                    if(close(pipes[i]) < 0){
                        perror(cmd[0]);
                        exit(EXIT_FAILURE);
                    }
                }
                
                if(execvp(cmd[0], cmd) < 0){
                    DEBUGLOG("%s failed",__FUNCTION__);
                    perror(cmd[0]);
                    exit(EXIT_FAILURE);
                }
                DEBUGLOG("%s success",__FUNCTION__);
                exit(0);
            default:
                for(i = 0 ; i < p ; i++){
                    int f = 0;
                    for(k = 0; k < outn ; k++){
                        int l = (outIndex[k] - 1) * 2 + 1;
                        if(l == i){
                            f = 1;
                        }
                    }
                    if(i == ((proxyOffset + cmdIndex) * 2)){
                        f = 1;
                    }
                    if(!f){
                        if(close(pipes[i]) < 0){
                            DEBUGLOG("%s failed",__FUNCTION__);
                            perror(cmd[0]);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                while ((read_bytes = read(fd[0], readbuffer, 1024)) > 0) {
                    DEBUGLOG("%s: multiplex read some data '%s'\n",__FUNCTION__, readbuffer);
                    for(k = 0; k < outn ; k++){
                        write(pipes[(outIndex[k] - 1) * 2 + 1], readbuffer, read_bytes + 1);
                    }
                }
                DEBUGLOG("%s: multiplex data finished readed='%zd' errno='%d'\n",__FUNCTION__, read_bytes, errno);
                
                close(fd[0]);
                for(k = 0; k < outn ; k++){
                    close(pipes[(outIndex[k] - 1) * 2 + 1]);
                }
                exit(0);
                break;
            case -1:
                DEBUGLOG("%s failed",__FUNCTION__);
                perror("fork");
                exit(1);
        }
    }else{
        readIndex = readIndex - 1;
        if(readIndex >= 0){
            if( dup2(pipes[readIndex * 2], 0) < 0){
                DEBUGLOG("%s failed read\n",cmd[0]);
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        }
        
        if(outn == 1){
            if( dup2(pipes[(outIndex[0] - 1) * 2 + 1], 1) < 0 ){
                DEBUGLOG("%s failed write\n",cmd[0]);
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        }
        
        for(i = 0 ; i < p ; i++){
            if(close(pipes[i]) < 0){
                perror("dup2: close");
                exit(EXIT_FAILURE);
            }
        }
        
        if(execvp(cmd[0], cmd) < 0){
            DEBUGLOG("%s failed",__FUNCTION__);
            perror(cmd[0]);
            exit(EXIT_FAILURE);
        }
    }
}

void run(exe_node node,int index, int nop, int * pipes, int proxyOffset){
    int pid,n = 0;
    int osize = node.out_size;
    int readIndex = -1;
    int outIndex = -1;
    char ** cmd = parseCommand(node.command, &n);
    readIndex = node.in_pipe;
    
    if(osize > 0) {
        outIndex = node.out_pipes[0];
    }
    DEBUGLOG("%s: %d of %d cmd='%s' nop='%d' ri='%d' wi='%d'\n",__FUNCTION__,index+1, (nop-proxyOffset), node.command, nop, readIndex, outIndex);
    switch (pid = fork()) {
        case 0:
            run_command(cmd, readIndex, osize, node.out_pipes, nop, pipes,index, proxyOffset);
        default:
            DEBUGLOG("%s: %s parent=%d\n",__FUNCTION__,node.command,pid);
            break;
        case -1:
            perror("fork");
            exit(1);
    }
}

void execute(int numberOfPipes){
    int i,t;
    int status,pid;
    DEBUGLOG("%s: %d of nodes and %d pipes\n",__FUNCTION__, node_size,numberOfPipes);
    
    t = numberOfPipes + node_size;
    int *fd = (int*) malloc(sizeof(int) * t * 2);
    for(i = 0; i < t;i++){
        fd[i*2] = 0;
        fd[i*2+1] = 0;
        pipe(fd + ( i * 2));
    }
    
    renamePipeIds();
    
    if(numberOfPipes == 0){
        DEBUGLOG("%s run via stdin and stdout\n",__FUNCTION__);
        for(i = 0; i < node_size; i++){
            run(nodes[i], i, 0, NULL,0);
        }
        while ((pid = wait(&status)) != -1){
            DEBUGLOG("process %d exits with %d\n", pid, WEXITSTATUS(status));
        }
        free(fd);
        return;
    }
    
    for(i = 0; i < node_size; ++i){
        DEBUGLOG("%s: %d executing\n",__FUNCTION__, i+1)
        run(nodes[i], i, t, fd, numberOfPipes);
        DEBUGLOG("%s: %d executed\n",__FUNCTION__, i+1)
    }
    
    for(i = 0 ; i < (t*2) ; i++){
        close(fd[i]);
    }
    
    while ((pid = wait(&status)) != -1){
        DEBUGLOG( "process %d exits with %d\n", pid, WEXITSTATUS(status));
    }
    free(fd);
}

int main(int argc, const char * argv[]) {
    clear_nodes();
    int res;
    int pipes=0;
    while(1){
        res = read_line();
        switch (res) {
            case 0:
                if(is_executable(&pipes)){
                    execute(pipes);
                    clear_nodes();
                }
                break;
            case 1:
                exit(0);
            default:
                break;
        }
    }
    return 0;
}
