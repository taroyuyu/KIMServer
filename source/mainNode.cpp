//
// Created by Kakawater on 2018/1/9.
//
#include <Node/Node.h>
int main(int argc,char * argv[])
{
    kakaIM::node::Node node;
    if(node.init(argc,argv)){
        return node.start();
    }else{
        return -1;
    }
}
