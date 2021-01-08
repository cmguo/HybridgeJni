package com.tal.hybridge;

import java.util.Map;

public abstract class Channel
{
    private long handle_ = 0;

    static {
        System.loadLibrary("HybridgeJnid");
    }

    /*
        if pressures != null, addPressure is ignored
     */
    public Channel() {
        handle_ = create();
    }

    @Override
    public void finalize() {
        free(handle_);
        handle_ = 0;
    }

    public void registerObject(final String name, Object object) {
        registerObject(handle_, name, object);
    }

    public void deregisterObject(Object object) {
        deregisterObject(handle_, object);
    }

    public boolean blockUpdates() {
        return blockUpdates(handle_);
    }

    public void setBlockUpdates(boolean block) {
        setBlockUpdates(handle_, block);
    }

    public void connectTo(Transport transport) {
        connectTo2(transport, null);
    }

    public void connectTo2(Transport transport, ProxyObject.OnResult onResult) {
        connectTo(handle_, transport.handle(), onResult);
    }

    public void disconnectFrom(Transport transport) {
        disconnectFrom(handle_, transport.handle());
    }

    /* Protected methods called by devided class */

    protected void propertyChanged(Object object, String name) {
        propertyChanged(handle_, object, name);
    }

    protected void timerEvent() {
        timerEvent(handle_);
    }

    /* Protected methods implemented by devided class */

    protected abstract String createUuid();

    protected abstract void startTimer(int msec);

    protected abstract void stopTimer();

    /* Native methods */

    private native long create();

    private native void registerObject(long handle, String name, Object object);

    private native void deregisterObject(long handle, Object object);

    private native boolean blockUpdates(long handle);

    private native void setBlockUpdates(long handle, boolean block);

    private native void connectTo(long handle, long transport, ProxyObject.OnResult onResult);

    private native void disconnectFrom(long handle, long transport);

    private native void propertyChanged(long handle, Object object, String name);

    private native void timerEvent(long handle);

    private native void free(long handle);
}
