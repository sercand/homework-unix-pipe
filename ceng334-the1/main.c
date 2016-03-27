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
#include "list.h"

#define LOGLEVEL 1

#ifdef LOGLEVEL
#define DEBUGLOG printf
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

typedef struct pipe_node {
    int id;
    int fildes[2];
    int *writers;
    int *readers;
    int writer_count;
    int reader_count;
} pipe_node;

int node_size = 0;
exe_node* nodes = NULL;


char ** parseCommand(char * str, int* size){
    char * pch;
    char ** cmds = malloc(0);
    int n = 0;
    pch = strtok (str," ");
    while (pch != NULL)
    {
        cmds = realloc(cmds, sizeof(char*) * (n + 1));
        cmds[n] = (char*)malloc(sizeof(char*));
        strcpy(cmds[n], pch);
        pch = strtok (NULL, " ");
        n= n+1;
    }
    cmds = realloc(cmds, sizeof(char*) * (n + 1));
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
                nodes[node_size].out_pipes = malloc(sizeof(int));
                nodes[node_size].out_pipes[0] = j;
            }else{
                nodes[node_size].out_pipes = realloc(nodes[node_size].out_pipes, sizeof(int) * k);
                nodes[node_size].out_pipes[k - 1] = j;
            }
            
            nodes[node_size].out_size = k;
        }
    }
    node_size = node_size + 1;
}

int is_executable(){
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
    return llvalid();
}

char* substring(const char* str, size_t begin, size_t len)
{
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

pipe_node* findPipeNode(int id,int nn,pipe_node* nodes){
    int i;
    for(i = 0; i < nn; i++){
        if(nodes[i].id == id){
            return &nodes[i];
        }
    }
    return NULL;
}

void run_command(char**cmd,int shouldRead, int pfdIn[2], int outn, int *pfdout[]){
    int     pid;
    int     fd[2];
    int     k;
    char    readbuffer[1025];
    ssize_t read_bytes;
    
    DEBUGLOG("run_command '%s' read='%d' outn='%d'\n", cmd[0], shouldRead, outn);
    
    if(outn > 1){
        pipe(fd);
        
        switch (pid = fork()) {
            case 0: /* child */
                if(shouldRead){
                    dup2(pfdIn[0], fileno(stdin));
                    close(pfdIn[1]);
                }
                dup2(fd[1], fileno(stdout));
                close(fd[0]);
                
                execvp(cmd[0], cmd);
                perror(cmd[0]);
            default: /* parent */
                close(fd[1]);
                for(k = 0; k < outn ; k++){
                    close(pfdout[k][0]);
                }
                while ((read_bytes = read(fd[0], readbuffer, 1024)) > 0) {
                    for(k = 0; k < outn ; k++){
                        write(pfdout[k][1], readbuffer, read_bytes + 1);
                    }
                }
                break;
            case -1:
                perror("fork");
                exit(1);
        }
        return;
    }
    
    if(shouldRead){
        dup2(pfdIn[0], fileno(stdin));
        close(pfdIn[1]);
    }
    
    if(outn == 1){
        dup2(pfdout[0][1], fileno(stdout));
        close(pfdout[0][0]);
    }
    
    execvp(cmd[0], cmd);
    perror(cmd[0]);
}

void run(exe_node node, pipe_node* in,  pipe_node * out[]){
    int pid,n=0, fd[2],k;
    int osize = node.out_size;
    int canRead = in!=NULL;
    int ** outpfd = malloc(sizeof(int*) * osize);
    char ** cmd2 = parseCommand(node.command, &n);
    DEBUGLOG("%s: %s\n",__FUNCTION__, node.command);
    
    if(canRead){
        fd[0] = in->fildes[0];
        fd[1] = in->fildes[1];
    }
    for(k=0 ; k < osize ; k++){
        outpfd[k] = malloc(sizeof(int)*2);
        outpfd[k][0] = out[k]->fildes[0];
        outpfd[k][1] = out[k]->fildes[1];
    }
    
    switch (pid = fork()) {
        case 0: /* child */
            run_command(cmd2, canRead, fd, osize, outpfd);
        default: /* parent */
            break;
        case -1:
            perror("fork");
            exit(1);
    }
}

void execute(){
    int i,j,k,n = 0;
    int status,pid;
    DEBUGLOG("%s: %d of nodes\n",__FUNCTION__, node_size);
    
    pipe_node* pnodes = (pipe_node*) malloc(0);
    for(i = 0; i < node_size; i++){
        exe_node exe = nodes[i];
        
        if(exe.in_pipe != -1){
            pipe_node* node = findPipeNode(exe.in_pipe, n, pnodes);
            if( node == NULL){
                pnodes = (pipe_node*) realloc(pnodes, sizeof(pipe_node)*( n+ 1));
                pnodes[n].id = exe.in_pipe;
                pnodes[n].writer_count = 0;
                pnodes[n].reader_count = 1;
                pnodes[n].readers = malloc(sizeof(int));
                pnodes[n].writers = malloc(0);
                pnodes[n].readers[0] = i;
                pipe(pnodes[n].fildes);
                n = n + 1;
            }else{
                k = node->reader_count;
                node->readers = realloc(node->readers,sizeof(int) * (k + 1));
                node->readers[k] = i;
                node->reader_count = k+1;
            }
        }
        
        for(j = 0; j < exe.out_size; ++j){
            pipe_node* node = findPipeNode(exe.out_pipes[j], n, pnodes);
            if(node == NULL){
                pnodes = (pipe_node*) realloc(pnodes,sizeof(pipe_node) * ( n+ 1));
                
                pnodes[n].id = exe.out_pipes[j];
                pnodes[n].writer_count = 1;
                pnodes[n].reader_count = 0;
                pnodes[n].readers = malloc(0);
                pnodes[n].writers = malloc(sizeof(int));
                pnodes[n].writers[0] = i;
                
                pipe(pnodes[n].fildes);
                
                n = n + 1;
            }else{
                k = node->writer_count;
                node->writers = realloc(node->writers, sizeof(int) * (k + 1));
                node->writers[k] = i;
                node->writer_count = k + 1;
            }
        }
    }
    if(n == 0){
        DEBUGLOG("run via stdin and stdout\n");
        for(i = 0; i < node_size; i++){
            run(nodes[i], NULL, NULL);
        }
        //   while ((pid = wait(&status)) != -1)
        //       fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
        return;
    }
    for(i = 0; i < node_size; i++){
        exe_node exe = nodes[i];
        pipe_node* in = findPipeNode(exe.in_pipe, n, pnodes);
        pipe_node** out = malloc(sizeof(pipe_node*) * exe.out_size);
        
        for(j = 0; j < exe.out_size; j++){
            out[j] = findPipeNode(exe.out_pipes[j], n, pnodes);
        }
        
        run(exe, in, out);
    }/*
      while ((pid = wait(&status)) != -1){
      fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
      }*/
    free(pnodes);
}



int main(int argc, const char * argv[]) {
    clear_nodes();
    int res;
    while(1){
        res = read_line();
        switch (res) {
            case 0:
                if(is_executable()){
                    execute();
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
