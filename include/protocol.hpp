#pragma once
#include "utils.h"
#include <json/json.h>
using namespace std;

#define SEP "##"
constexpr size_t bufsize = 1024;
enum class ServiceCode:uint8_t
{
    postmsg, //to whom:gid
    signup,//uid: username/msg:pwd
    login,//uid:uid/msg:pwd
    query_uname,//uid:uid
    query_history,//to whom: gid/ msg:count
    cd,//to whom:cur/ gid msg:target gid
    create_group,//msg: group name
    join
};
enum class StatusCode:uint8_t{
    ok,
    error
};

class Request
{
public:

    using enum ServiceCode;
    string uid_;
    string msg_;
    ServiceCode service_;
    StatusCode status_;
    string to_whom_;
    Request():uid_(),msg_(),service_(ServiceCode::postmsg),status_(StatusCode::ok),to_whom_(){

    }
    static bool parse_request(string&str,Request* r){
        auto pos=str.find(SEP);
        if(pos!=string::npos){
            r->deserialize(str.substr(0,pos));
            str.erase(0,pos+strlen(SEP));
            return true;
        }
        return false;
    }
    string serialize()
    {
        Json::Value ret;
        Json::FastWriter writer;
        ret["uid"] = uid_;
        ret["status"] = static_cast<int>(status_);
        ret["service"] = static_cast<int>(service_);
        ret["msg"] = msg_;
        ret["towhom"]=to_whom_;
        auto retstr=writer.write(ret);
        retstr.pop_back();
        return retstr+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        uid_=v["uid"].asString();
        msg_=v["msg"].asString();
        to_whom_=v["towhom"].asString();
        service_=static_cast<ServiceCode>(v["service"].asInt());
        status_=static_cast<StatusCode>(v["status"].asInt());
    }
};

class Response
{
    public:
    using enum ServiceCode;
    string uid_;
    string msg_;
    ServiceCode service_;
    StatusCode status_;
    string to_whom_;
    Response():uid_(),msg_(),service_(ServiceCode::postmsg),status_(StatusCode::ok),to_whom_(){

    }
    static bool parse_response(string&str,Response* r){
        auto pos=str.find(SEP);
        if(pos!=string::npos){
            r->deserialize(str.substr(0,pos));
            str.erase(0,pos+strlen(SEP));
            return true;
        }
        return false;
    }
    string serialize()
    {
        Json::Value ret;
        Json::FastWriter writer;
        ret["uid"] = uid_;
        ret["status"] = static_cast<int>(status_);
        ret["service"] = static_cast<int>(service_);
        ret["msg"] = msg_;
        ret["towhom"]=to_whom_;
        auto retstr=writer.write(ret);
        retstr.pop_back();
        return retstr+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        uid_=v["uid"].asString();
        msg_=v["msg"].asString();
        to_whom_=v["towhom"].asString();
        service_=static_cast<ServiceCode>(v["service"].asInt());
        status_=static_cast<StatusCode>(v["status"].asInt());
    }
};