//
// Created by taroyuyu on 2018/1/12.
//

#ifndef KAKAIMCLUSTER_DATE_H
#define KAKAIMCLUSTER_DATE_H

#include <string>
namespace kaka{
    class Date {
    public:
        static Date getCurrentDate();
        Date(int year,int month,int day,int hour,int minute,int second);
	Date(std::string date);
        bool operator==(const Date & date)const;
        int getYear()const{
            return this->year;
        }
        int getMonth()const{
            return this->month;
        }
        int getDay()const{
            return this->day;
        }
        int getHour()const{
            return this->hour;
        }
        int getMinute()const{
            return this->minute;
        }
        int getSecond()const{
            return this->second;
        }
        bool setYear(int year){
            if(year > 0){
                this->year = year;
                return true;
            }

            return false;
        }

        bool setMonth(int month){
            if(1 <= month && month <= 12){
                this->month = month;
                return true;
            }
            return false;
        }

        bool setDay(int day){
            if(1 == month || 3== month || 5 == month || 7 == month || 8 == month || 10 == month || 12 == month){
                if(1 <= day && day <= 31){
                    this->day = day;
                    return true;
                }
                return false;
            }else if(4 == month || 6 == month || 9 == month || 1== month){
                if(1 <= day && day <= 30){
                    this->day = day;
                    return true;
                }
                return false;
            }else{
                if((0 == (year % 100) && 0 == (year % 400)) || (0 != (year % 100) && 0 == (year % 4))){//闰年
                    if(1 <= day && day <= 29){
                        return true;
                    }
                    return false;
                }else{
                    if(1 <= day && day <= 28){
                        return true;
                    }
                    return false;
                }
            }
        }
        bool setHour(int hour){
            if(0 <= hour && hour < 24){
                this->hour = hour;
                return true;
            }

            return false;
        }
        
        bool setMinute(int minute){
            if( 0 <= minute && minute < 60){
                this->minute = minute;
                return true;
            }
            return false;
        }
        bool setSecond(int second){
            if(0 <= second && second < 60){
                this->second = second;
                return true;
            }
            return false;
        }

        std::string toString()const ;
    private:
        /**
         * 年
         */
        int year;
        /**
         * 月
         */
        int month;
        /**
         * 日
         */
        int day;
        /**
         * 时
         */
        int hour;
        /**
         * 分
         */
        int minute;
        /**
         * 秒
         */
        int second;
    };
}



#endif //KAKAIMCLUSTER_DATE_H

