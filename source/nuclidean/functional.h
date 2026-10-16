// Project Nuclidean Source File
/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once

#include<type_traits>
#include<functional>


namespace nc {

  template<typename T>
  struct EmptyFunctionHelper {
    static_assert(sizeof(EmptyFunctionHelper) < 0, "Instantiating EmptyFunctionHelper with invalid function signature!");
  };
  template<typename TRet, typename... TArgs>
  struct EmptyFunctionHelper<TRet(TArgs...)> {
    using ptr_t = TRet(*)(TArgs...);
    static inline constexpr ptr_t value = [](TArgs...) -> TRet { TRet ret; return ret; };
  };
  template<typename... TArgs>
  struct EmptyFunctionHelper<void(TArgs...)> {
    using ptr_t = void(*)(TArgs...);
    static inline constexpr ptr_t value = [](TArgs...) -> void{ };
  };

  // Reference to a lambda without ownership semantics
  template<typename TSignature>
  struct FunctionReference
  {
    static_assert(sizeof(FunctionReference) < 0, "Instantiating FunctionReference with invalid function signature!");
  };

  template<typename TRet, typename... TArgs>
  struct FunctionReference<TRet(TArgs...)>
  {
  private:
    using wrapper_function_t = TRet(const void* const capture, TArgs...);
    static inline constexpr wrapper_function_t* EMPTY_FUNCTION = EmptyFunctionHelper<wrapper_function_t>::value;
  public:

    template<typename TLambda>
    FunctionReference(const TLambda& lambda)
      : lambda(static_cast<const void*>(&lambda))
      , call_wrapper(static_cast<wrapper_function_t*>([](const void* const capture, TArgs... args)->TRet {
          return (*static_cast<const TLambda*>(capture))(std::forward<TArgs>(args)...);
        }))
    {}

    FunctionReference()
      : lambda(nullptr)
      , call_wrapper(EMPTY_FUNCTION)
    { }

    TRet operator()(TArgs... args)
    {
      return call_wrapper(lambda, std::forward<TArgs>(args)...);
    }

    operator bool()const { return call_wrapper != EMPTY_FUNCTION; }

  private:
    const void* lambda;
    wrapper_function_t* call_wrapper;
  };

  // Function invoked on each element of some collection by iterate_*() functions.
  // Returns either `true` to continue iterating, or `false` to break 
  template<typename TElement>
  using IteratorFunction = FunctionReference<bool(TElement)>;

  template<typename TSignature>
  class Delegate
  {
    static_assert(sizeof(FunctionReference<TSignature>) < 0, "Instantiating FunctionReference with invalid function signature!");
  };

  template<typename... TArgs>
  class Delegate<void(TArgs...)>
  {
    using MemberFunction = std::function<void(TArgs...)>;
    std::vector<MemberFunction> functions;
  public:
    Delegate(const std::initializer_list<MemberFunction>& list)
      : functions(list)
    {}

    void operator()(TArgs... args) {
      for(const auto& fn: functions) {
        fn(std::forward<TArgs>(args)...);
      }
    }
    void do_register(MemberFunction&& func)
    {
      functions.emplace_back(func);
    }
    // TODO: add do_unregister()
  };

  template class Delegate<void(int)>;
}
