#include "jnivariant.h"
#include "jniclass.h"

#include <algorithm>

struct JniConverter : public Class
{
    JniConverter(JNIEnv *env, char const * className = nullptr)
        : Class(env, className)
    {
    }
    virtual Value toValue(jobject object) = 0;
    virtual jobject fromValue(Value const & value) = 0;
};

struct BoxConverter : JniConverter
{
    BoxConverter(JNIEnv *env, char const * className, char const * valueMethod,
             char const * valueSignature, char const * initSignature)
        : JniConverter(env, className)
    {
        unbox_ = env->GetMethodID(clazz_, valueMethod, valueSignature);
        box_ = env_->GetMethodID(clazz_, "<init>", initSignature);
        JThrowable::check(env);
    }
    Value toArrayValue(jobject jarray)
    {
        int n = getArrayLength(jarray);
        void * array = getArrayElements(jarray);
        Array varray;
        for (int i = 0; i < n; ++i) {
            varray.emplace_back(getArrayValue(array, i));
        }
        releaseArrayElements(jarray, array, false);
        return std::move(varray);
    }
    jobject fromArrayValue(Value const & value)
    {
        Array const & varray = value.toArray();
        int n = static_cast<int>(varray.size());
        jobject jarray = newArray(n);
        void * array = getArrayElements(jarray);
        for (int i = 0; i < n; ++i) {
            setArrayValue(array, i, varray[static_cast<size_t>(i)]);
        }
        releaseArrayElements(jarray, array, true);
        return jarray;
    }
    int getArrayLength(jobject object) { return env_->GetArrayLength(static_cast<jarray>(object)); }
    virtual void * getArrayElements(jobject jarrry) = 0;
    virtual Value getArrayValue(void * array, int index) = 0;
    virtual jobject newArray(int length) = 0;
    virtual void setArrayValue(void * array, int index, Value const & value) = 0;
    virtual void releaseArrayElements(jobject jarrry, void * array, bool commit) = 0;
protected:
    jmethodID unbox_;
    jmethodID box_;
};

