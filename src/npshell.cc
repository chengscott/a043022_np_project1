#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
//#define DBG(x, ...) fprintf(stderr, x, __VA_ARGS__);
#define DBG(x, ...) 0;
using namespace std;

void convert(const vector<string> &from, vector<char *> &to) {
    // convert vector of c++ string to vector of c string
    auto it_end = from.end();
    for (auto it = from.begin(); it != it_end; ++it)
        to.push_back(const_cast<char *>(it->c_str()));
    to.push_back(NULL);
}

int exec(const vector<vector<string>> &args) {
    size_t i, cur;
    const size_t len = args.size();
    int status, pid[2], fd[2][4];
    for (i = 0; i < len; ++i) {
        cur = i & 1;
        pipe(&fd[cur][0]);
        if ((pid[cur] = fork()) == 0) {
            // fd[0] -> stdin
            if (i != 0) {
                DBG("#%d %d: fd[%d][0] -> stdin\n", __LINE__, i, 1 - cur);
                close(fd[1 - cur][1]);
                dup2(fd[1 - cur][0], 0);
            }
            // stdout -> fd[1]
            if (i != len - 1) {
                DBG("#%d %d: fd[%d][1] -> stdout\n", __LINE__, i, cur);
                close(fd[cur][0]);
                dup2(fd[cur][1], 1);
            } else {
                DBG("#%d %d: fd[%d] closed\n", __LINE__, i, cur);
                close(fd[cur][0]);
                close(fd[cur][1]);
            }
            vector<char *> arg;
            convert(args[i], arg);
            DBG("#%d %d: exec %s\n", __LINE__, i, arg[0]);
            exit(execvp(arg[0], &arg[0]));
        }
        if (i != 0) {
            DBG("#%d %d wait for %d\n", __LINE__, i, 1 - cur);
            waitpid(pid[1 - cur], &status, 0);
            close(fd[1 - cur][0]);
            close(fd[1 - cur][1]);
            DBG("#%d %d wait for %d is done\n", __LINE__, i, 1 - cur);
            if (WEXITSTATUS(status) == 255)
                cout << "Unknown command: [" << args[i - 1][0] << "]" << endl;
        }
    }
    DBG("#%d %d wait for %d\n", __LINE__, i, cur);
    waitpid(pid[cur], &status, 0);
    DBG("#%d %d wait for %d is done\n", __LINE__, i, cur);
    if (WEXITSTATUS(status) == 255)
        cout << "Unknown command: [" << args[i - 1][0] << "]" << endl;
    return 0;
}

int main() {
    // set default environment variables
    setenv("PATH", "bin:.", 1);
    string cmd, arg;
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
            // parse all commands and arguments
            vector<vector<string>> args;
            bool has_next = true;
            while (has_next) {
                has_next = false;
                vector<string> argv = {cmd};
                while (ss >> arg) {
                    if (arg == ">" || arg[0] == '|' || arg[0] == '!') {
                        has_next = true;
                        ss >> cmd;
                        break;
                    }
                    argv.push_back(arg);
                }
                args.emplace_back(argv);
            }
            // execute commands
            int pid;
            if ((pid = fork()) == 0) {
                exit(exec(args));
            } else {
                wait(NULL);
            }
        }
    } while (true);
}
