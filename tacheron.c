#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <pwd.h>

#define log(fmt, ...) \
    gettimeofday(&tv,NULL); \
    printf("%02ld:%02ld:%02ld.%06ld: ", tv.tv_sec % (24 * 3600) / 3600, tv.tv_sec % (24 * 3600) % 3600 / 60 , tv.tv_sec % (24 * 3600) % 60, tv.tv_usec); \
    printf(fmt,## __VA_ARGS__); \
    fflush(stdout);

#define FLUSH fflush(stdout);
#define USEC_PER_SEC            1000000
#define TACHERONTAB_ROOT        "/etc/tacherontab"

typedef struct {
    uint8_t sec[4];
    uint8_t minute[60];
    uint8_t hour[24];
    uint8_t day[31+1];
    uint8_t months[12+1];
    uint8_t weekday[7+1];
    char * mission;
} mission_t;

struct timeval tv;
mission_t missions[1024];
int mission_cnt = 0;

static void adjust_time_to_nearest_point() {
    gettimeofday(&tv,NULL);
    usleep((15 - tv.tv_sec % (24 * 3600) % 60 % 15) * USEC_PER_SEC - tv.tv_usec);
    return;
}

void clean_missions() {
    int i;
    for (i=0;i<1024;i++) {
        if(missions[i].mission) {
            free(missions[i].mission);
        }
    }
    memset(missions, 0, sizeof(mission_t) * 1024);
    mission_cnt = 0;
    return;
}

void print_mission(int index) {
    char line[BUFSIZ];
    char tmp[10];
    char *sec = "second: ";
    char *minute = "minute: ";
    char *hour = "hour: ";
    char *day = "day: ";
    char * months = "months: ";
    char *weekday = "weekday: ";
    char *mission = "mission: ";
    int i;
    log("======================================\n");
    memset(line, 0, BUFSIZ);
    memcpy(line, sec, strlen(sec) * sizeof(char));
    for(i=0;i<4;i++) {
        if (missions[index].sec[i] == 1) {
            sprintf(tmp, "%d ", i * 15);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    memset(line, 0, BUFSIZ);
    memcpy(line, minute, strlen(minute) * sizeof(char));
    for(i=0;i<60;i++) {
        if (missions[index].minute[i] == 1) {
            sprintf(tmp, "%d ", i);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    memset(line, 0, BUFSIZ);
    memcpy(line, hour, strlen(hour) * sizeof(char));
    for(i=0;i<24;i++) {
        if (missions[index].hour[i] == 1) {
            sprintf(tmp, "%d ", i);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    memset(line, 0, BUFSIZ);
    memcpy(line, day, strlen(day) * sizeof(char));
    for(i=1;i<=31;i++) {
        if (missions[index].day[i] == 1) {
            sprintf(tmp, "%d ", i);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    memset(line, 0, BUFSIZ);
    memcpy(line, months, strlen(months) * sizeof(char));
    for(i=1;i<=12;i++) {
        if (missions[index].months[i] == 1) {
            sprintf(tmp, "%d ", i);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    memset(line, 0, BUFSIZ);
    memcpy(line, weekday, strlen(weekday) * sizeof(char));
    for(i=1;i<=7;i++) {
        if (missions[index].weekday[i] == 1) {
            sprintf(tmp, "%d ", i);
            strcat(line, tmp);
        }
    }
    log("%s\n",line);
    log("mission is %s\n",missions[index].mission);
    log("======================================\n");
    return;
}

void get_param_array_ptr_and_len(mission_t *mission_ptr, int param_index, uint8_t **ptr, int *len) {
    switch (param_index) {
        case 0:
            *ptr=mission_ptr->sec;
            *len=4;
            break;
        case 1:
            *ptr=mission_ptr->minute;
            *len=60;
            break;
        case 2:
            *ptr=mission_ptr->hour;
            *len=24;
            break;
        case 3:
            *ptr=mission_ptr->day;
            *len=31+1;
            break;
        case 4:
            *ptr=mission_ptr->months;
            *len=12+1;
            break;
        case 5:
            *ptr=mission_ptr->weekday;
            *len=7+1;
            break;
        default:
            // impossible here
            break;
    }
    return;
}

int parse_until_empty_char(uint8_t *ptr, int len, char *line, int *i) {
    char *original_c_ptr = &line[*i];
    char *c_ptr = original_c_ptr;
    char *num_start = NULL;
    char *num_end = NULL;
    char num_str[BUFSIZ];
    int j;
    int num_int;
    int num_len;
    int former_num = -1;
    char former_punc = '?';
    while(1) {
        if(*c_ptr <= '9' && *c_ptr >= '0') {
            if(!num_start) {
                num_start=c_ptr;
                num_end=c_ptr;
                c_ptr++;
            } else {
                num_end++;
                c_ptr++;
            }
            continue;
        } else if (*c_ptr == ',') {
            if (former_punc == '-' || former_punc == '~') {
                return -1;
            }
            num_len = num_end - num_start + 1;
            memcpy(num_str, num_start, num_len * sizeof(char));
            num_str[num_len] = '\0';
            num_int = atoi(num_str);
            if (num_int >= len) {
                return -1;
            } else {
                ptr[num_int] = 1;
                c_ptr++;
                former_punc = ',';
                former_num = num_int;
                num_start=NULL;
                continue;
            }
        } else if (*c_ptr == '-') {
            if (former_punc != '?') {
                return -1;
            }
            num_len = num_end - num_start + 1;
            memcpy(num_str, num_start, num_len * sizeof(char));
            num_str[num_len] = '\0';
            num_int = atoi(num_str);
            if (num_int >= len) {
                return -1;
            } else {
                c_ptr++;
                former_punc = '-';
                former_num = num_int;
                num_start=NULL;
                continue;
            }
        } else if (*c_ptr == '~') {
            if (former_punc != '-' && former_punc != '~') {
                return -1;
            }
            if (former_punc == '-') {
                num_len = num_end - num_start + 1;
                memcpy(num_str, num_start, num_len * sizeof(char));
                num_str[num_len] = '\0';
                num_int = atoi(num_str);
                if (num_int >= len) {
                    return -1;
                }
                for(j=former_num;j<=num_int;j++) {
                    ptr[j] = 1;
                }
                c_ptr++;
                former_punc = '~';
                former_num = num_int;
                num_start=NULL;
                continue;
            } else {
                num_len = num_end - num_start + 1;
                memcpy(num_str, num_start, num_len * sizeof(char));
                num_str[num_len] = '\0';
                num_int = atoi(num_str);
                if (num_int >= len) {
                    return -1;
                } else {
                    ptr[num_int] = 0;
                    c_ptr++;
                    former_punc = '~';
                    former_num = num_int;
                    num_start=NULL;
                    continue;
                }
            }
        } else if (*c_ptr == ' ' || *c_ptr == '\t') {
            if (former_punc == '?' || former_punc == ',') {
                num_len = num_end - num_start + 1;
                memcpy(num_str, num_start, num_len * sizeof(char));
                num_str[num_len] = '\0';
                num_int = atoi(num_str);
                if (num_int >= len) {
                    return -1;
                } else {
                    ptr[num_int] = 1;
                    *i += (c_ptr - original_c_ptr);
                }
            } else if (former_punc == '-') {
                num_len = num_end - num_start + 1;
                memcpy(num_str, num_start, num_len * sizeof(char));
                num_str[num_len] = '\0';
                num_int = atoi(num_str);
                if (num_int >= len) {
                    return -1;
                } else {
                    for(j=former_num;j<=num_int;j++) {
                        ptr[j] = 1;
                    }
                    *i += (c_ptr - original_c_ptr);
                }
            } else if (former_punc == '~') {
                num_len = num_end - num_start + 1;
                memcpy(num_str, num_start, num_len * sizeof(char));
                num_str[num_len] = '\0';
                num_int = atoi(num_str);
                if (num_int >= len) {
                    return -1;
                } else {
                    ptr[num_int] = 0;
                    *i += (c_ptr - original_c_ptr);
                }
            } else {
                return -1;
            }
            return 0;
        } else {
            return -1;
        }
    }
}
void add_all_possibility(uint8_t *ptr, int len) {
    int i;
    for(i=0;i<len;i++) {
        ptr[i] = 1;
    }
    return;
}

int parse_each_mission(char *line) {
    int i;
    int param_index = 0;
    uint8_t *ptr;
    int len;
    get_param_array_ptr_and_len(&missions[mission_cnt], param_index, &ptr, &len);
    for(i=0;i<strlen(line);i++) {
        if (param_index == 6) {
            if(line[i] == ' ' || line[i] == '\t') {
                continue;
            } else if(line[i] == '\n') {
                return -1;
            } else {
                missions[mission_cnt].mission = (char *)malloc((strlen(line) - i - 1) * sizeof(char));
                memcpy(missions[mission_cnt].mission, &line[i], strlen(line) - i - 1);
                return 0;
            }
        }
        if (line[i] == '#') {
            return 0;
        } else if (line[i] == ' ' || line[i] == '\t') {
            continue;
        } else if (line[i] == '\n') {
            // line with only \n
            if (i == 0) {
                return 0;
            }
        } else if (line[i] == '*') {
            if(line[i+1] != ' ' && line[i+1] != '\t') {
                return -1;
            }
            add_all_possibility(ptr, len);
            param_index++;
            get_param_array_ptr_and_len(&missions[mission_cnt], param_index, &ptr, &len);
        } else if (line[i] >= '0' && line[i] <= '9') {
            if(parse_until_empty_char(ptr, len, line, &i)) {
                return -1;
            } else {
                param_index ++;
                get_param_array_ptr_and_len(&missions[mission_cnt], param_index, &ptr, &len);
            }
            continue;
        } else {
            // invalid char
            return -1;
        }
    }
    return -1;
}

int read_mission_from_file() {
    struct dirent *entry = NULL;
    DIR *d = opendir(TACHERONTAB_ROOT);
    FILE *fp;
    char filename[BUFSIZ];
    char line[BUFSIZ];
    char * standard_prefix = "tacherontab";
    const struct passwd *pas;
    int rc = 0;
    do {
        clean_missions();
        if(d) {
            // read mission from file
            for(entry = readdir(d); entry != NULL; entry = readdir(d)) {
                if (!strcmp(entry->d_name, ".")) {
                    continue;
                }
                if (!strcmp(entry->d_name, "..")) {
                    continue;
                }
                if(strlen(entry->d_name) <= strlen("tacherontab")) {
                    log("a\n");
                    continue;
                }
                if(memcmp(entry->d_name, standard_prefix, strlen(standard_prefix))) {
                    continue;
                }
                memset(line, 0, BUFSIZ);
                memcpy(line, entry->d_name + strlen(standard_prefix), strlen(entry->d_name) - strlen(standard_prefix));
                pas = getpwnam(line);
                if (pas == NULL) {
                    // no user may be invalid filename
                    continue;
                }
                //log("read file named %s\n",entry->d_name);
                sprintf(filename, "%s/%s", TACHERONTAB_ROOT, entry->d_name);
                if(access(filename, R_OK)) {
                    log("cannot read tacherontab file '%s'\n",filename);
                    rc = -1;
                    break;
                }
                fp = fopen(filename, "r");
                if (!fp) {
                    log("read tacherontab file '%s' error\n",filename);
                    rc = -1;
                    break;
                }
                while(1) {
                    fgets(line, BUFSIZ, fp);
                    if (feof(fp)) {
                        break;
                    }
                    if (strlen(line) == 1 && line[0] == '\n') {
                        // only \n
                        continue;
                    }
                    if(parse_each_mission(line)) {
                        log("invalid mission in tacherontab file\n");
                        rc = -1;
                        break;
                    } else {
                        mission_cnt ++;
                    }
                }
                if (rc == -1) {
                    break;
                }
            }
            if (rc == -1) {
                break;
            }
        } else {
            // no tacherontab file
            log("no tacherontab file, wait until next 15 seconds\n");
            rc = -1;
        }
    } while(0);
    if(rc) {
        clean_missions();
    }
// for debug
#if 0
    if(mission_cnt > 0) {
        int j;
        for(j=0;j<mission_cnt;j++) {
            print_mission(j);
        }
    } else {
        log("no mission!!!\n");
    }
#endif
    return rc;
}

void carry_out_mission(char * mission) {
    int rc;
    rc = system(mission);
    if(rc) {
        log("mission '%s' carried out failed! error code is %d\n",mission, rc);
    } else {
        log("mission '%s' carried out success!\n",mission);
    }
    return;
}

int decide_carry_out_mission(int sec, int min, int hour, int day, int months, int weekday, int mission_index) {
    //log("%d/%d 星期 %d  %d:%d:%d, deciding mission %d \n", months, day, weekday, hour, min, sec, mission_index);
    if (missions[mission_index].sec[sec/15] == 1 &&
        missions[mission_index].minute[min] == 1 &&
        missions[mission_index].hour[hour] == 1 &&
        missions[mission_index].day[day] == 1 &&
        missions[mission_index].months[months] == 1 &&
        missions[mission_index].weekday[weekday] == 1) {
        carry_out_mission(missions[mission_index].mission);
    }
    return 0;
}

int main() {
    int i;
    struct tm *lt;
    // read mission
    log("tacheron service started...\n");
    if(read_mission_from_file()) {
        log("read tacherontab files failed!May be next round will success!\n");
    } else {
        log("read tacherontab files success!\n");
    }
    // adjust time to 0, 15, 30, 45 sec
    adjust_time_to_nearest_point();
    while(1) {
        // update time
        gettimeofday(&tv,NULL);
        lt = localtime(&(tv.tv_sec));
        // dicede mission should be carried out and carry it out
        for(i=0;i<mission_cnt;i++) {
            decide_carry_out_mission(lt->tm_sec,
                                     lt->tm_min,
                                     lt->tm_hour,
                                     lt->tm_mday,
                                     lt->tm_mon+1,
                                     // Zeller
                                     ((lt->tm_mday+2*(lt->tm_mon+1)+3*((lt->tm_mon+1)+1)/5+(lt->tm_year+1900)+(lt->tm_year+1900)/4-(lt->tm_year+1900)/100+(lt->tm_year+1900)/400)%7+1)%7,
                                     i);
        }
        adjust_time_to_nearest_point();
        // update mission
        if(read_mission_from_file()) {
            log("update tacherontab files failed!May be next round will success!\n");
            log("all recorded tacherontab mission already be cleand!\n");
        } else {
            log("update tacherontab files success!\n");
        }
    }
    return 0;
}
