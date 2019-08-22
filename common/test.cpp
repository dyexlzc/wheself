#include<iostream>
#include<vector>
#include "common.h"
using namespace std;
int main(){
//a,,1,"b,"""
    string p;
    cin>>p;
    vector<string> list=str_tok(p,",");
    vector<string>::iterator it;
    for(it=list.begin();it<list.end();it++){
        cout<<*it<<endl;
    }
    return 0;
}