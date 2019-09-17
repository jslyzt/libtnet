#include "tnet_test.h"
#include "tnet.h"
#include "log.h"
#include "log_lvl.h"
#include <thread>
#include <libgo/coroutine.h>

TNetEnvironment::TNetEnvironment() {
}

TNetEnvironment::~TNetEnvironment() {
}

void TNetEnvironment::TearDown() {
    testing::Environment::TearDown();
}

void TNetEnvironment::SetUp() {
    testing::Environment::SetUp();
}

////////////////////////////////////////////////////////////////////////////

TNetTest::TNetTest()
    : testing::Test()
{}

TNetTest::~TNetTest()
{}

void TNetTest::SetUpTestCase()
{}

void TNetTest::TearDownTestCase()
{}

void TNetTest::SetUp()
{}

void TNetTest::TearDown()
{}

#define MIN_THREAD 2
#define MAX_THREAD 6

////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    testing::AddGlobalTestEnvironment(new TNetEnvironment);
    testing::InitGoogleTest(&argc, argv);
    tnet::platformInit();
    tnet::Log::rootLog().setLevel(tnet::TRACE);

    std::thread t([] {
        co_sched.Start(MIN_THREAD, MAX_THREAD);
    });
    t.detach();

    return RUN_ALL_TESTS();
}
