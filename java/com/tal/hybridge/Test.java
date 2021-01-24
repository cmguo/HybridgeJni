package com.tal.hybridge;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.concurrent.Future;

class TestChannel extends Channel {

    private static int id;

    private int x;

    public int getX() {
        return x;
    }

    public void setX(int x) {
        this.x = x;
    }

    public int incX() {
        return x++;
    }

    public Channel self() {
        return this;
    }

    @Override
    protected String createUuid() {
        return "O" + String.valueOf(id++);
    }

    @Override
    protected void startTimer(int msec) {
        System.out.println("startTimer " + msec);
    }

    @Override
    protected void stopTimer() {
        System.out.println("stopTimer");
    }

}

class TestTransport extends Transport {

    private Transport another;
    public TestTransport() {
    }
    public TestTransport(TestTransport another) {
        this.another = another;
        another.another = this;
    }
    @Override
    protected void sendMessage(String message) {
        System.out.println("sendMessage " + message);
         if (another != null) {
            another.messageReceived(message);
        }
   }

}

interface ITestObject
{
    Future<String> createUuid();
}

public class Test {
    public static void main(String[] args) {
        //try { System.in.read(); } catch (Throwable e) {}

        TestChannel cp = new TestChannel();
        TestTransport tp = new TestTransport();
        cp.registerObject("channel", cp);
        cp.connectTo(tp);
        //
        TestChannel cr = new TestChannel();
        cr.connectTo2(new TestTransport(tp), result -> {
            Map<String, ProxyObject> map = ((Map<String, ProxyObject>) result);
            System.out.println("init " + map.size());
            ProxyObject po = map.get("channel");
            ITestObject test = ProxyObjectHandler.getObject(ITestObject.class, po);
            try {
                System.out.println(test.createUuid().get());
            } catch (Exception e) {
                System.err.println(e);
                e.printStackTrace(System.err);
            }
        });
        
        try {
            String l;
            BufferedReader r = new BufferedReader(new InputStreamReader(System.in));   
            while(true) {
                l = r.readLine();
                if (l == null || l.length() == 0) {
                    break;
                }
                if (l.equals("+")) {
                    cp.setX(cp.getX() + 1);
                    cp.propertyChanged(cp, "x");
                    continue;
                }
                if (l.equals("t")) {
                    cp.timerEvent();
                    continue;
                }
                tp.messageReceived(l);
            }
        } catch (Exception e) {
            System.err.println(e);
            e.printStackTrace(System.err);
        }
        System.out.println("exit...");
    }
}
