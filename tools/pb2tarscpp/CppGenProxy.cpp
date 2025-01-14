﻿// Generates C++ tars service interface out of Protobuf IDL.
//
// This is a Proto2 compiler plugin.  See net/proto2/compiler/proto/plugin.proto
// and net/proto2/compiler/public/plugin.h for more information on plugins.

#include <string>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/descriptor.h>

#include "CppGenCallback.h"

static std::string GenSyncCall(const ::google::protobuf::MethodDescriptor* method, const std::string& pkg, int indent) {
    std::string out;
    out.reserve(8 * 1024);

    out += ToCppNamespace(method->output_type()->full_name()) + " " + method->name() + "(const " +
           ToCppNamespace(method->input_type()->full_name()) + "& req, const std::map<std::string, std::string>& context = TARS_CONTEXT(), " +
           "std::map<std::string, std::string>* pResponseContext = NULL)";
    out += LineFeed(indent);
    out += "{" + LineFeed(++indent);
    out += "std::string _os;" + LineFeed(indent) +
           "req.SerializeToString(&_os);" + LineFeed(indent) +
           "std::vector<char> _vc(_os.begin(), _os.end());" + LineFeed(indent);
    out += LineFeed(indent);
    out += "std::map<std::string, std::string> _mStatus;" + LineFeed(indent);
    out += "shared_ptr<tars::ResponsePacket> rep = tars_invoke(tars::TARSNORMAL, \"" + method->name() + "\", _vc, context, _mStatus);" + LineFeed(indent);
    out += "if (pResponseContext)" + LineFeed(++indent);
    out += "*pResponseContext = rep->context;" + LineFeed(--indent);

    out += LineFeed(indent);
    out += ToCppNamespace(method->output_type()->full_name()) + " _ret;" + LineFeed(indent);
    out += "_ret.ParseFromArray(rep->sBuffer.data(), rep->sBuffer.size());" + LineFeed(indent) + 
           "return _ret;";
    out += LineFeed(--indent) + "}";
    out += LineFeed(indent);
    out += LineFeed(indent);

    return out;
}

static std::string GenAsyncCall(const ::google::protobuf::MethodDescriptor* method,
                                const std::string& name,
                                const std::string& pkg,
                                int indent) {
    std::string out;
    out.reserve(8 * 1024);

    out += "void async_" + method->name() + "(" + name + "PrxCallbackPtr callback, const " +
           ToCppNamespace(method->input_type()->full_name()) + "& req, const std::map<std::string, std::string>& context = TARS_CONTEXT())" + LineFeed(indent);
    out += "{" + LineFeed(++indent);
    out += "std::string _os;" + LineFeed(indent) +
           "req.SerializeToString(&_os);" + LineFeed(indent) +
           "std::vector<char> _vc(_os.begin(), _os.end());" + LineFeed(indent);
    out += "std::map<std::string, std::string> _mStatus;" + LineFeed(indent);
    out += "tars_invoke_async(tars::TARSNORMAL, \"" + method->name() + "\", _vc, context, _mStatus, callback);";
    out += LineFeed(--indent) + "}";
    out += LineFeed(indent);
    out += LineFeed(indent);

    return out;
}

static std::string GenCoroCall(const ::google::protobuf::MethodDescriptor* method,
                                const std::string& name,
                                const std::string& pkg,
                                int indent) {
    std::string out;
    out.reserve(8 * 1024);

    out += "void coro_" + method->name() + "(" + name + "CoroPrxCallbackPtr callback, const " +
           ToCppNamespace(method->input_type()->full_name()) + "& req, const std::map<std::string, std::string>& context = TARS_CONTEXT())" + LineFeed(indent);
    out += "{" + LineFeed(++indent);
    out += "std::string _os;" + LineFeed(indent) +
           "req.SerializeToString(&_os);" + LineFeed(indent) +
           "std::vector<char> _vc(_os.begin(), _os.end());" + LineFeed(indent);
    out += "std::map<std::string, std::string> _mStatus;" + LineFeed(indent);
    out += "tars_invoke_async(tars::TARSNORMAL, \"" + method->name() + "\", _vc, context, _mStatus, callback, true);";
    out += LineFeed(--indent) + "}";
    out += LineFeed(indent);

    return out;
}

std::string GenPrx(const ::google::protobuf::ServiceDescriptor* desc, int indent) {
    std::string out;
    out.reserve(8 * 1024);

    const auto& name = desc->name();
    const auto& pkg = desc->file()->package();
    const auto& proxy = name + "Proxy";

    out += LineFeed(indent);
    out += "/* proxy for client */";
    out += LineFeed(indent);
    out += "class " + proxy + " : public tars::ServantProxy";
    out += LineFeed(indent);
    out += "{";
    out += LineFeed(indent);
    out += "public:";
    out += LineFeed(++indent);
    out += "typedef std::map<std::string, std::string> TARS_CONTEXT;" + LineFeed(indent);

    //sort by method name
	std::map<std::string, const ::google::protobuf::MethodDescriptor*> m_method;
	for (int i = 0; i < desc->method_count(); ++i)
	{
		m_method[desc->method(i)->name()] = desc->method(i);
	}

	// gen methods
	for(auto it = m_method.begin(); it != m_method.end(); ++it)
	{
		auto method = it->second;
		out += LineFeed(indent);
		// sync method call
		out += GenSyncCall(method, pkg, indent);
		// async method call
		out += GenAsyncCall(method, name, pkg, indent);
              // coro method call
              out += GenCoroCall(method, name, pkg, indent);
	}


    // hash call
    out += LineFeed(indent);
    out += proxy + "* tars_hash(int64_t key)";
    out += "{" + LineFeed(++indent);
    out += "return (" + proxy + "*)ServantProxy::tars_hash(key);";
    out += LineFeed(--indent) + "}";
    out += LineFeed(indent);

    // consist hash
    out += LineFeed(indent);
    out += proxy + "* tars_consistent_hash(int64_t key)";
    out += "{" + LineFeed(++indent);
    out += "return (" + proxy + "*)ServantProxy::tars_consistent_hash(key);";
    out += LineFeed(--indent) + "}";
    out += LineFeed(indent);

    // set_timeout
    out += LineFeed(indent);
    out += proxy + "* tars_set_timeout(int msecond)";
    out += "{" + LineFeed(++indent);
    out += "return (" + proxy + "*)ServantProxy::tars_set_timeout(msecond);";
    out += LineFeed(--indent) + "}";

    out += LineFeed(--indent) + "}; // end class " + proxy;
    out += LineFeed(indent); // end class

    out += LineFeed(indent);
    out += "typedef tars::TC_AutoPtr<" + proxy + "> " + name + "Prx;";
    out += LineFeed(indent);

    return out;
}

