package com.tal.hybridge;

public abstract class Transport
{
    private long handle_ = 0;

    public Transport() {
        handle_ = create();
    }

    @Override
    public void finalize() {
        free(handle_);
        handle_ = 0;
    }

    long handle() {
         return handle_;
    }

    protected abstract void sendMessage(String message);

    protected void messageReceived(String message) {
        messageReceived(handle_, message);
    }

    private native void messageReceived(long handle, String message);

    private native long create();

    private native void free(long handle);
}
