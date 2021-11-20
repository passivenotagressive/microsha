#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/resource.h>
#include <algorithm>
#include <fstream>

using namespace std;

void sigfunc(int val) {
    printf("\n");
}


vector<string> split(string s) {
    string word;
    vector<string> words;
    while(s.find('\t') != string::npos || s.find(' ') != string::npos){
        unsigned long n;
        if (s.find('\t') != string::npos && s.find(' ') == string::npos){
            n = s.find('\t');
        } else if (s.find('\t') == string::npos && s.find(' ') != string::npos){
            n = s.find(' ');
        } else {
            n = min(s.find(' '), s.find('\t'));
        }
        word = s.substr(0, n);
        s = s.substr(n + 1, s.length() - n - 1);
        if (word.length() != 0){
            words.push_back(word);
        }
    }
    if (s.length() != 0){
        words.push_back(s);
    }
    return words;
}

struct NFA{
    vector<int> Q;
    int F;
    vector<pair<int, char>> D;
}; //структура для очень конкретного вида НКА

NFA create(string R) {
    NFA A;
    int n = R.length();
    int k = 1; //количество состояний в автомате
    A.Q.push_back(0);
    for (int i = 0; i < n; ++i) {
        if (R[i] != '*') {
            A.Q.push_back(k);
            k++;
        }
    }
    vector<pair<int, char>> x(k);
    A.D = x;
    int m = 0; //счетчик состояний
    for (int i = 0; i < n; ++i) {
        if (R[i] == '*') {
            A.D[m].first = 1;
        } else if (R[i] == '?') {
            A.D[m].second = '*';
            m++;
        } else {
            A.D[m].second = R[i];
            m++;
        }
    } //заполняем таблицу переходов
    A.F = k - 1; //единственное финальное
    return A;
} //создание очень конкретного НКА по ограниченному виду РВ

bool check(string R, string s) {
    NFA A = create(R);
    int n = s.length();
    vector<int> cur;
    vector<int> next;
    cur.push_back(0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < cur.size(); ++j) {
            if (A.D[cur[j]].first == 1) {
                next.push_back(cur[j]);
            }
            if (A.D[cur[j]].second == s[i] || A.D[cur[j]].second == '*') {
                next.push_back(cur[j] + 1);
            }
        }
        cur = next;
        next.clear();
    }
    if (find(cur.begin(), cur.end(), A.F) == cur.end()) {
        return false;
    } else {
        return true;
    }
} //проверка принадлежности слова РЯ

vector<string> split_adress(string s) {
    string word;
    vector<string> words;
    while(s.find('/') != string::npos){
        unsigned long n;
        n = s.find('/');
        word = s.substr(0, n);
        s = s.substr(n + 1, s.length() - n - 1);
        if (word.length() != 0){
            words.push_back(word);
        }
    }
    if (s.length() != 0){
        words.push_back(s);
    }
    return words;
} //разбиение файла на директории

vector<vector<string>> split_components(vector<string> words) {
    vector<vector<string>> components;
    vector<string> component;
    for (int i = 0; i < words.size(); ++i) {
        if (words[i] == "|") {
            components.push_back(component);
            component.clear();
        } else {
            component.push_back(words[i]);
        }
    }
    components.push_back(component);
    return components;
} //разбиваем строку на компоненты конвейера

bool check_redirections(vector<vector<string>> components) {
    unsigned long n = components.size();
    if (n > 1) {
        for (int i = 0; i < components[0].size(); ++i) {
            if (components[0][i] == ">" || (components[0][i] == "<" && i != components[0].size() - 2)) return false;
        }
        for (int i = 1; i < n - 1; ++i) {
            for (int j = 0; j < components[i].size(); ++j) {
                if (components[i][j] == "<" || components[i][j] == ">") return false;
            }
        }
        for (int i = 0; i < components[n - 1].size(); ++i) {
            if (components[n - 1][i] == "<" || (components[n - 1][i] == ">" && i != components[n - 1].size() - 2)) return false;
        }
    } else {
        int k = 0;
        int m = 0;
        n = components[0].size();
        for (int i = 0; i < n; ++i) {
            if (components[0][i] == "<") k++;
            else if (components[0][i] == ">") m++;
        }
        if (k == 1 && m == 1 && n > 4 && ((components[0][n - 2] == "<" && components[0][n - 4] == ">") || (components[0][n - 2] == ">" && components[0][n - 4] == ">"))) return true;
        else if (k == 1 && n > 2 && components[0][n - 2] == "<") return true;
        else if (m == 1 && n > 2 && components[0][n - 2] == ">") return true;
        else if (m == 0 && k == 0) return true;
        else return false;
    }
    return true;
} //абсолютно уродски проверяем корректность перенаправлений

