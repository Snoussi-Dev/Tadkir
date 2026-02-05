#include <libnotify/notify.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <string>
#include <pwd.h>
#include <vector>
#include <ctime>

const std::string PID_FILE = "/tmp/tadkir.pid";
std::string GLOBAL_CONFIG_PATH;

std::string get_default_path() {
    const char *homedir = getenv("HOME");
    if (homedir == nullptr) homedir = getpwuid(getuid())->pw_dir;
    return std::string(homedir) + "/Documents/tadkir.conf";
}

bool is_running() {
    std::ifstream file(PID_FILE);
    if (!file.is_open()) return false;
    pid_t pid;
    file >> pid;
    return (kill(pid, 0) == 0);
}

void stop_daemon() {
    std::ifstream file(PID_FILE);
    if (file.is_open()) {
        pid_t pid;
        file >> pid;
        kill(pid, SIGTERM);
        unlink(PID_FILE.c_str());
        std::cout << "Tadkir stopped." << std::endl;
    } else {
        std::cerr << "Tadkir is not running." << std::endl;
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        std::ofstream file(PID_FILE);
        file << pid;
        file.close();
        exit(EXIT_SUCCESS);
    }

    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [start|stop|status] [optional_path]" << std::endl;
        return 1;
    }

    std::string cmd = argv[1];
    GLOBAL_CONFIG_PATH = (argc >= 3) ? argv[2] : get_default_path();

    if (cmd == "status") {
        if (is_running()) {
            std::ifstream file(PID_FILE);
            pid_t pid;
            file >> pid;
            std::cout << "Tadkir is running (PID: " << pid << ")." << std::endl;
        } else {
            std::cout << "Tadkir is stopped." << std::endl;
        }
        return 0;
    }

    if (cmd == "stop") {
        stop_daemon();
        return 0;
    }

    if (cmd == "start") {
        if (is_running()) {
            std::cout << "Already running." << std::endl;
            return 1;
        }

        std::ifstream check_conf(GLOBAL_CONFIG_PATH);
        if (!check_conf.is_open()) {
            std::string input_time;
            std::cout << "Config not found at: " << GLOBAL_CONFIG_PATH << std::endl;
            std::cout << "Enter interval in seconds (default 200): ";
            std::getline(std::cin, input_time);
            
            std::ofstream conf(GLOBAL_CONFIG_PATH);
            conf << (input_time.empty() ? "200" : input_time) << "\nسبحان الله\nالحمد لله\nلا إله إلا الله\nالله أكبر";
            conf.close();
        }

        daemonize();
    } else {
        return 1;
    }

    srand(time(nullptr));
    notify_init("Tadkir");

    while (true) {
        unsigned int interval = 200;
        std::vector<std::string> messages;
        
        std::ifstream conf(GLOBAL_CONFIG_PATH);
        if (conf.is_open()) {
            std::string line;
            if (conf >> interval) {
                std::getline(conf, line);
                while (std::getline(conf, line)) {
                    if (!line.empty()) messages.push_back(line);
                }
            }
            conf.close();
        }

        if (messages.empty()) {
            messages.push_back("سبحان الله، والحمد لله، ولا إله إلا الله، والله أكبر");
        }

        std::string random_msg = messages[rand() % messages.size()];

        NotifyNotification *n = notify_notification_new("سبح", random_msg.c_str(), "dialog-information");
        notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
        notify_notification_show(n, nullptr);
        g_object_unref(G_OBJECT(n));

        sleep(interval);
    }

    notify_uninit();
    return 0;
}