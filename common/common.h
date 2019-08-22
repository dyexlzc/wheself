#ifndef _COMMON_H
#define _COMMON_H
#include<string>
#include<iostream>
#include<vector>
using namespace std;
vector<string> str_tok(string _Src,string const &_Token){
    /**
     * 参数说明：
     * _Src：源字符串，要被处理的字符串
     * _Token:分隔符
     */
    vector<string> list;
    int size=_Src.size();
    for(int i=0;i<size;i++){
        cout<<_Src<<" ";
        int pos=_Src.find(_Token,0);
        if(pos!=string::npos){
            string tmp=_Src.substr(0,pos);
            //list.push_back(tmp);
            _Src.erase(0,tmp.size()+1);
            cout<<tmp<<endl;
        }
    }
    return list;
}



#endif