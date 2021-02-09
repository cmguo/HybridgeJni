#include "jnimeta.h"
#include "jniclass.h"
#include "jnivariant.h"
#include <core/message.h>

#include <iostream>

template <typename Meta>
size_t findMeta(std::vector<Meta> const & metas, const Meta & meta)
{
    if (&meta >= &metas[0] && &meta < &metas.back())
        return std::size_t(&meta - &metas.front());
    for (size_t i = 0; i < metas.size(); ++i)
        if (meta == metas.at(i))
            return i;
    return size_t(-1);
}

JniMetaObject::JniMetaObject(JniMetaObject * super, jclass clazz)
    : super_(super)
    , clazz_(static_cast<jclass>(env()->NewGlobalRef(clazz)))
{
//    for (int i = 0; i < meta_.enumeratorCount(); ++i) {
//        metaEnums_.append(JniMetaEnum(meta_.enumerator(i)));
//    }

    JNIEnv * env = super->env();
    ClassClass & cc = classClass(env);
    MethodClass & mc = methodClass(env);
    FieldClass & fc = fieldClass(env);
    ModifierClass mfc = modifierClass(env);
    (void) fc;
    // class name
    className_ = cc.getName(clazz);
    std::cout << "JniMetaObject: " << className_ << std::endl;
    // fields
    std::vector<JniMetaProperty> metaProps;
    for (auto field : cc.getDeclaredFields(clazz)) {
        JLocalObjectRef lr(env, field);
        metaProps.emplace_back(JniMetaProperty(this, field));
    }
    // methods
    for (auto method : cc.getDeclaredMethods(clazz)) {
        JLocalObjectRef lr(env, method);
        int mod = mc.getModifiers(method);
        if (!mfc.isPublic(mod) || mfc.isStatic(mod) || mfc.isAbstract(mod))
            continue;
        JniMetaMethod m(this, env->NewGlobalRef(method));
        if (m.parameterCount() == 0 && strncmp(m.name(), "get", 3) == 0) {
            std::string name = m.name() + 3;
            if (!name.empty() && name[0] <= 'Z')
                name[0] += 'z' - 'Z';
            size_t ip = findMeta(metaProps, JniMetaProperty(name));
            if (ip < metaProps.size()) {
                metaProps[ip].setGetter(method);
                continue;
            }
        }
        if (m.parameterCount() == 1 && strncmp(m.name(), "set", 3) == 0) {
            std::string name = m.name() + 3;
            if (!name.empty() && name[0] <= 'Z')
                name[0] += 'z' - 'Z';
            size_t ip = findMeta(metaProps, JniMetaProperty(name));
            if (ip < metaProps.size()) {
                metaProps[ip].setSetter(method);
                continue;
            }
        }
        std::cout << "JniMetaObject: method: " << m.name() << std::endl;
        metaMethods_.emplace_back(std::move(m));
    }
    // props
    for (auto & prop : metaProps) {
        if (prop.isValid()) {
            std::cout << "JniMetaObject: property: " << prop.name() << std::endl;
            metaProps_.emplace_back(std::move(prop));
        }
    }
}

JniMetaObject::~JniMetaObject()
{
    if (clazz_)
        env()->DeleteGlobalRef(clazz_);
}

const char *JniMetaObject::className() const
{
    return className_.c_str();
}

size_t JniMetaObject::propertyCount() const
{
    return super_->propertyCount() + metaProps_.size();
}

const MetaProperty &JniMetaObject::property(size_t index) const
{
    size_t n = super_->propertyCount();
    return index < n ? super_->property(index) : metaProps_.at(index - n);
}

size_t JniMetaObject::methodCount() const
{
    return super_->methodCount() + metaMethods_.size();
}

const MetaMethod &JniMetaObject::method(size_t index) const
{
    size_t n = super_->methodCount();
    return index < n ? super_->method(index) : metaMethods_.at(index - n);
}

size_t JniMetaObject::enumeratorCount() const
{
    return super_->enumeratorCount() + metaEnums_.size();
}

const MetaEnum &JniMetaObject::enumerator(size_t index) const
{
    size_t n = super_->enumeratorCount();
    return index < n ? super_->enumerator(index) : metaEnums_.at(index - n);
}

bool JniMetaObject::connect(const Connection &c) const
{
    if (c.signalIndex() == 0)
        return true;
    return false;
}

bool JniMetaObject::disconnect(const Connection &c) const
{
    return c;
}

