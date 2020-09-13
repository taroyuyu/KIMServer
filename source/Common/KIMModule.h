//
// Created by taroyuyu on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_KIMMODULE_H
#define KAKAIMCLUSTER_KIMMODULE_H

#include <thread>

namespace kakaIM {
    namespace common {
        class KIMModule {
        public:
            KIMModule(): m_isStarted(false)
            {

            }
	    /**
             * @description 对模块进行初始化
             * @return 模块初始化成功，则返回true,失败则返回false
             */
            virtual bool init() = 0;
            /**
            * @description 启动
            */
            virtual void start();

            /**
             * @description 停止
             */
            virtual void stop();

            /**
             * @description 重启
             */
            virtual void restart()
            {
                this->stop();
                this->start();
            }

        protected:
            virtual void execute() = 0;
        protected:
            std::thread m_workThread;
            bool m_isStarted;
        };
    }
}

#endif //KAKAIMCLUSTER_KIMMODULE_H
