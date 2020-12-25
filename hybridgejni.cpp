#include "hybridgejni.h"
#include "jnimeta.h"

static JavaVM* s_vm = nullptr;

static jclass sc_RuntimeException = nullptr;

HybridgeJni::HybridgeJni()
{
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    JNIEnv *env = nullptr;
    int status = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status != JNI_OK)
        return status;
    if (!JniMeta::init(vm, env))
        return JNI_ERR;
    // RuntimeException
    sc_RuntimeException = env->FindClass("java/lang/RuntimeException");
    if (sc_RuntimeException == nullptr) {
        return JNI_ERR;
    }
    sc_RuntimeException = reinterpret_cast<jclass>(env->NewGlobalRef(sc_RuntimeException));
    // PointF fields
    jclass clazzPointF = env->FindClass("android/graphics/PointF");
    if (clazzPointF == nullptr) {
        return JNI_ERR;
    }
    sf_PointF_X = env->GetFieldID(clazzPointF, "x", "F");
    sf_PointF_Y = env->GetFieldID(clazzPointF, "y", "F");
    // RectF methods
    jclass clazzRectF = env->FindClass("android/graphics/RectF");
    if (clazzRectF == nullptr) {
        return JNI_ERR;
    }
    sm_RectF_set = env->GetMethodID(clazzRectF, "set", "(FFFF)V");
    // Matrix methods
    jclass clazzMatrix = env->FindClass("android/graphics/Matrix");
    if (clazzMatrix == nullptr) {
        return JNI_ERR;
    }
    sm_Matrix_getValues = env->GetMethodID(clazzMatrix, "getValues", "([F)V");
    // Stroke methods
    JNINativeMethod2 methods[] = {
        {"create", "([Landroid/graphics/PointF;[FFZZZ)J", reinterpret_cast<void*>(&createStroke)},
        {"clone", "(J)J", reinterpret_cast<void*>(&cloneStroke)},
        {"transform", "(JLandroid/graphics/Matrix;)V", reinterpret_cast<void*>(&transformStroke)},
        {"hitTest", "(JLandroid/graphics/PointF;)Z", reinterpret_cast<void*>(&hitTestStroke)},
        {"getGeometry", "(JLandroid/graphics/RectF;)Landroid/graphics/Path;", reinterpret_cast<void*>(&getStrokeGeometry)},
        {"free", "(J)V", reinterpret_cast<void*>(&freeStroke)},
    };
    jclass clazzStroke = env->FindClass("com/tal/inkcanvas/Stroke");
    if (clazzStroke == nullptr) {
        return JNI_ERR;
    }
    status = env->RegisterNatives(clazzStroke, reinterpret_cast<JNINativeMethod*>(methods), sizeof(methods) / sizeof(methods[0]));
    if (status != JNI_OK)
        return status;
    return JNI_VERSION_1_6;
}