vector<string> arguements(vector<string> words) {
    string comand =  words[0];
    vector<string> arguements;
    int res = 0;

    for (int k = 0; k < words.size(); ++k) {
        if (words[k].find('*') != string :: npos ||  words[k].find('?') != string :: npos) {
            string adress = words[k];
            string line;
            if (adress.substr(0, 2) == "./"){
                adress = adress.substr(2, adress.length() - 2);
            }
            vector<string> dirs = split_adress(adress);
            unsigned long n = dirs.size();
            vector<string> queue(1);

            if (adress[0] == '/') {
                vector<string> queue_new;
                queue[0] = "/";
                for (int i = 0; i < n - 1; ++i) {
                    ofstream out;
                    out.open("files.txt");
                    out << "";
                    out.close();
                    for (int j = 0; j < queue.size(); ++j) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            int file = open("files.txt", O_WRONLY);
                            dup2(file, 1);
                            res = execlp("ls", "ls", queue[j].c_str(),  NULL);
                            if (res == -1) {
                                perror(nullptr);
                            }
                            close(file);
                        }
                        wait(nullptr);
                        ifstream fin("files.txt");
                        while(getline(fin, line)) {
                            DIR* res = opendir((queue[j] + line).c_str());
                            if(check(dirs[i], line) && res != nullptr) {
                                queue_new.push_back(queue[j] + line + "/");
                                closedir(res);
                            }
                        }
                        fin.close();
                    }
                    queue = queue_new;
                    queue_new.clear();
                }
                for (int j = 0; j < queue.size(); ++j) {
                    ofstream out;
                    out.open("files.txt");
                    out << "";
                    out.close();
                    pid_t pid = fork();
                    if (pid == 0) {
                        int file = open("files.txt", O_WRONLY);
                        dup2(file, 1);
                        execlp("ls", "ls", queue[j].c_str(),  NULL);
                        close(file);
                    }
                    wait(nullptr);
                    ifstream fin("files.txt");
                    while(getline(fin, line)) {
                        if (adress[adress.length() - 1] == '/') {
                            DIR* res = opendir(line.c_str());
                            if(check(dirs[n - 1], line) && res != nullptr) {
                                queue_new.push_back(queue[j] + line);
                                closedir(res);
                            }
                        } else {
                            if(check(dirs[n - 1], line)) {
                                queue_new.push_back(queue[j] + line);
                            }
                        }
                    }
                    fin.close();
                }
                queue = queue_new;
                queue_new.clear();
            }
            else {
                vector<string> queue_new;
                queue[0] = ".";
                for (int i = 0; i < n - 1; ++i) {
                    ofstream out;
                    out.open("files.txt");
                    out << "";
                    out.close();
                    for (int j = 0; j < queue.size(); ++j) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            int file = open("files.txt", O_WRONLY);
                            dup2(file, 1);
                            res = execlp("ls", "ls", queue[j].c_str(),  NULL);
                            if (res == -1) {
                                perror(nullptr);
                            }
                            close(file);
                        }
                        wait(nullptr);
                        ifstream fin("files.txt");
                        while(getline(fin, line)) {
                            DIR* res = opendir(line.c_str());
                            if(check(dirs[i], line) && res != nullptr) {
                                queue_new.push_back(queue[j] + "/" + line);
                                closedir(res);

                            }
                        }
                        fin.close();
                    }
                    queue = queue_new;
                    queue_new.clear();
                }

                for (int j = 0; j < queue.size(); ++j) {
                    ofstream out;
                    out.open("files.txt");
                    out << "";
                    out.close();
                    pid_t pid = fork();
                    if (pid == 0) {
                        int file = open("files.txt", O_WRONLY);
                        dup2(file, 1);
                        res = execlp("ls", "ls", queue[j].c_str(),  NULL);
                        if (res == -1) {
                            perror(nullptr);
                        }
                    }
                    wait(nullptr);
                    ifstream fin("files.txt");
                    while(getline(fin, line)) {
                        if (adress[adress.length() - 1] == '/') {
                            DIR* res = opendir(line.c_str());
                            if(check(dirs[n - 1], line) && res != nullptr) {
                                queue_new.push_back(queue[j] + "/" + line);
                                closedir(res);
                            }
                        } else {
                            if(check(dirs[n - 1], line)) {
                                queue_new.push_back(queue[j] + "/" + line);
                            }
                        }

                    }
                    fin.close();
                }
                queue = queue_new;
            }
            for (int j = 0; j < queue.size(); ++j) {
                arguements.push_back(queue[j]);
            }
        }
        else {
            arguements.push_back(words[k]);
        }
    }
    return arguements;
}