size_t JniMetaObject::metaIndexOf(void const *meta, MetaType type) const
{
    if (type == Property)
        return findMeta(metaProps_, *reinterpret_cast<JniMetaProperty const*>(meta))
                + super_->propertyCount();
    else if (type == Method)
        return findMeta(metaMethods_, *reinterpret_cast<JniMetaMethod const*>(meta))
                + super_->methodCount();
    else
        return findMeta(metaEnums_, *reinterpret_cast<JniMetaEnum const*>(meta))
                + super_->enumeratorCount();
}

JniMetaObject::JniMetaObject(JNIEnv *env)
    : env_(env)
    , clazz_(nullptr)
{

}

JniMetaProperty::JniMetaProperty(JniMetaObject *obj, jobject field)
    : obj_(obj)
    , field_(env()->NewGlobalRef(field))
{
    name_ = fieldClass().getName(field);
    std::cout << "JniMetaProperty: " << name_ << std::endl;
}

JniMetaProperty::JniMetaProperty(JniMetaProperty &&o)
    : obj_(o.obj_)
    , field_(o.field_)
    , name_(std::move(o.name_))
    , getter_(o.getter_)
    , setter_(o.setter_)
{
    o.obj_ = nullptr;
    o.field_ = nullptr;
    o.getter_ = nullptr;
    o.setter_ = nullptr;
}

JniMetaProperty::JniMetaProperty(const std::string &name)
    : obj_(nullptr)
    , field_(nullptr)
    , name_(name)
{
}

JniMetaProperty::~JniMetaProperty()
{
    if (setter_)
        env()->DeleteGlobalRef(setter_);
    if (getter_)
        env()->DeleteGlobalRef(getter_);
    if (field_)
        env()->DeleteGlobalRef(field_);
}

void JniMetaProperty::setSetter(jobject setter)
{
    setter_ = env()->NewGlobalRef(setter);
}

void JniMetaProperty::setGetter(jobject getter)
{
    getter_ = env()->NewGlobalRef(getter);
}

bool JniMetaProperty::operator==(const MetaProperty &o)
{
    return strcmp(name(), o.name()) == 0;
}

bool operator==(const JniMetaProperty &l, const JniMetaProperty &r)
{
    return strcmp(l.name(), r.name()) == 0;
}

const char *JniMetaProperty::name() const
{
    return name_.c_str();
}

bool JniMetaProperty::isValid() const
{
    if (obj_ == nullptr)
        return false;
    int mod = fieldClass().getModifiers(field_);
    return !modifierClass().isStatic(mod) && (getter_ || modifierClass().isPublic(mod));
}

Value::Type JniMetaProperty::type() const
{
    return Value::None;
}

bool JniMetaProperty::isConstant() const
{
    return false;
}

size_t JniMetaProperty::propertyIndex() const
{
    return obj_->metaIndexOf(this, JniMetaObject::Property);
}

bool JniMetaProperty::hasNotifySignal() const
{
    return false;
}

size_t JniMetaProperty::notifySignalIndex() const
{
    return size_t(-1);
}

const MetaMethod &JniMetaProperty::notifySignal() const
{
    static JniMetaMethod emptyMethod;
    return emptyMethod;
}

Value JniMetaProperty::read(const Object *object) const
{
    jobject jobj = static_cast<jobject>(const_cast<Object*>(object));
    if (getter_) {
        return JniVariant::toValue(methodClass().invoke(getter_, jobj, nullptr));
    }
    return JniVariant::toValue(fieldClass().get(field_, jobj));
}

bool JniMetaProperty::write(Object *object, Value &&value) const
{
    jobject jobj = static_cast<jobject>(object);
    if (setter_) {
        Array array;
        array.emplace_back(std::move(value));
        methodClass().invoke(setter_, jobj,
                             static_cast<jobjectArray>(JniVariant::fromValue(std::move(array))));
        return !JThrowable::clear(env());
    }
    fieldClass().set(field_, static_cast<jobject>(object), JniVariant::fromValue(value));
    return !JThrowable::clear(env());
}

