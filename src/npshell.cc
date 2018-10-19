#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
//#define DBG(x, ...) fprintf(stderr, x, __VA_ARGS__);
#define DBG(x, ...) 0;
#define IS_PIPE(x) ((x) > 2)
using namespace std;

void convert(const vector<string> &from, vector<char *> &to) {
    // convert vector of c++ string to vector of c string
    auto it_end = from.end();
    for (auto it = from.begin(); it != it_end; ++it)
        to.push_back(const_cast<char *>(it->c_str()));
    to.push_back(NULL);
}

void exec(const vector<vector<string>> &args, deque<int> &pidin, deque<int> &pidout,
                 int fdin, int fdout, int mode) {
    // fdin -> (exec args)  -> fdout
    const size_t len = args.size();
    size_t i, cur;
    int pid[2], fd[2][2];
    for (i = 0; i < len; ++i) {
        cur = i & 1;
        if (i != len - 1) pipe(fd[cur]);
        while ((pid[cur] = fork()) == -1) {
            waitpid(pidin.front(), NULL, 0);
            pidin.pop_front();
        }
        if (pid[cur] == 0) {
            // fd[0] -> stdin
            if (i != 0) {
                close(fd[1 - cur][1]);
                dup2(fd[1 - cur][0], 0);
            } else {
                dup2(fdin, 0);
            }
            // fd[1] -> stdout
            if (i != len - 1) {
                close(fd[cur][0]);
                dup2(fd[cur][1], 1);
            } else {
                dup2(fdout, 1);
                if (mode == 21) dup2(fdout, 2);
            }
            vector<char *> arg;
            convert(args[i], arg);
            DBG("#%d %zu: exec %s\n", __LINE__, i, arg[0]);
            execvp(arg[0], &arg[0]);
            cerr << "Unknown command: [" << arg[0] << "]." << endl;
            exit(0);
        }
        pidout.push_back(pid[cur]);
        if (i != 0) {
            close(fd[1 - cur][0]);
            close(fd[1 - cur][1]);
        }
    }
}

int main() {
    // set default environment variables
    setenv("PATH", "bin:.", 1);
    string cmd, arg;
    // numbered pipe
    int line, fd_table[2000][2];
    deque<int> pid_table[2000];
    for (size_t i = 0; i < 2000; ++i) fd_table[i][0] = 0, fd_table[i][1] = 1;
    do {
        // prompt string
        cout << "% ";
        // EOF
        if (!getline(cin, cmd)) {
            cout << endl;
            break;
        }
        stringstream ss(cmd);
        ss >> cmd;
        if (cmd == "setenv") {
            // synopsis: setenv [environment variable] [value to assign]
            ss >> cmd >> arg;
            setenv(cmd.c_str(), arg.c_str(), 1);
        } else if (cmd == "printenv") {
            // synopsis: printenv [environment variable]
            ss >> arg;
            char *env = getenv(arg.c_str());
            if (env) cout << env << endl;
        } else if (cmd == "exit") {
            // synopsis: exit
            break;
        } else {
            if (cmd.size() == 0) continue;
            line = (line + 1) % 2000;
            /* mode
             0: stdout to overwrite file
             10: single line stdout pipe (default)
             20: stdout numbered pipe
             21: stdout stderr numbered pipe
            */
            int np = -1, mode = 10;
            // parse all commands and arguments
            vector<vector<string>> args;
            bool has_next = true;
            while (has_next) {
                has_next = false;
                vector<string> argv = {cmd};
                while (ss >> arg) {
                    if (arg == ">") {
                        mode = 0;
                        ss >> cmd;
                        break;
                    } else if (arg == "|") {
                        has_next = true;
                        ss >> cmd;
                        break;
                    } else if (arg[0] == '|' || arg[0] == '!') {
                        // assert(np > 0)
                        np = stoi(arg.substr(1, arg.size() - 1));
                        mode = 20 + (arg[0] == '!' ? 1 : 0);
                        break;
                    }
                    argv.push_back(arg);
                }
                args.emplace_back(argv);
            }
            // execute commands
            int nline = (line + np) % 2000;
            if ((mode == 20 || mode == 21) && !IS_PIPE(fd_table[nline][0]))
                pipe(fd_table[nline]);
            if (mode == 0)
                fd_table[nline][1] =
                    open(cmd.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (IS_PIPE(fd_table[line][1])) close(fd_table[line][1]);
            exec(args, pid_table[line], mode < 20 ? pid_table[line] : pid_table[nline],
                fd_table[line][0], fd_table[nline][1], mode);
            if (IS_PIPE(fd_table[line][0])) close(fd_table[line][0]);
            if (mode < 20) {
                for (int p : pid_table[line]) waitpid(p, NULL, 0);
            }
            // cleanup current line
            fd_table[line][0] = 0;
            fd_table[line][1] = 1;
            pid_table[line].clear();
        }
    } while (true);
}
