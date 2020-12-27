#include "jnivariant.h"
#include "jniclass.h"

#include <algorithm>

struct JniConverter
{
    JniConverter(JNIEnv *env, char const * className = nullptr)
    {
        env_ = env;
        if (className) {
            clazz_ = env->FindClass(className);
            clazz_ = static_cast<jclass>(env->NewGlobalRef(clazz_));
        } else {
            clazz_ = nullptr;
        }
    }
    virtual ~JniConverter()
    {
        if (clazz_)
            env_->DeleteGlobalRef(clazz_);
    }
    virtual Value toValue(jobject object) = 0;
    virtual jobject fromValue(Value const & value) = 0;
    jclass clazz() const { return clazz_; }
    friend bool operator==(JniConverter const & l, jclass const & r)
    {
        return l.env_->IsSameObject(l.clazz_, r);
    }
protected:
    JNIEnv *env_;
    jclass clazz_;
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
            jobject object = env_->GetObjectArrayElement(array, i);
            array2.emplace_back(JniVariant::toValue(object));
            env_->DeleteLocalRef(object);
        }
        return std::move(array2);
    }
    virtual jobject fromValue(Value const & value) override {
        return env_->NewStringUTF(value.toString().c_str());
    }
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
            jobject o = iteratorConverter_.next(iterater);
            varray.emplace_back(JniVariant::toValue(o));
            env_->DeleteLocalRef(o);
        }
        return std::move(varray);
    }
    virtual jobject fromValue(Value const & value) override {
        return env_->NewStringUTF(value.toString().c_str());
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
            jobject key = entryConverter_.getKey(entry);
            jobject value = entryConverter_.getValue(entry);
            map.emplace(cc.toString(key), JniVariant::toValue(value));
            env_->DeleteLocalRef(entry);
        }
        return std::move(map);
    }
    virtual jobject fromValue(Value const & value) override {
        return env_->NewStringUTF(value.toString().c_str());
    }
private:
    struct EntryConverter : JniConverter
    {
        EntryConverter(JNIEnv *env) : JniConverter(env, "java/util/Map/Entry") {
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
        new IterableConverter(env),
        nullptr,
    };
    std::copy(classes2, classes2 + 12, classes);
}

Value JniVariant::toValue(const jobject & object)
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
    if (pjc < classes + 9) {
        if (comtype == type)
            return (*pjc)->toValue(object);
        else
            return static_cast<BoxConverter*>(*pjc)->toArrayValue(static_cast<jarray>(object));
    }
    if (comtype != type) {
        return classes[9]->toValue(object);
    }
    if (ci.isInstance(classes[10]->clazz(), object)) {
        return classes[10]->toValue(object);
    }
    if (ci.isInstance(classes[11]->clazz(), object)) {
        return classes[11]->toValue(object);
    }
    return Value(object);
}

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
