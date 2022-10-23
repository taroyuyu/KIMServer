#include <President/President.h>

int main(int argc,char * argv[]) {
    auto president = kakaIM::president::President::sharedPresident();
    if(president->init(argc,argv)){
        return president->run();
    }else{
        return -1;
    }
}
