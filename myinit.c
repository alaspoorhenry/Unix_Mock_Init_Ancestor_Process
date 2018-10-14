#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

/* Tools to be aware of : struct,  */

static int rv=3;

struct process_holder{
    char run_level[1000];
    char status[100];
    char command[1000];
    int pid;
};

int main(int argc, char** argv)
{   
    extern int docommand(struct process_holder cmd[], int howmany);
    extern int there_space(char* buffer);
    int c, hld_c=0;
    FILE* fp;
    char param_check[100], bufcheck[1000], buf[1000], buf_2[1000] , buf_3[1000], buf_4[1000],*p, *t;
    struct process_holder cmd_hld[100];
    
    while ((c = getopt(argc, argv, "r:")) != EOF) {
        switch (c) {
            case 'r':
                rv = atoi(optarg);
                break;
            case '?':
            default:
                if (argc!=1) {
                    fprintf(stderr,"usage: [-r runlevel] file\n");
                    return 1;
                }
                break;
        }
    }
    if ((fp = fopen(argv[optind],"r")) == NULL){
        perror(argv[optind]);
        return 1;
    }
    /* parse the input here from opening the file  */
    while (fgets(buf, sizeof buf, fp)){
        if ((strcpy(buf_2, buf) == NULL)){
            fprintf(stderr,"Copy error\n");
            return 1;
            }
        if ((p = strchr(buf_2,'#')))
            *p='\0';
        if ((p = strchr(buf_2, '\n')))
            *p='\0';
        /* run-level comparison  */
        if (strcpy(bufcheck,buf_2)){
            if ((p=strchr(bufcheck, ':'))){
                *p='\0';
                strcpy(param_check,bufcheck);
            }
        }
        if ((((p=strchr(param_check,'0'+rv))) || ((strcmp(bufcheck,""))==0)) && (there_space(buf_2)==0)){
            if (((p = strchr(buf_2, ':')) && (t=strchr(buf_2,':')))){
                strcpy(buf_3,t+1);
                *p='\0';
                strcpy(cmd_hld[hld_c].run_level,buf_2);
                if ((p = strchr(buf_3, ':')) && (t=strchr(buf_3, ':'))){                
                    *p='\0';
                    strcpy(cmd_hld[hld_c].status,buf_3);
                if (!((strcmp(cmd_hld[hld_c].status, "respawn")) || (strcmp(cmd_hld[hld_c].status, "once")))){ 
                    perror(cmd_hld[hld_c].status);
                    return 1;
                }
                    strcpy(buf_4,t+1);
                    strcpy(cmd_hld[hld_c].command,buf_4);
                } else {
                    fprintf(stderr,"Missing Colon\n");
                    return 1;
                }
                hld_c++;
            } else {
                fprintf(stderr, "Unparsable line\n");
                return 1;
            }
        }
    }
    if (hld_c>100){
        fprintf(stderr,"Maximum number of entries exceeded");
        return 0;
    }
    fclose(fp);
    if (docommand(cmd_hld, hld_c)){
        return 1;
    }
    return 0;
}

int there_space(char *buf){
    while (*buf != '\0'){
        if (!isspace((unsigned char)* buf)){
            return 0;
            buf++;
        }
    }
    return 1;
}

int docommand(struct process_holder proc[], int howmany){
    char com[1000];
    int result, respawn_size=0, proc_ind=0;
    struct process_holder cont[howmany], cont_2[howmany];
    
    extern int respawncommand(struct process_holder arr[], int k);
    
    for (int i=0;i<howmany;i++){
        result = fork();
        if (result == -1){
            perror("fork:");
            exit(1);
        } else if (result==0) { /* child  */
            strcpy(com, proc[i].command);
            execl("/bin/sh","sh", "-c", com, (char *)NULL);
            perror("/bin/sh"); /* execl error checking, should only get here on error  */
            exit(1);
        } else { /* parent  */
            if ((strcmp(proc[i].status, "respawn")==0)){
                cont[i].pid = result;
            } else {
                cont[i].pid = -1;
            }
        }
    }
    
    for (int i=0;i<howmany;i++){
        pid_t pid;
        int status;
        if ((pid=wait(&status)) == -1 ){
            perror("wait"); 
        }
    }

    for (int i=0;i<howmany;i++){
        if (cont[i].pid>0){
            strcpy(cont_2[proc_ind].command,proc[i].command);
            respawn_size++;
            proc_ind++;
        }
    }
    
    if (respawn_size!=0){
        if ((respawncommand(cont_2,respawn_size)) == 1){
            return 1;
        }
    }
    
    return 0;
}

int respawncommand(struct process_holder proc[], int howmany){
    int result;
    char com[howmany];
    for (int i=0;i<howmany;i++){
        result=fork();
        if (result==-1){
            perror("fork:");
            exit(1);
        } else if (result == 0) {
            strcpy(com, proc[i].command);
            execl("/bin/sh", "sh", "-c", com, (char *)NULL);
        }
    }
    for (int i=0;i<howmany;i++){
        pid_t pid;
        int status;
        if ((pid=wait(&status)) == -1){
            perror("wait");
        }
    }
    respawncommand(proc,howmany);
    fprintf(stderr,"Respawn call fails\n");
    return 1;
}