#define DEFINE_BOX_CONVERTER(boxType, primitiveType, typeSignature) \
    struct boxType ## Converter : BoxConverter \
{ \
    typedef j ## primitiveType JElem; \
    typedef j ## primitiveType ## Array JArray; \
    boxType ## Converter(JNIEnv *env) : BoxConverter(env, "java/lang/" #boxType, \
            #primitiveType "Value", "()" #typeSignature, "(" #typeSignature ")V") {} \
    virtual Value toValue(jobject object) override { \
            return env_->Call ## boxType ## Method(object, unbox_); } \
    virtual jobject fromValue(Value const & value) override { \
            return env_->NewObject(clazz_, box_, static_cast<JElem>(value.to ## boxType ())); } \
    virtual void * getArrayElements(jobject jarray) override { \
            return env_->Get ## boxType ## ArrayElements(static_cast<JArray>(jarray), nullptr); } \
    virtual Value getArrayValue(void * array, int index) override { \
            return static_cast<JElem*>(array)[index]; } \
    virtual jarray newArray(int length) override { return env_->New ## boxType ## Array(length); } \
    virtual void setArrayValue(void * array, int index, Value const & value) override { \
            JElem v = static_cast<JElem>(value.to ## boxType ()); \
            static_cast<JElem*>(array)[index] = v; } \
    virtual void releaseArrayElements(jobject jarray, void * array, bool commit) override { \
            env_->Release ## boxType ## ArrayElements(static_cast<JArray>(jarray), static_cast<JElem*>(array), commit ? 0 : JNI_ABORT); } \
};

struct StringConverter : JniConverter
{
    StringConverter(JNIEnv *env) : JniConverter(env, "java/lang/String") {}
    virtual Value toValue(jobject object) override {
        return static_cast<std::string>(JString(env_, static_cast<jstring>(object)));
    }
    virtual jobject fromValue(Value const & value) override {
        return env_->NewStringUTF(value.toString().c_str());
    }
};

// handle jobjectArray only
struct ArrayConverter : JniConverter
{
    ArrayConverter(JNIEnv *env) : JniConverter(env) {}
    virtual Value toValue(jobject object) override {
        jobjectArray array = static_cast<jobjectArray>(object);
        Array array2;
        int n = env_->GetArrayLength(array);
        for (int i = 0; i < n; ++i) {
            JLocalObjectRef object(env_, env_->GetObjectArrayElement(array, i));
            array2.emplace_back(JniVariant::toValue(object));
        }
        return std::move(array2);
    }
    virtual jobject fromValue(Value const & value) override {
        Array const & varray = value.toArray();
        int n = static_cast<int>(varray.size());
        jobjectArray jarray = env_->NewObjectArray(n, classClass().objectClass(), nullptr);
        for (int i = 0; i < n; ++i) {
            env_->SetObjectArrayElement(jarray, i, JniVariant::fromValue(varray[static_cast<size_t>(i)]));
        }
        return jarray;
    }
};

struct ArrayListClass : Class
{
    ArrayListClass(JNIEnv * env)
        : Class(env, "java/lang/ArrayList")
    {
        add_ = env->GetMethodID(clazz_, "add", "(Ljava/lang/Object;)Z");
    }
    void add(jobject iterator, jobject entry) { env_->CallVoidMethod(iterator, add_, entry); }
    jmethodID add_;
};

struct IterableConverter : JniConverter
{
    IterableConverter(JNIEnv *env) : JniConverter(env, "java/lang/Iterable"), iteratorConverter_(env) {
        iterator_ = env->GetMethodID(clazz_, "iterator", "()Ljava/util/Iterator;");
    }
    jobject iterater(jobject object) {
        return env_->CallObjectMethod(object, iterator_);
    }
    struct IteratorConverter;
    IteratorConverter & iteratorConverter() {
        return iteratorConverter_;
    }
    virtual Value toValue(jobject object) override {
        Array varray;
        jobject iterater = this->iterater(object);
        while (iteratorConverter_.hasNext(iterater)) {
            JLocalObjectRef o(env_, iteratorConverter_.next(iterater));
            varray.emplace_back(JniVariant::toValue(o));
        }
        return std::move(varray);
    }
    virtual jobject fromValue(Value const & value) override {
        Array const & varray = value.toArray();
        int n = static_cast<int>(varray.size());
        static ArrayListClass alc(env_);
        jobject jlist = alc.newInstance();
        for (int i = 0; i < n; ++i) {
            JLocalObjectRef item(env_, JniVariant::fromValue(varray[static_cast<size_t>(i)]));
            alc.add(jlist, item);
        }
        return jlist;
    }
public:
    struct IteratorConverter : JniConverter
    {
        IteratorConverter(JNIEnv *env) : JniConverter(env, "java/util/Iterator") {
            hasNext_ = env->GetMethodID(clazz_, "hasNext", "()Z");
            next_ = env->GetMethodID(clazz_, "next", "()Ljava/lang/Object;");
        }
        bool hasNext(jobject iterator) { return env_->CallBooleanMethod(iterator, hasNext_); }
        jobject next(jobject iterator) { return env_->CallObjectMethod(iterator, next_); }
        virtual Value toValue(jobject) override { return Value(); }
        virtual jobject fromValue(const Value &) override { return nullptr; }
    private:
        jmethodID hasNext_;
        jmethodID next_;
    };
protected:
    jmethodID iterator_;
    IteratorConverter iteratorConverter_;
};

struct TreeMapClass : Class
{
    TreeMapClass(JNIEnv * env)
        : Class(env, "java/util/TreeMap")
    {
        put_ = env->GetMethodID(clazz_, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    }
    void put(jobject map, jobject key, jobject entry) { env_->CallObjectMethod(map, put_, key, entry); }
    jmethodID put_;
};

struct MapConverter : JniConverter
{
    MapConverter(JNIEnv *env, IterableConverter * iterable) : JniConverter(env, "java/util/Map"),
            iterableConverter_(iterable), entryConverter_(env) {
        entrySet_ = env->GetMethodID(clazz_, "entrySet", "()Ljava/util/Set;");
    }
    virtual Value toValue(jobject object) override {
        Map map;
        jobject set = env_->CallObjectMethod(object, entrySet_);
        jobject iterater = iterableConverter_->iterater(set);
        IterableConverter::IteratorConverter & iteratorConverter = iterableConverter_->iteratorConverter();
        ClassClass & cc = classClass();
        while (iteratorConverter.hasNext(iterater)) {
            jobject entry = iteratorConverter.next(iterater);
            JLocalObjectRef key(env_, entryConverter_.getKey(entry));
            JLocalObjectRef value(env_, entryConverter_.getValue(entry));
            map.emplace(cc.toString(key), JniVariant::toValue(value));
        }
        return std::move(map);
    }
    virtual jobject fromValue(Value const & value) override {
        Map const & map = value.toMap();
        static TreeMapClass tmc(env_);
        jobject jmap = tmc.newInstance();
        for (auto & it : map) {
            JLocalObjectRef key(env_, env_->NewStringUTF(it.first.c_str()));
            JLocalObjectRef value(env_, JniVariant::fromValue(it.second));
            tmc.put(jmap, key, value);
        }
        return jmap;
    }
private:
    struct EntryConverter : JniConverter
    {
        EntryConverter(JNIEnv *env) : JniConverter(env, "java/util/Map$Entry") {
            getKey_ = env->GetMethodID(clazz_, "getKey", "()Ljava/lang/Object;");
            getValue_ = env->GetMethodID(clazz_, "getValue", "()Ljava/lang/Object;");
        }
        jobject getKey(jobject entry) { return env_->CallObjectMethod(entry, getKey_); }
        jobject getValue(jobject entry) { return env_->CallObjectMethod(entry, getValue_); }
        virtual Value toValue(jobject) override { return Value(); }
        virtual jobject fromValue(const Value &) override { return nullptr; }
    private:
        jmethodID getKey_;
        jmethodID getValue_;
    };
    jmethodID entrySet_;
    IterableConverter * iterableConverter_;
    EntryConverter entryConverter_;
};

#define toBoolean toBool
#define GetBooleanArrayElement GetBoolArrayElement
DEFINE_BOX_CONVERTER(Boolean, boolean, Z)
#undef toBoolean
#define toByte toInt
DEFINE_BOX_CONVERTER(Byte, byte, B)
#undef toByte
#define toCharacter toInt
#define CallCharacterMethod CallIntMethod
#define GetCharacterArrayElements GetCharArrayElements
#define NewCharacterArray NewCharArray
#define ReleaseCharacterArrayElements ReleaseCharArrayElements
DEFINE_BOX_CONVERTER(Character, char, C)
#undef ReleaseIntegerArrayElements
#undef NewCharacterArray
#undef GetCharacterArrayElements
#undef CallCharacterMethod
#undef toChar
#define toShort toInt
DEFINE_BOX_CONVERTER(Short, short, S)
#undef toShort
#define CallIntegerMethod CallIntMethod
#define toInteger toInt
#define GetIntegerArrayElements GetIntArrayElements
#define NewIntegerArray NewIntArray
#define ReleaseIntegerArrayElements ReleaseIntArrayElements
DEFINE_BOX_CONVERTER(Integer, int, I)
#undef ReleaseIntegerArrayElements
#undef NewIntegerArray
#undef GetIntegerArrayElements
#undef toInteger
#undef CallIntergerMethod
DEFINE_BOX_CONVERTER(Long, long, J)
DEFINE_BOX_CONVERTER(Float, float, F)
DEFINE_BOX_CONVERTER(Double, double, D)

struct Converters {
    enum {
        Boolean, Byte, Character, Short, Integer, Long, Float, Double, String,
        Array, Iterable, Map
    };
};

static JniConverter * classes[12]; // last three: Array, Iterable, Map

void JniVariant::init(JNIEnv *env)
{
    auto iterableConverter = new IterableConverter(env);
    JniConverter * classes2[] = {
        new BooleanConverter(env),
        new ByteConverter(env),
        new CharacterConverter(env),
        new ShortConverter(env),
        new IntegerConverter(env),
        new LongConverter(env),
        new FloatConverter(env),
        new DoubleConverter(env),
        new StringConverter(env),
        new ArrayConverter(env),
        iterableConverter,
        new MapConverter(env, iterableConverter),
    };
    JThrowable::check(env);
    std::copy(classes2, classes2 + 12, classes);
}

Value::Type JniVariant::type(jclass type)
{
    ClassClass & ci = classClass();
    if (ci.isArray(type)) {
        return Value::Array_;
    }
    JniConverter ** pjc = std::find_if(classes, classes + 9, [type] (JniConverter * jc) {
        return *jc == type;
    });
    if (pjc < classes + Converters::Array) {
        int n = static_cast<int>(pjc - classes);
        if (n == Converters::Boolean) {
            n = Value::Bool;
        } else if (n < Converters::Long) {
            n = Value::Int;
        } else {
            n =  n - Converters::Long + Value::Long;
        }
        return static_cast<Value::Type>(n);
    }
    if (ci.isAssignableFrom(classes[Converters::Iterable]->clazz(), type)) {
        return Value::Array_;
    }
    if (ci.isAssignableFrom(classes[Converters::Map]->clazz(), type)) {
        return Value::Map_;
    }
    return Value::Object_;
}

// ProxyObject.invoke(args): args
// ProxyObject.setProperty(value): value
// Object.getProperty(): result
// Object.invoke(): result

Value JniVariant::toValue(jobject object)
{
    ClassClass & ci = classClass();
    jclass type = ci.getClass(object);
    jclass comtype = type;
    if (ci.isArray(type)) {
        comtype = ci.getComponentType(type);
    }
    JniConverter ** pjc = std::find_if(classes, classes + 9, [type] (JniConverter * jc) {
        return *jc == type;
    });
    if (pjc < classes + Converters::Array) {
        if (comtype == type)
            return (*pjc)->toValue(object);
        else
            return static_cast<BoxConverter*>(*pjc)->toArrayValue(static_cast<jarray>(object));
    }
    if (comtype != type) {
        return classes[Converters::Array]->toValue(object);
    }
    if (ci.isInstance(classes[Converters::Iterable]->clazz(), object)) {
        return classes[Converters::Iterable]->toValue(object);
    }
    if (ci.isInstance(classes[Converters::Map]->clazz(), object)) {
        return classes[Converters::Map]->toValue(object);
    }
    return Value(registerObject(classes[0]->env(), object));
}

// init(): result
// ProxyObject.invoke(): result
// ProxyObject.getProperty(): result
// ProxyObject.signal(args): args
// Object.setProperty(value): value
// Object.invoke(args): args

jobject JniVariant::fromValue(const Value &v)
{
    if (v.isInt())
        return classes[Converters::Integer]->fromValue(v);
    else if (v.isLong())
        return classes[Converters::Long]->fromValue(v);
    else if (v.isFloat())
        return classes[Converters::Float]->fromValue(v);
    else if (v.isDouble())
        return classes[Converters::Double]->fromValue(v);
    else if (v.isString())
        return classes[Converters::String]->fromValue(v);
    else if (v.isBool())
        return classes[Converters::Boolean]->fromValue(v);
    else if (v.isArray()) {
        return classes[Converters::Array]->fromValue(v); // TODO: support primitive array
    } else if (v.isMap()) {
        return classes[Converters::Map]->fromValue(v);
    } else if (v.isObject()) {
        return static_cast<jobject>(v.toObject());
    }
    return nullptr;
}

static std::vector<jobject> s_objects;

jobject JniVariant::registerObject(JNIEnv * env, jobject object)
{
    auto it = std::find(s_objects.begin(), s_objects.end(), object);
    if (it == s_objects.end()) {
        it = std::find_if(s_objects.begin(), s_objects.end(), JObjectFinder(env, object));
        if (it == s_objects.end()) {
            object = env->NewWeakGlobalRef(object);
            s_objects.push_back(object);
        } else {
            object = *it;
        }
    } else {
        object = *it;
    }
    return object;
}

jobject JniVariant::findObject(JNIEnv *env, jobject object)
{
    auto it = std::find_if(s_objects.begin(), s_objects.end(), JObjectFinder(env, object));
    if (it == s_objects.end())
        return nullptr;
    return *it;
}

jobject JniVariant::deregisterObject(JNIEnv *env, jobject object)
{
    auto it = std::find_if(s_objects.begin(), s_objects.end(), JObjectFinder(env, object));
    if (it == s_objects.end())
        return nullptr;
    object = *it;
    s_objects.erase(it);
    return object;
}
