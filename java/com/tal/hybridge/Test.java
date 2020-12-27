package com.tal.hybridge;

class TestChannel extends Channel
{

    private int x;

    public int getX() { return x; }
    public void setX(int x) {
        this.x = x;
    }

    @Override
    protected String createUuid() {
        return null;
    }

    @Override
    protected void startTimer(int msec) {
    }

    @Override
    protected void stopTimer() {
    }
    
}

class TestTransport extends Transport
{

    @Override
    protected void sendMessage(String message) {
        // TODO Auto-generated method stub

    }
    
}

public class Test
{
    public static void main(String[] args) {
        try {
            System.in.read();
        } catch (Exception e) {
        }
        Channel c = new TestChannel();
        Transport t = new TestTransport();
        c.registerObject("channel", c);
        c.connectTo(t);
    }
}
