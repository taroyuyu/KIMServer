//
// Created by taroyuyu on 2018/3/17.
//

#ifndef KAKAIMCLUSTER_MEM_UTIL_H
#define KAKAIMCLUSTER_MEM_UTIL_H
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
namespace kaka {
    class MemoryUsage{
    public:
        MemoryUsage():total(0),free(0),buffers(0),cached(0){

        }
        static std::pair<bool,MemoryUsage> getCurrentMemoryUsage(){
            MemoryUsage memoryUsage;

            std::ifstream cpuStatFile("/proc/meminfo");
            if (!cpuStatFile) {
                return std::make_pair(false, memoryUsage);
            }

            std::map<std::string,unsigned long long> infoMap;

            std::string record;
            while (std::getline(cpuStatFile, record),!record.empty()){
                record.erase(std::remove_if(record.begin(), record.end(), [](unsigned char x)->bool {return std::isspace(x);}), record.end());

                std::string recordName;
                unsigned long long recordValue;
                std::stringstream recordStream(record);

                int i = 0;
                for (std::string line;std::getline(recordStream, line, ':'),!line.empty();) {
                    ++i;
                    if(i == 1){
                        recordName = line;
                    }else if(i == 2){
                        recordValue = std::stoull(line);
                    }else{
                        break;
                    }
                }
                infoMap.insert(std::make_pair(recordName,recordValue));
            }
            auto itemIt = infoMap.find("MemTotal");
            if(itemIt == infoMap.end()){
                return std::make_pair(false, memoryUsage);
            }
            memoryUsage.total = itemIt->second;

            itemIt = infoMap.find("MemFree");
            if(itemIt == infoMap.end()){
                return std::make_pair(false, memoryUsage);
            }
            memoryUsage.free = itemIt->second;

            itemIt = infoMap.find("Buffers");
            if(itemIt == infoMap.end()){
                return std::make_pair(false, memoryUsage);
            }
            memoryUsage.buffers = itemIt->second;

            itemIt = infoMap.find("Cached");
            if(itemIt == infoMap.end()){
                return std::make_pair(false, memoryUsage);
            }
            memoryUsage.cached = itemIt->second;

            return std::make_pair(true, memoryUsage);
        }
        unsigned long long total;
        unsigned long long free;
        unsigned long long buffers;
        unsigned long long cached;

    };
}
#endif //KAKAIMCLUSTER_MEM_UTIL_H

