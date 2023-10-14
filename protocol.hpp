#include "utils.h"
#include <json/json.h>
using namespace std;

#define SEP "\r\n\r\n"

enum class ServiceCode:int32_t
{
    post,
};

class Request
{
public:

    using enum ServiceCode;
    string user_;
    string msg_;
    ServiceCode code_;
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
        ret["code"] = static_cast<int>(code_);
        ret["msg"] = msg_;
        return writer.write(ret)+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        user_=v["user"].asString();
        msg_=v["msg"].asString();
        code_=static_cast<ServiceCode>(v["code"].asInt());
    }
};

class Response
{
    using enum ServiceCode;
    string user_;
    string msg_;
    ServiceCode code_;
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
        ret["code"] = static_cast<int>(code_);
        ret["msg"] = msg_;
        return writer.write(ret)+SEP;
    }
    void deserialize(const string& str){
        Json::Reader reader;
        Json::Value v;
        reader.parse(str,v);
        user_=v["user"].asString();
        msg_=v["msg"].asString();
        code_=static_cast<ServiceCode>(v["code"].asInt());
    }
};