#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
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

int exec(const vector<vector<string>> &args, const vector<int> &pidin, int fdin,
         int fdout) {
    size_t i, cur;
    const size_t len = args.size();
    int status, pid[2], fd[2][2];
    for (i = 0; i < len; ++i) {
        cur = i & 1;
        if (i != len - 1) pipe(fd[cur]);
        if ((pid[cur] = fork()) == 0) {
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
            }
            vector<char *> arg;
            convert(args[i], arg);
            DBG("#%d %zu: exec %s\n", __LINE__, i, arg[0]);
            exit(execvp(arg[0], &arg[0]));
        }
        if (i != 0) {
            DBG("#%d %zu wait for %zu (%d)\n", __LINE__, i, 1 - cur,
                pid[1 - cur]);
            waitpid(pid[1 - cur], &status, 0);
            DBG("#%d %zu wait for %zu (%d) is done\n", __LINE__, i, 1 - cur,
                pid[1 - cur]);
            close(fd[1 - cur][0]);
            close(fd[1 - cur][1]);
            if (WEXITSTATUS(status) == 255)
                cout << "Unknown command: [" << args[i - 1][0] << "]" << endl;
        } else {
            for (int pid : pidin) {
                DBG("#%d %zu wait for (%d)\n", __LINE__, i, pid);
                waitpid(pid, NULL, 0);
                DBG("#%d %zu wait for (%d) is done\n", __LINE__, i, pid);
            }
            if (IS_PIPE(fdin)) close(fdin);
        }
    }
    if (IS_PIPE(fdout)) close(fdout);
    DBG("#%d %zu wait for %zu (%d)\n", __LINE__, i, cur, pid[cur]);
    waitpid(pid[cur], &status, 0);
    DBG("#%d %zu wait for %zu (%d) is done\n", __LINE__, i, cur, pid[cur]);
    if (WEXITSTATUS(status) == 255)
        cout << "Unknown command: [" << args[i - 1][0] << "]" << endl;
    return 0;
}

int main() {
    // set default environment variables
    setenv("PATH", "bin:.", 1);
    string cmd, arg;
    // numbered pipe
    int line, fd_table[2000][2];
    vector<int> pid_table[2000];
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
            int np = -1;
            bool out_err;
            // parse all commands and arguments
            vector<vector<string>> args;
            bool has_next = true;
            while (has_next) {
                has_next = false;
                vector<string> argv = {cmd};
                while (ss >> arg) {
                    if (arg == ">") {
                        ss >> cmd;
                        break;
                    } else if (arg == "|") {
                        has_next = true;
                        ss >> cmd;
                        break;
                    } else if (arg[0] == '|' || arg[0] == '!') {
                        np = stoi(arg.substr(1, arg.size() - 1));
                        out_err = (arg[0] == '!');
                        break;
                    }
                    argv.push_back(arg);
                }
                args.emplace_back(argv);
            }
            // execute commands
            int nline = (line + np) % 2000;
            if (np != -1 && !IS_PIPE(fd_table[nline][0])) pipe(fd_table[nline]);
            int pid;
            if ((pid = fork()) == 0) {
                if (IS_PIPE(fd_table[line][1])) close(fd_table[line][1]);
                if (IS_PIPE(fd_table[nline][0])) close(fd_table[nline][0]);
                exec(args, pid_table[line], fd_table[line][0],
                     fd_table[nline][1]);
                exit(0);
            }
            if (IS_PIPE(fd_table[line][0])) close(fd_table[line][0]);
            if (IS_PIPE(fd_table[line][1])) close(fd_table[line][1]);
            if (np == -1) {
                DBG("#%d npshell wait for (%d)\n", __LINE__, pid);
                waitpid(pid, NULL, 0);
                DBG("#%d npshell wait for (%d) is done\n", __LINE__, pid);
            } else {
                pid_table[nline].push_back(pid);
            }
            // cleanup current line
            fd_table[line][0] = 0;
            fd_table[line][1] = 1;
            pid_table[line].clear();
        }
    } while (true);
}
