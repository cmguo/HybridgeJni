#include "jnimeta.h"
#include "jniclass.h"
#include "jnivariant.h"

#include <iostream>

template <typename Meta>
size_t findByName(std::vector<Meta> const & metas, const std::string &name)
{
    for (size_t i = 0; i < metas.size(); ++i)
        if (name == metas.at(i).name())
            return i;
    return size_t(-1);
}

JniMetaObject::JniMetaObject(JniMetaObjectBase * super, jclass clazz)
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
    // class name
    className_ = cc.getName(clazz);
    std::cout << "JniMetaObject: " << className_ << std::endl;
    // fields
    std::vector<JniMetaProperty> metaProps;
    for (auto field : cc.getDeclaredFields(clazz)) {
        JLocalRef lr(env, field);
        metaProps.emplace_back(JniMetaProperty(this, field));
    }
    // methods
    metaMethods_.push_back(JniMetaMethod(this));
    for (auto method : cc.getDeclaredMethods(clazz)) {
        JLocalRef lr(env, method);
        int mod = mc.getModifiers(method);
        if (!mfc.isPublic(mod) || mfc.isStatic(mod) || mfc.isAbstract(mod))
            continue;
        JniMetaMethod m(this, env->NewGlobalRef(method));
        if (m.parameterCount() == 0 && strncmp(m.name(), "get", 3) == 0) {
            std::string name = m.name() + 3;
            if (!name.empty() && name[0] <= 'Z')
                name[0] += 'z' - 'Z';
            size_t ip = findByName(metaProps, name);
            if (ip < metaProps.size()) {
                metaProps[ip].setGetter(method);
                continue;
            }
        }
        if (m.parameterCount() == 1 && strncmp(m.name(), "set", 3) == 0) {
            std::string name = m.name() + 3;
            if (!name.empty() && name[0] <= 'Z')
                name[0] += 'z' - 'Z';
            size_t ip = findByName(metaProps, name);
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
    return metaMethods_.size();
}

const MetaMethod &JniMetaObject::method(size_t index) const
{
    size_t n = super_->methodCount();
    return index < n ? super_->method(index) : metaMethods_.at(index - n);
}

size_t JniMetaObject::enumeratorCount() const
{
    return metaEnums_.size();
}

const MetaEnum &JniMetaObject::enumerator(size_t index) const
{
    size_t n = super_->enumeratorCount();
    return index < n ? super_->enumerator(index) : metaEnums_.at(index - n);
}

size_t JniMetaObject::metaIndexOf(const std::string &name, MetaType type) const
{
    if (type == Property)
        return findByName(metaProps_, name);
    else if (type == Method)
        return findByName(metaMethods_, name);
    else
        return findByName(metaEnums_, name);
}

size_t JniMetaObject::metaIndexOf(void const *meta, MetaType type) const
{
    if (type == Property)
        return static_cast<size_t>(static_cast<JniMetaProperty const*>(meta) - &metaProps_[0]);
    else if (type == Method)
        return static_cast<size_t>(static_cast<JniMetaMethod const*>(meta) - &metaMethods_[0]);
    else
        return static_cast<size_t>(static_cast<JniMetaEnum const*>(meta) - &metaEnums_[0]);
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

int JniMetaProperty::type() const
{
    return -1;
}

bool JniMetaProperty::isConstant() const
{
    return false;
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

bool JniMetaProperty::write(Object *object, const Value &value) const
{
    fieldClass().set(field_, static_cast<jobject>(object), JniVariant::fromValue(value));
    return true;
}

JniMetaMethod::JniMetaMethod(JniMetaObject *obj, jobject method)
    : obj_(obj)
    , method_(method)
{
    if (method) {
        method_ = obj->env()->NewGlobalRef(method);
        name_ = methodClass().getName(method);
        std::cout << "JniMetaMethod: " << name_ << std::endl;
    } else {
        name_ = "destroyed";
    }
}

JniMetaMethod::JniMetaMethod(JniMetaMethod &&o)
    : obj_(o.obj_)
    , method_(o.method_)
    , name_(std::move(o.name_))
{
    o.obj_ = nullptr;
    o.method_ = nullptr;
}

JniMetaMethod::~JniMetaMethod()
{
    if (method_)
        obj_->env()->DeleteGlobalRef(method_);
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

size_t JniMetaMethod::parameterCount() const
{
    return static_cast<size_t>(methodClass().getParameterCount(method_));
}

int JniMetaMethod::parameterType(size_t index) const
{
    (void) index;
    return -1;
}

const char *JniMetaMethod::parameterName(size_t index) const
{
    (void) index;
    return nullptr;
}

Value JniMetaMethod::invoke(Object *object, const Array &args) const
{
    jobject returnValue = methodClass().invoke(method_, static_cast<jobject>(object),
           static_cast<jobjectArray>(JniVariant::fromValue(Value::ref(const_cast<Array&>(args)))));
    return JniVariant::toValue(returnValue);
}

JniMetaEnum::JniMetaEnum(JniMetaObject *obj, jclass enumClass)
    : obj_(obj)
    , enumClass_(enumClass)
{
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
    : env_(env)
{
}

const MetaProperty &JniObjectMetaObject::property(size_t) const
{
    static JniMetaProperty p;
    return p;
}

const MetaMethod &JniObjectMetaObject::method(size_t) const
{
    static JniMetaMethod m;
    return m;
}

const MetaEnum &JniObjectMetaObject::enumerator(size_t) const
{
    static JniMetaEnum e;
    return e;
}
