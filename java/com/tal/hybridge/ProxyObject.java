package com.tal.hybridge;

import java.lang.reflect.Method;
import java.util.TreeMap;
import java.util.ArrayList;

public class ProxyObject {

    public static interface OnResult
    {
        void apply(Object result);
    }

    public static interface SignalHandler
    {
        void apply(ProxyObject object, int signalIndex, Object[] args);
    }

    private long handle_;
    private TreeMap<Integer, ArrayList<SignalHandler>> connections_;

    private static SignalHandler SignalHandler = new SignalHandler() {
        @Override
        public void apply(ProxyObject object, int signalIndex, Object[] args) {
            ArrayList<SignalHandler> handlers = object.connections_.get(signalIndex);
            if (handlers == null) return;
            for (SignalHandler handler : handlers) {
                handler.apply(object, signalIndex, args);
            }
        }
    };

    public ProxyObject(long handle) {
        handle_ = handle;
        connections_ = new TreeMap<>();
    }

	public Object readProperty(String property) {
        return readProperty(handle_, property);
    }
    
	public boolean writeProperty(String property, Object value) {
        return writeProperty(handle_, property, value);
    }
    
	public boolean invokeMethod(Method method, Object[] args, OnResult onResult) {
        return invokeMethod(handle_, method, args, onResult);
    }
    
	public boolean connect(int signalIndex, SignalHandler handler) {
        ArrayList<SignalHandler> handlers = connections_.get(signalIndex);
        if (handlers == null) {
            handlers = new ArrayList<>();
            connections_.put(signalIndex, handlers);
            handlers.add(handler);
            return connect(handle_, signalIndex, handler);
        }
        handlers.add(handler);
        return true;
    }
    
	public boolean disconnect(int signalIndex, SignalHandler handler) {
        ArrayList<SignalHandler> handlers = connections_.get(signalIndex);
        if (handlers != null && handlers.remove(handler) && handlers.isEmpty())
            return disconnect(handle_, signalIndex, handler);
        else
            return true;
    }
    
    private native Object readProperty(long handle, String property);

    private native boolean writeProperty(long handle, String property, Object value);

    private native boolean invokeMethod(long handle, Method method, Object[] args, OnResult onResult);

    private native boolean connect(long handle, int signalIndex, SignalHandler handler);

    private native boolean disconnect(long handle, int signalIndex, SignalHandler handler);
}
