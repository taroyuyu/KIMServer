//
// Created by Kakawater on 2018/3/16.
//

#ifndef KAKAIMCLUSTER_CPU_UTIL_H
#define KAKAIMCLUSTER_CPU_UTIL_H

#include <fstream>
#include <string>
#include <sstream>
#include <regex>

namespace kaka {

    bool isNumber(std::string x);
    class CpuUsage {
    public:
        CpuUsage() : user(0), nice(0), system(0), idle(0), iowait(0), irq(0), softirq(0), stealstolen(0), guest(0),
                     other(0), total(0), work(0) {

        }

        static std::pair<bool, CpuUsage> getCurrentCPUUsage() {
            CpuUsage cpuUsage;
            std::ifstream cpuStatFile("/proc/stat");
            if (!cpuStatFile) {
                return std::make_pair(false, cpuUsage);
            }
            std::string record;
            std::getline(cpuStatFile, record);
            std::stringstream recordStream(record);
            int i = 0;
            for (std::string line; std::getline(recordStream, line, ' ');) {
                if (isNumber(line)) {
                    cpuUsage.total += std::stoull(line);
                    switch (++i) {
                        case 1: {
                            cpuUsage.user = std::stoull(line);
                        }
                            break;
                        case 2: {
                            cpuUsage.nice = std::stoull(line);
                        }
                            break;
                        case 3: {
                            cpuUsage.system = std::stoull(line);
                        }
                            break;
                        case 4: {
                            cpuUsage.idle = std::stoull(line);
                        }
                            break;
                        case 5: {
                            cpuUsage.iowait = std::stoull(line);
                        }
                            break;
                        case 6: {
                            cpuUsage.irq = std::stoull(line);
                        }
                            break;
                        case 7: {
                            cpuUsage.softirq = std::stoull(line);
                        }
                            break;
                        case 8: {
                            cpuUsage.stealstolen = std::stoull(line);
                        }
                            break;
                        case 9: {
                            cpuUsage.guest = std::stoull(line);
                        }
                            break;
                        default: {
                            cpuUsage.other = std::stoull(line);
                        }
                            break;
                    }
                }
            }

            cpuUsage.work = cpuUsage.total - cpuUsage.idle;
            return std::make_pair(true, cpuUsage);
        }

        unsigned long long user;
        unsigned long long nice;
        unsigned long long system;
        unsigned long long idle;
        unsigned long long iowait;
        unsigned long long irq;
        unsigned long long softirq;
        unsigned long long stealstolen;
        unsigned long long guest;
        unsigned long long other;
        unsigned long long total;
        unsigned long long work;//work = total - idle
    };

    bool isNumber(std::string x){
        std::regex e ("^-?\\d+");
        if (std::regex_match (x,e))
	     return true;
        else return false;
    }
}
#endif //KAKAIMCLUSTER_CPU_UTIL_H

