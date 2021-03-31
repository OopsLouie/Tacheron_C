#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>


#define TACHERONTAB_ROOT        "/etc/tacherontab"
#define TEMPFILE_ROOT           "/tmp"

// { -l | -r | -e }
enum {
    HANDLER_L = 1,
    HANDLER_R,
    HANDLER_E,
};

// show user how to use this program
void print_usage(int argc, char **argv) {
    printf("usage:  tacherontab [ -u user ] { -l | -r | -e }\n");
    printf("        -l:     list user's tacherontab\n");
    printf("        -r:     remove user's tacherontab\n");
    printf("        -e:     edit user's tacherontab\n");
    return;
}

int compaire_operation(char * input, char * target) {
    int i;
    if (strlen(input) != strlen(target)) {
        return -1;
    }
    for(i=0;i<strlen(input);i++) {
        if (input[i] != target[i]) {
            return -1;
        }
    }
    return 0;
}

// parse input
int parse_input(int argc, char **argv) {
    /* argv[0] is program name
     * argv[1] shoule be -u
     * argv[2] shoule be user
     * argv[3] shoule be operation handler
     */
    int ret_handler = 0;
    if (compaire_operation(argv[1], "-u")) {
        printf("tacherontab: invalid operation handler '%s'\n",argv[1]);
        return -1;
    }
    const struct passwd *pas;
    pas = getpwnam(argv[2]);
    if (pas == NULL) {
        printf("tacherontab: user not exist\n");
        exit(-1);
    }
    //printf("pw_name is %s\n", pas->pw_name);
    //printf("pw_passwd is %s\n", pas->pw_passwd);
    //printf("pw_dir is %s\n", pas->pw_dir);
    //printf("pw_shell is %s\n", pas->pw_shell);
    //printf("pw_gecos is %s\n", pas->pw_gecos);
    if (compaire_operation(argv[3], "-l") == 0) {
        ret_handler = HANDLER_L;
    } else if (compaire_operation(argv[3], "-r") == 0) {
        ret_handler = HANDLER_R;
    } else if (compaire_operation(argv[3], "-e") == 0) {
        ret_handler = HANDLER_E;
    } else {
        printf("tacherontab: invalid operation handler '%s'\n",argv[3]);
        return -1;
    }
    return ret_handler;
}

void cat_user_tacherontab(char * username) {
    int fs;
    char filename[BUFSIZ];
    char line[BUFSIZ];
    FILE * fp;
    sprintf(filename, "%s/tacherontab%s", TACHERONTAB_ROOT, username);
    //printf("filename is %s\n", filename);
    // judge if file exist
    fs = access(filename, F_OK);
    if (fs) {
        printf("tacherontab: no tacherontab file found for user %s\n", username);
        return;
    }
    // judge if file readable
    fs = access(filename, R_OK);
    if (fs) {
        printf("tacherontab: access denied for reading tacherontab file of user %s\n", username);
        return;
    }
    // open file only for read
    fp = fopen(filename,"r");
    if (!fp) {
        printf("tacherontab: internal error occurred");
        return;
    }
    // print each line
    while(1) {
        fgets(line, BUFSIZ, fp);
        if (feof(fp)) {
            break;
        }
        printf("%s", line);
    }
    fclose(fp);
    return;
}

int copy_file(char *src_filepath, char *dst_filepath) {
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    char buf[1024*2];
    uint32_t byte_cnt = 0;
    if((src_filepath == NULL) || (dst_filepath == NULL)) {
        return -1;
    }
    if((fp1 = fopen(src_filepath, "r")) != NULL) {
        if((fp2 = fopen(dst_filepath, "w")) != NULL) {
            while((byte_cnt = fread(buf, 1, sizeof(buf), fp1)) > 0) {
                fwrite(buf, byte_cnt, 1, fp2);
                memset(buf, 0, sizeof(buf));
            }
            fclose(fp1);
            fclose(fp2);
            return 0;
        }
    }
    return -1;
}

