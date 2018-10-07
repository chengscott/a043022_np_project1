#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
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
        }
    } while (true);
    /*
    char* env_p = std::getenv("PATH");
    cout << env_p << endl;
    vector<string> args = {"cat", "test.html"};
    vector<char *> cargs;
    for (auto it = args.begin(); it != args.end(); ++it)
        cargs.push_back(const_cast<char*>(it->c_str()));
    cargs.push_back(NULL);
    //char *args[3] = {"cat", "test.html", NULL};
    cout << cargs.size() << endl;
    execvp("cat", &cargs[0]);
    return 0;*/
}
