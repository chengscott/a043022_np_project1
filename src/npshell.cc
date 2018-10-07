#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

void execute(int n, const vector<vector<string> >& args) {
    // convert c++ string to c string
    vector<char *> carg;
    auto it_end = args[n].end();
    for (auto it = args[n].begin(); it != it_end; ++it)
        carg.push_back(const_cast<char *>(it->c_str()));
    carg.push_back(NULL);
    if (n != args.size()) {
        // execute(n + 1, args);
    }
    exit(execvp(carg[0], &carg[0]));
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
            // parse all commands and arguments
            vector<vector<string> > args;
            bool has_next;
            do {
                has_next = false;
                vector<string> argv = { cmd };
                while (ss >> arg) {
                    if (arg == ">" || arg[0] == '|' || arg[0] == '!') {
                        has_next = true;
                        ss >> cmd;
                        break;
                    }
                    argv.push_back(arg);
                }
                args.emplace_back(argv);
            } while (has_next);
            // execute commands
            int pid;
            if ((pid = fork()) == 0) {
                execute(0, args);
            } else {
                int status;
                waitpid(pid, &status, 0);
                status = WEXITSTATUS(status);
                if (status == 255) {
                    cout << "Unknown command: [" << cmd << "]" << endl;
                }
            }
        }
    } while (true);
}