int edit_file_in_window(char * filename) {
    pid_t pid;

    pid = vfork();
    if (pid < 0) {
        return -1;
    } else if (pid == 0) {
        // child process : run editor
        //printf("==========================================\n");
        //printf("this is child process\n");
        //printf("child process exist\n");
        //printf("child process pid is %d\n", getpid());
        execlp("vi","vi",filename, NULL);
        exit(0);
    } else {
        // parent process : wait child process die
        //printf("==========================================\n");
        //printf("this is parent process\n");
        //printf("parent process pid is %d\n", getpid());
        int stat_val;
        pid_t child_pid;
        child_pid = wait(&stat_val);
        //printf("child process has exited, pid = %d\n", child_pid);
    }
    return 0;
}

void edit_user_tacherontab(char * username) {
    int fd;
    int fs;
    FILE *fp = NULL;
    char tempfilename[BUFSIZ];
    char finalfilename[BUFSIZ];
    sprintf(tempfilename, "%s/tacherontab.%u", TEMPFILE_ROOT, (unsigned) getpid());
    sprintf(finalfilename, "%s/tacherontab%s", TACHERONTAB_ROOT, username);
    printf("tempfilename is %s\n", tempfilename);
    printf("finalfilename is %s\n", finalfilename);
    if (access(TACHERONTAB_ROOT, F_OK)) {
        if(mkdir(TACHERONTAB_ROOT,0777)) {
            printf("tacherontab: have no access to create tacherontab dir\n");
            return;
        }
    }
    // check final file
    // judge if file exist
    fs = access(finalfilename, F_OK);
    if (fs) {
        // create final file
        fp = fopen(finalfilename, "w");
        if (!fp) {
            printf("tacherontab: have no access to create tacherontab file for user %s\n", username);
            return;
        }
    }
    // judge if file readable
    fs = access(finalfilename, R_OK);
    if (fs) {
        printf("tacherontab: tacherontab file of user %s isn't readable\n", username);
        return;
    }
    // judge if file writable
    fs = access(finalfilename, W_OK);
    if (fs) {
        printf("tacherontab: tacherontab file of user %s isn't writable\n", username);
        return;
    }
    // copy user's tacherontab file to temp file
    copy_file(finalfilename, tempfilename);
    // edit temp file
    edit_file_in_window(tempfilename);
    // save temp file to user's tacherontab file
    copy_file(tempfilename, finalfilename);
    printf("tacherontab: file successfully save to %s\n", finalfilename);
    // delete tempfile
    unlink(tempfilename);
}

void remove_user_tacherontab(char * username) {
    int fs;
    char filename[BUFSIZ];
    sprintf(filename, "%s/tacherontab%s", TACHERONTAB_ROOT, username);
    //printf("filename is %s\n", filename);
    // judge if file exist
    fs = access(filename, F_OK);
    if (fs) {
        // not exist do nothing
        printf("tacherontab: tacherontab file of user %s not found\n", username);
    } else {
        // judge if file readable
        fs = access(filename, W_OK);
        if (fs) {
            printf("tacherontab: access denied for removing tacherontab file of user %s\n", username);
            return;
        }
        // remove it
        unlink(filename);
    }
    return;
}

// example : tacherontab [-u user] { -l | -r | -e }
int main(int argc, char **argv) {
    int rc = 0;
    int handler = 0;
    do {
        if (argc != 4) {
            printf("tacherontab: invalid input\n");
            rc = -1;
            break;
        }
        handler = parse_input(argc, argv);
        if (handler < 0) {
            rc = -1;
            break;
        }
        switch(handler) {
            case HANDLER_L:
            cat_user_tacherontab(argv[2]);
            break;
            case HANDLER_R:
            remove_user_tacherontab(argv[2]);
            break;
            case HANDLER_E:
            edit_user_tacherontab(argv[2]);
            break;
            default:
            rc = -1;
            printf("tacherontab: internal error occurred\n");
            break;
        }
    } while(0);
    if(rc) {
        print_usage(argc, argv);
    }
    return rc;
}
