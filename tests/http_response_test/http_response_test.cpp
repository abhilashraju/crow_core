#include "http_response.hpp"

#include "gtest/gtest.h"

TEST(http_response, Defaults) {
  using namespace crow;
  Response res;
  EXPECT_EQ(std::holds_alternative<Response::string_body_response_type>(
                res.genericResponse.value()),
            true);
}