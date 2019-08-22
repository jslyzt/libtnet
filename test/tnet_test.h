#pragma once

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////

class TNetEnvironment : public testing::Environment {
public:
    TNetEnvironment();
    virtual ~TNetEnvironment();
public:
    virtual void SetUp();
    virtual void TearDown();
};

////////////////////////////////////////////////////////////////////////////

class TNetTest : public testing::Test {
public:
    TNetTest();
    virtual ~TNetTest();
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
protected:
    virtual void SetUp();
    virtual void TearDown();
};

////////////////////////////////////////////////////////////////////////////

class CometTest : public TNetTest {
public:
    CometTest() {}
    ~CometTest() {}
};


class EchoTest : public TNetTest {
public:
    EchoTest() {}
    ~EchoTest() {}
};


class HttpTest : public TNetTest {
public:
    HttpTest() {}
    ~HttpTest() {}
};


class NotifyTest : public TNetTest {
public:
    NotifyTest() {}
    ~NotifyTest() {}
};


class RedisTest : public TNetTest {
public:
    RedisTest() {}
    ~RedisTest() {}
};


class SignalTest : public TNetTest {
public:
    SignalTest() {}
    ~SignalTest() {}
};


class TimerTest : public TNetTest {
public:
    TimerTest() {}
    ~TimerTest() {}
};


class TimeWheelTest : public TNetTest {
public:
    TimeWheelTest() {}
    ~TimeWheelTest() {}
};


class WSTest : public TNetTest {
public:
    WSTest() {}
    ~WSTest() {}
};
