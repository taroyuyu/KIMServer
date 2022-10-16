#include <President/President.h>

int main(int argc,char * argv[]) {
    kakaIM::president::President president;
    if(president.init(argc,argv)){
        return president.start();
    }else{
        return -1;
    }
}
