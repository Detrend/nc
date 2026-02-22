#include<config.h>

#ifdef NC_TESTS

#include <token.h>
#include<unit_test.h>
#include<common.h>

#include<random>

namespace nc {

  template<typename TToken>
  static void test_basic(const std::string& str) {
    TToken tok(str);
    nc_assert(str == tok.to_string());
  }


  template<typename TToken>
  bool basic_token_test([[maybe_unused]] nc::unit_test::TestCtx& _dummy) {

#   define do_test(str)                                                 \
      do{                                                               \
        TToken tok(str);                                                \
        std::string result = tok.to_string();                           \
        if(str != result){                                              \
          nc_crit("Test failed: '{0}' != '{1}'", str, result);          \
          NC_TEST_FAIL;                                                 \
        }                                                               \
      } while (false)

    static constexpr size_t MAX_LENGTH = TToken::MAX_LENGTH;

    do_test("");

    static const std::string CHARS(BasicToken::PERMITTED_CHARS);

    // test all repetitions of a single char
    for (auto c : CHARS) {
      for (size_t t = 1; t <= MAX_LENGTH; ++t) {
        std::string str(t, c);
        do_test(str);
      }
    }
    //do some randomized tests
    srand(542354544);// some fixed seed
    for (size_t test_idx = 0; test_idx < 100'000; ++test_idx){
      static constexpr size_t MIN_RANDOM_STRING_LENGTH = 4;
      size_t length = MIN_RANDOM_STRING_LENGTH + (rand() % (MAX_LENGTH - MIN_RANDOM_STRING_LENGTH));
      std::string str;
      for (size_t t = 0; t < length; ++t)
        str.push_back(CHARS[rand() % CHARS.size()]);
      do_test(str);
    }

#undef do_test

    NC_TEST_SUCCESS;
  }



  NC_UNIT_TEST(basic_token_test<BasicToken>)->name("token_basic_test");
  NC_UNIT_TEST(basic_token_test<CompositeToken<2>>)->name("token2_test");
  NC_UNIT_TEST(basic_token_test<CompositeToken<3>>)->name("token3_test");

}



#endif