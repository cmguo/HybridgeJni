#include "jniclass.h"
#include "jnitransport.h"

#include <core/value.h>

JniTransport::JniTransport(JNIEnv * env, jobject handle)
    : env_(env)
    , handle_(env_->NewWeakGlobalRef(handle))
{
    sendMessage_ = env->GetMethodID(env->GetObjectClass(handle), "sendMessage", "(Ljava/lang/String;)V");
}

JniTransport::~JniTransport()
{
    env_->DeleteGlobalRef(handle_);
}

void JniTransport::sendMessage(const Message &message)
{
    std::string json = Value::toJson(Value(const_cast<Message &>(message)));
    env_->CallVoidMethod(handle_, sendMessage_, env_->NewStringUTF(json.c_str()));
    JThrowable::check(env_);
}

void JniTransport::messageReceived(jstring message)
{
    Value v = Value::fromJson(JString(env_, message));
    Map emptyMap;
    return Transport::messageReceived(std::move(v.toMap(emptyMap)));
}
