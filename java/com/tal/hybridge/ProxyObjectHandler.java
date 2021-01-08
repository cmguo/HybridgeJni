package com.tal.hybridge;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.CompletableFuture;

public class ProxyObjectHandler implements InvocationHandler {

    private ProxyObject object;

    public ProxyObjectHandler(ProxyObject object) {
        this.object = object;
    }

    public static <T> T getObject(Class<T> clazz, ProxyObject object) {
        Class<?>[] classes = new Class<?>[1];
        classes[0] = clazz;
        Object o = Proxy.newProxyInstance(ProxyObjectHandler.class.getClassLoader(), classes, new ProxyObjectHandler(object));
        return (T) o;
    }

    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        final CompletableFuture<Object> future = new CompletableFuture<>();
        boolean ok = object.invokeMethod(method, args, (Object result) -> {
            future.complete(result);
        });
        if (!ok)
            future.completeExceptionally(new NoSuchMethodError());
        return future;
    }

}