JniMetaMethod::JniMetaMethod(JniMetaObject *obj, jobject method)
    : obj_(obj)
    , method_(method)
{
    JNIEnv * env = obj->env();
    if (method) {
        method_ = env->NewGlobalRef(method);
        name_ = methodClass().getName(method);
        returnType_ = JniVariant::type(methodClass().getReturnType(method));
        jobjectArray types = methodClass().getParameterTypes(method);
        int n = env->GetArrayLength(types);
        for (int i = 0; i < n; ++i) {
            JLocalClassRef t(env, static_cast<jclass>(env->GetObjectArrayElement(types, i)));
            paramTypes_.push_back(JniVariant::type(t));
        }
        std::cout << "JniMetaMethod: " << name_ << std::endl;
    } else {
        name_ = "destroyed";
    }
}

JniMetaMethod::JniMetaMethod(JniMetaMethod &&o)
    : obj_(o.obj_)
    , method_(o.method_)
    , name_(std::move(o.name_))
    , paramTypes_(std::move(o.paramTypes_))
{
    o.obj_ = nullptr;
    o.method_ = nullptr;
}

JniMetaMethod::~JniMetaMethod()
{
    if (method_)
        obj_->env()->DeleteGlobalRef(method_);
}

static std::vector<Value::Type> parameterTypes(const MetaMethod &o)
{
    std::vector<Value::Type> vec;
    for (size_t i = 0; i < o.parameterCount(); ++i)
        vec.push_back(o.parameterType(i));
    return vec;
}

bool JniMetaMethod::operator==(const MetaMethod &o)
{
    return strcmp(name(), o.name()) == 0
            && parameterCount() == o.parameterCount()
            && paramTypes_ == parameterTypes(o);
}

bool operator==(const JniMetaMethod &l, const JniMetaMethod &r)
{
    return strcmp(l.name(), r.name()) == 0
            && l.paramTypes_ == r.paramTypes_;
}

const char *JniMetaMethod::name() const
{
    return name_.c_str();
}

bool JniMetaMethod::isValid() const
{
    return obj_ != nullptr;
}

bool JniMetaMethod::isSignal() const
{
    return method_ == nullptr;
}

bool JniMetaMethod::isPublic() const
{
    return true;
}

size_t JniMetaMethod::methodIndex() const
{
    return obj_->metaIndexOf(this, JniMetaObject::Method);
}

const char *JniMetaMethod::methodSignature() const
{
    return nullptr;
}

Value::Type JniMetaMethod::returnType() const
{
    return returnType_;
}

size_t JniMetaMethod::parameterCount() const
{
    if (method_ == nullptr)
        return 0;
    return paramTypes_.size();
}

Value::Type JniMetaMethod::parameterType(size_t index) const
{
    return paramTypes_.at(index);
}

const char *JniMetaMethod::parameterName(size_t index) const
{
    static std::vector<std::string> names;
    for (size_t i = names.size(); i <= index; ++i)
        names.emplace_back(std::string("parameter") + stringNumber(index));
    return names.at(index).c_str();
}

bool JniMetaMethod::invoke(Object *object, Array &&args, const MetaMethod::Response &resp) const
{
    jobject returnValue = methodClass().invoke(method_, static_cast<jobject>(object),
           static_cast<jobjectArray>(JniVariant::fromValue(std::move(args))));
    resp(JniVariant::toValue(returnValue));
    return JThrowable::clear(obj_->env());
}

JniMetaEnum::JniMetaEnum(JniMetaObject *obj, jclass enumClass)
    : obj_(obj)
    , enumClass_(enumClass)
{
}

JniMetaEnum::JniMetaEnum(const std::string &name)
    : obj_(nullptr)
    , enumClass_(nullptr)
    , name_(name)
{
}

bool operator==(const JniMetaEnum &l, const JniMetaEnum &r)
{
    return strcmp(l.name(), r.name()) == 0;
}

const char *JniMetaEnum::name() const
{
    return name_.c_str();
}

size_t JniMetaEnum::keyCount() const
{
    return 0;
}

const char *JniMetaEnum::key(size_t index) const
{
    (void) index;
    return nullptr;
}

int JniMetaEnum::value(size_t index) const
{
    (void) index;
    return 0;
}

JniObjectMetaObject::JniObjectMetaObject(JNIEnv *env)
    : JniMetaObject(env)
{
    metaMethods_.emplace_back(JniMetaMethod(this));
}

const char *JniObjectMetaObject::className() const
{
    return "Object";
}

size_t JniObjectMetaObject::methodCount() const
{
    return metaMethods_.size();
}

const MetaMethod &JniObjectMetaObject::method(size_t index) const
{
    return metaMethods_.at(index);
}
