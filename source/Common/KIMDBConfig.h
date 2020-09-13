//
// Created by taroyuyu on 2018/3/12.
//

#ifndef KAKAIMCLUSTER_KIMDBCONFIG_H
#define KAKAIMCLUSTER_KIMDBCONFIG_H
namespace kakaIM {
    namespace common {
        class KIMDBConfig{
        public:
            KIMDBConfig():port(0){

            }
            KIMDBConfig(std::string hostaddr,uint16_t port,std::string dbname,std::string userAccount,std::string userPassword):
                    hostaddr(hostaddr),port(port),dbname(dbname),userAccount(userAccount),userPassword(userPassword){

            }
            std::string getHostAddr()const{
                return this->hostaddr;
            }
            void setHostAddr(const std::string hostAddr){
                this->hostaddr = hostAddr;
            }
            uint16_t getPort()const{
                return this->port;
            }
            void setPort(uint16_t port){
                this->port = port;
            }
            std::string getDBName()const{
                return this->dbname;
            }
            void setDBName(const std::string dbName){
                this->dbname = dbName;
            }
            std::string getUserAccount()const{
                return this->userAccount;
            }
            void setUserAccount(const std::string userAccount){
                this->userAccount = userAccount;
            }
            std::string getUserPassword()const{
                return this->userPassword;
            }
            void setUserPassword(const std::string userPassword){
                this->userPassword = userPassword;
            }
        private:
            std::string hostaddr;
            uint16_t port;
            std::string dbname;
            std::string userAccount;
            std::string userPassword;

        };
    }
}
#endif //KAKAIMCLUSTER_KIMDBCONFIG_H

