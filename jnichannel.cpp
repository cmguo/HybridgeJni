#include "jnichannel.h"
#include "jnimeta.h"
#include "jniclass.h"

#include <algorithm>

JniChannel::JniChannel(JNIEnv * env, jobject handle)
    : env_(env)
    , handle_(env_->NewWeakGlobalRef(handle))
{
    jclass clazz = env->GetObjectClass(handle);
    createUuid_ = env->GetMethodID(clazz, "createUuid", "()Ljava/lang/String;");
    startTimer_ = env->GetMethodID(clazz, "startTimer", "(I)V");
    stopTimer_ = env->GetMethodID(clazz, "stopTimer", "()V");
    classMetas_.emplace("java.lang.Object", new JniObjectMetaObject(env));
}

JniChannel::~JniChannel()
{
    env_->DeleteWeakGlobalRef(handle_);
}

MetaObject *JniChannel::metaObject(const Object *object) const
{
    jobject jobj = static_cast<jobject>(const_cast<Object*>(object));
    jclass jcls = env_->GetObjectClass(jobj);
    return metaObject2(jcls);
}

std::string JniChannel::createUuid() const
{
    jobject uuid = env_->CallObjectMethod(handle_, createUuid_);
    return JString(env_, uuid);
}

MetaObject::Connection JniChannel::connect(const Object *object, size_t signalIndex)
{
    (void) object;
    (void) signalIndex;
    return MetaObject::Connection();
}

bool JniChannel::disconnect(const MetaObject::Connection &c)
{
    (void) c;
    return false;
}

void JniChannel::startTimer(int msec)
{
    env_->CallVoidMethod(handle_, startTimer_, msec);
}

void JniChannel::stopTimer()
{
    env_->CallVoidMethod(handle_, stopTimer_);
}

void JniChannel::registerObject(const std::string &id, jobject object)
{
    object = env_->NewWeakGlobalRef(object);
    Channel::registerObject(id, object);
    objects_.push_back(object);
}

void JniChannel::deregisterObject(jobject object)
{
    auto it = std::find_if(objects_.begin(), objects_.end(), JObjectFinder(env_, object));
    if (it != objects_.end()) {
        jobject o = *it;
        objects_.erase(it);
        Channel::deregisterObject(o);
    }
}

void JniChannel::propertyChanged(jobject object, jstring property)
{
    JniMetaObject * meta = static_cast<JniMetaObject*>(metaObject2(env_->GetObjectClass(object)));
    Channel::propertyChanged(object, meta->metaIndexOf(property, JniMetaObject::Property));
}

JniMetaObjectBase *JniChannel::metaObject2(jclass clazz) const
{
    std::string name = classClass(env_).getName(clazz);
    auto it = classMetas_.find(name);
    if (it == classMetas_.end()) {
        jclass super = classClass().getSuperclass(clazz);
        JniMetaObjectBase * smeta = metaObject2(super);
        JniMetaObject * meta = new JniMetaObject(smeta, clazz);
        it = classMetas_.insert(std::make_pair(name, meta)).first;
    }
    return it->second;
}
