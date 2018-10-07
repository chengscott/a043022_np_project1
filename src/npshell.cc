#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

int main() {
    setenv("PATH", "bin:.", 1);
    string cmd;
    do {
        cout << "% ";
        if (!getline(cin, cmd)) {
            cout << endl;
            break;
        }
        stringstream ss(cmd);
        ss >> cmd;
        if (cmd == "setenv") {
           string val;
           ss >> cmd >> val;
           setenv(cmd.c_str(), val.c_str(), 1);
        } else if (cmd == "printenv") {
            ss >> cmd;
            char *env = getenv(cmd.c_str());
            if (env) cout << env << endl;
        } else if (cmd == "exit") {
            break;
        } else {
            vector<char *> cargs;
            string arg;
            cargs.push_back(const_cast<char*>(cmd.c_str()));
            while (ss >> arg) {
                cargs.push_back(const_cast<char*>(arg.c_str()));
            }
            cargs.push_back(NULL);
            int pid;
            if ((pid = fork()) == 0) {
                exit(execvp(cmd.c_str(), &cargs[0]));
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
