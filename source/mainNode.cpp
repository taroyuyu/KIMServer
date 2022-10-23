//
// Created by Kakawater on 2018/1/9.
//
#include <Node/Node.h>
int main(int argc,char * argv[])
{
    auto node = kakaIM::node::Node::sharedNode();
    if(node->init(argc,argv)){
        return node->run();
    }else{
        return -1;
    }
}