int find(vector<string> words, string sample) {
    int res = -1;
    for(int i = 0; i < words.size(); ++i) {
        if (words[i] == sample) res = i;
    }
    return res;
}

void free_args(char** args, unsigned long n) {
    //for (int i = 0; i < n; ++i) {
      //  free(args[i]);
    //}
    free(args);
}

int execute(char** home_dir, char** curr_dir, unsigned long* m, vector<string> words) {
    vector<vector<string>> components = split_components(words);//разбиваем все слова на компоненты конвейера
    for (int i = 0; i < components.size(); ++i) {
        unsigned long s = components[i].size();
        components[i] = arguements(components[i]);
        if (components[i].size() < s) {
            errno = ENOENT;
            perror(components[i][0].c_str());
            return 0;
        }
    }
    if (!check_redirections(components)) {
        write(2, "Incorrect redirection\n", 22);
    } else {
        string comand = components[0][0];
        if (comand == "time") {
            struct rusage rus;
            struct timespec begin, end;
            words.erase(words.begin());
            execute(home_dir,curr_dir, m, words);
            clock_gettime(CLOCK_REALTIME, &begin);
            clock_gettime(CLOCK_REALTIME, &end);
            long sec = end.tv_sec - begin.tv_sec;
            long nanosec = end.tv_nsec - begin.tv_nsec;
            double duration = sec + nanosec*1e-9;
            if ( getrusage(RUSAGE_CHILDREN, &rus) != -1 ){
                cout << "real: " << duration << "s\n";
                cout << "user: " << (double)rus.ru_utime.tv_sec+ (double)rus.ru_utime.tv_usec / 1000000.0 << "s\n";
                cout << "sys:  " <<  (double)rus.ru_stime.tv_sec + (double)rus.ru_stime.tv_usec / 1000000.0 << "s\n";
            }
        } else if (components.size() == 1) {
            if (comand == "cd"){
                if (words.size() == 1) {
                    chdir(home_dir[0]);
                    getcwd(curr_dir[0], m[0]);
                } else if (words.size() == 2) {
                    string s_str = words[1];
                    const char* s = s_str.c_str();
                    DIR* res = opendir(s);
                    if (res == nullptr) {
                        perror("");
                    } else {
                        curr_dir[0] = (char*)realloc(curr_dir[0], sizeof(char) * (m[0] + 1 + s_str.length()));
                        strcat(curr_dir[0], "/");
                        strcat(curr_dir[0], s); //добавляем к текущей директории папку
                        m[0] += s_str.length() + 1; //запоминаем длину текущей директории
                        chdir(curr_dir[0]);
                        closedir(res);
                    }
                } else {
                    errno  = E2BIG;
                    perror("");
                    //return 0;
                }
            }
            else if (comand == "pwd"){
                printf("%s\n", curr_dir[0]);
            } else {
                vector<string> args_v = components[0];
                int file_1 = -1;
                int file_2 = -1;
                int stdin_copy = dup(0);
                int stdout_copy = dup(1);
                int i = 0;
                while (i < args_v.size()){
                    if (args_v[i] == ">") {
                        file_1 = open(args_v[i + 1].c_str(), O_WRONLY);
                        if (file_1 == -1) {
                            errno = ENOENT;
                            perror(args_v[i].c_str());
                            //return 0;
                        } else {
                            args_v.erase(args_v.begin() + i + 1);
                            args_v.erase(args_v.begin() + i);
                            dup2(file_1, 1);
                        }
                    } else if (args_v[i] == "<") {
                        file_2 = open(args_v[i + 1].c_str(), O_RDONLY);
                        if (file_2 == -1) {
                            errno = ENOENT;
                            perror(args_v[i].c_str());
                            //return 0;
                        } else {
                            args_v.erase(args_v.begin() + i + 1);
                            args_v.erase(args_v.begin() + i);
                            dup2(file_2, 0);
                        }
                    } else {
                        i++;
                    }
                }
                char** args = (char**) malloc(sizeof(char*) * (args_v.size() + 1));
                for (i = 0; i < args_v.size(); ++i) {
                    args[i] = (char*) args_v[i].c_str();
                }
                args[args_v.size()] = nullptr;
                pid_t pid = fork();
                signal(SIGINT, sigfunc);
                if (pid == 0) {
                    execvp(comand.c_str(), args);
                }
                wait(nullptr);
                free_args(args, args_v.size());
                if (file_1 != -1) {
                    close(file_1);
                    dup2(stdout_copy, 1);
                }
                if (file_2 != -1){
                    close(file_2);
                    dup2(stdin_copy, 0);
                }
            }
        }
        else {
            int file;
            int stdin_copy = dup(0);
            int stdout_copy = dup(1);
            unsigned long n = components.size();
            int** fds = (int**)malloc(sizeof(int*) * n);
            for (int i = 0; i < n; ++i) {
                fds[i] = (int*)malloc(sizeof(int) * 2);
            }
            int k = find(components[0], "<");
            if (k != -1) {
                file = open(components[0][k + 1].c_str(), O_RDONLY);
                if (file == -1) {
                    errno = ENOENT;
                    perror(components[0][k + 1].c_str());
                    //return 0;
                } else {
                    components[0].erase(components[0].begin() + k + 1);
                    components[0].erase(components[0].begin() + k);
                    dup2(file, 0);
                }
            }
            vector<char**> args(n);
            for (int i = 0; i < n - 1; ++i) {
                args[i] = (char**) malloc(sizeof(char*) * (components[i].size() + 1));
                for (int j = 0; j < components[i].size(); ++j) {
                    args[i][j] = (char*) components[i][j].c_str();
                }
                args[i][components[i].size()] = nullptr;
            }
            pid_t pid;
            pipe(fds[0]);
            pid = fork();
            if (pid == 0) {
                dup2(fds[0][1], 1);
                close(fds[0][0]);
                execvp(components[0][0].c_str(), args[0]);
            }
            //wait(nullptr);
            if (k != -1) {
                close(file);
            }
            close(fds[0][1]);
            for (int i = 1; i < n - 1; ++i) {
                pipe(fds[i]);
                pid = fork();
                if (pid == 0) {
                    dup2(fds[i - 1][0], 0);
                    dup2(fds[i][1], 1);
                    close(fds[i][0]);
                    execvp(components[i][0].c_str(), args[i]);
                }
                //wait(nullptr);
                close(fds[i - 1][0]);
                close(fds[i][1]);
            }
            k = find(components[n - 1], ">");
            if (k != -1) {
                file = open(components[n - 1][k + 1].c_str(), O_WRONLY);
                if (file == -1) {
                    errno = ENOENT;
                    perror(components[n - 1][k + 1].c_str());
                    //return 0;
                } else {
                    components[n - 1].erase(components[n - 1].begin() + k + 1);
                    components[n - 1].erase(components[n - 1].begin() + k);
                    dup2(file, 1);
                }
            }
            args[n - 1] = (char**) malloc(sizeof(char*) * (components[n - 1].size() + 1));
            for (int j = 0; j < components[n - 1].size(); ++j) {
                args[n - 1][j] = (char*) components[n - 1][j].c_str();
            }
            args[n - 1][components[n - 1].size()] = nullptr;
            pid = fork();
            if (pid == 0) {
                dup2(fds[n - 2][0], 0);
                execvp(components[n - 1][0].c_str(),  args[n - 1]);
            }
            dup2(stdout_copy, 0);
            dup2(stdin_copy, 1);

            if (k == -1)  close(file);
            close(fds[n - 2][0]);
            for (int i = 0; i < n; ++i) {
                free(fds[i]);
                free_args(args[i], components[i].size());
            }

            free(fds);
        }
    }


    return 0;
}
int main() {
    int status;
    unsigned long m = 100; //предполагаем, что момент запуска программы длина текущей директории не более m
    char* home_dir = (char*) malloc(sizeof(char) * m);
    char* curr_dir = (char*) malloc(sizeof(char) * m);
    getcwd(home_dir, m);
    getcwd(curr_dir, m);
    char c = '>';
    if (strcmp(getenv("USER"), "root") != 0) {
        c = '!';
    }
    printf("%s%c", home_dir, c);
    string str;
    while (getline(cin, str)) {
        vector<string> words = split(str); //получаем массив слов введенной строки
        execute(&home_dir, &curr_dir, &m, words);
        do {
            status = wait(nullptr);
            if(status == -1 && errno != ECHILD) {
                perror("Error during wait()");
            }
        } while (status > 0);
        c = '>';
        if (strcmp(getenv("USER"), "root") != 0) {
            c = '!';
        }
        printf("%s%c", curr_dir, c);
    }
    free(home_dir);
    free(curr_dir);
    printf("\n");
}
