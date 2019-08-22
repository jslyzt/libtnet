#include "tnet_test.h"

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

////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    testing::AddGlobalTestEnvironment(new TNetEnvironment);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}