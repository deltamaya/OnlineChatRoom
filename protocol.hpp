#pragma once
#include "utils.h"
#include <json/json.h>
using namespace std;

#define SEP "##"
constexpr size_t bufsize = 1024;
enum class ServiceCode:uint8_t
{
    postmsg,
    signup,
    login,
};
enum class StatusCode:uint8_t{
    ok,
    error
};

class Request
{
public:

    using enum ServiceCode;
    string user_;
    string msg_;
    ServiceCode servcode_;
    StatusCode stuscode_;
    Request():user_(),msg_(),servcode_(ServiceCode::postmsg),stuscode_(StatusCode::ok){

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
        ret["user"] = user_;
        ret["stuscode"] = static_cast<int>(stuscode_);
        ret["servcode"] = static_cast<int>(servcode_);
        ret["msg"] = msg_;
        auto retstr=writer.write(ret);
        retstr.pop_back();
        return retstr+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        user_=v["user"].asString();
        msg_=v["msg"].asString();
        servcode_=static_cast<ServiceCode>(v["servcode"].asInt());
        stuscode_=static_cast<StatusCode>(v["stuscode"].asInt());
    }
};

class Response
{
    public:
    using enum ServiceCode;
    string user_;
    string msg_;
    ServiceCode servcode_;
    StatusCode stuscode_;
    Response():user_(),msg_(),servcode_(ServiceCode::postmsg),stuscode_(StatusCode::ok){

    }
    static bool parse_response(string&str,Response* r){
        auto pos=str.find(SEP);
        if(pos!=string::npos){
            r->deserialize(str.substr(0,pos));
            str.erase(0,pos);
            return true;
        }
        return false;
    }
    string serialize()
    {
        Json::Value ret;
        Json::FastWriter writer;
        ret["user"] = user_;
        ret["stuscode"] = static_cast<int>(stuscode_);
        ret["servcode"] = static_cast<int>(servcode_);
        ret["msg"] = msg_;
        auto retstr=writer.write(ret);
        retstr.pop_back();
        return retstr+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        user_=v["user"].asString();
        msg_=v["msg"].asString();
        servcode_=static_cast<ServiceCode>(v["servcode"].asInt());
        stuscode_=static_cast<StatusCode>(v["stuscode"].asInt());
    }
};