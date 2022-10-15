//
// Created by Kakawater on 2018/1/12.
//

#include <sstream>
#include <iostream>
#include <iomanip>
#include "Date.h"

namespace kaka {
    Date Date::getCurrentDate() {

        time_t timep;//系统时间
        struct tm *p;//格林尼治时间
        //获取系统时间
        time(&timep);
        //获取格林尼治时间
        p = gmtime(&timep);

        Date date(p->tm_year+1900, p->tm_mon+1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
//        free(p);//不要调用
        return date;

    }

    Date::Date(std::string date) {
        sscanf(date.c_str(),"%4d-%d-%d %d:%d:%d",&this->year,&this->month ,&this->day,&this->hour,&this->minute,&this->second);
    }
    Date::Date(int year, int month, int day, int hour, int minute, int second) :
            year(year), month(month), day(day), hour(hour), minute(minute), second(second) {
    }

    bool Date::operator==(const Date &date)const {
        if(this->second != date.second){
            return false;
        }
        if(this->minute != date.minute){
            return false;
        }
        if(this->hour != date.hour){
            return false;
        }
        if(this->day != date.day){
            return false;
        }
        if(this->month != date.month){
            return false;
        }
        if(this->year != date.year){
            return false;
        }
        return true;
    }

    std::string Date::toString() const{

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) <<year
            << "-" << std::setw(2) << month
            << "-" << std::setw(2) << day
            << " " << std::setw(2) << hour
            << ":" << std::setw(2) << minute
            << ":" << std::setw(2) << second;
        return oss.str();
    }
}
