#pragma once

#include<config.h>

#include<token.h>
#include<functional.h>

#include<memory>

#include<string>
#include<vector>
#include<type_traits>
#include<deque>
#include<unordered_map>
#include<unordered_set>
#include<source_location>

namespace nc
{

  /// <summary>
  /// Simple, extremely fast bump allocator, must only be used from the main thread.
  /// </summary>
  /// 
  struct ScratchAllocator
  {
    static ScratchAllocator& get();

    using uchar = unsigned char;

    ScratchAllocator(std::unique_ptr<uchar[]>&& the_data, const size_t the_size);

    ScratchAllocator(const size_t the_size) : ScratchAllocator(std::make_unique<uchar[]>(the_size), the_size) {}


    void* allocate(const size_t size_to_alloc, const size_t alignment);

    void free([[maybe_unused]] void* const ptr);

    using memcopy_callback_t = FunctionReference<void(void* dst, void* src, size_t size)>;
    void* reallocate(void* const ptr, const size_t old_size, const size_t new_size, const size_t alignment, memcopy_callback_t memcopy_callback = memcopy_callback_t{}); // TODO: implement this

    struct Scope {
      Scope(ScratchAllocator* const the_allocator, const cstr& the_debug_name, std::unique_ptr<uchar[]> &&new_data=nullptr, size_t new_size = 0);
      Scope(Scope&&) = delete;
      Scope(const Scope&) = delete;
      ~Scope();

    private:
      ScratchAllocator* allocator;
      uchar* checkpoint;
      std::unique_ptr<uchar[]> data_checkpoint;
      size_t size_checkpoint;
#if NC_SCRATCH_ALLOCATOR_CHECKS
      size_t scope_id;
#endif
    };


    Scope create_scope(const cstr& debug_name) { return Scope(this, debug_name); }
    Scope create_scope(const cstr& debug_name, const size_t new_size) { return Scope(this, debug_name, std::make_unique<uchar[]>(new_size), new_size); }
    Scope create_scope(const std::source_location location = std::source_location::current()) { return create_scope(location.function_name()); }

    bool is_empty() const { return bump_ptr == data.get(); }
    size_t get_remaining_capacity() const { return (data.get() + size) - bump_ptr; }
    size_t get_total_capacity() const { return size; }
#if NC_SCRATCH_ALLOCATOR_CHECKS
    bool is_owner_of(size_t scope_id, void* const ptr) const;
#endif

  private:
    std::unique_ptr<uchar[]> data;
    size_t size;
    uchar* bump_ptr;

#if NC_SCRATCH_ALLOCATOR_CHECKS
    static constexpr size_t INVALID_SCOPE_ID = ~static_cast<size_t>(0);
    size_t current_top_scope = INVALID_SCOPE_ID;
    struct scope_info_t {
      cstr            scope_name;
      uchar*          scope_start;
      uchar*          allocator_data;
      size_t          allocator_size;
      size_t          allocations_count;
    };
    std::vector<scope_info_t> scopes_debug;

    size_t find_scope_for_allocation(void* const allocation) const;
    std::string get_current_scope_path_string() const;
#endif
  };


  template<typename T, typename... TArgs>
  T* scratch_new(TArgs&&... args) {
    if (void* const ret_raw = ScratchAllocator::get().allocate(sizeof(T), alignof(T))) {
      T* const ret = static_cast<T*>(ret_raw);
      try {
        new (ret) T(std::forward<TArgs>(args)...);
      }
      catch (...) { // In case the constructor threw an exception, don't forget to free the allocated memory before rethrowing
        ScratchAllocator::get().free(ret_raw);
        throw;
      }
      return ret;
    }
    throw std::bad_alloc();
  }

  template<typename T>
  void scratch_delete(T* const ptr) {
    if (! ptr) return;
    ptr->~T();
    ScratchAllocator::get().free(ptr);
  }

  void* scratch_malloc(const size_t size_to_alloc, const size_t alignment);
  void* scratch_malloc_safe(const size_t size_to_alloc, const size_t alignment);
  void* scratch_realloc(void* const ptr, const size_t old_size, const size_t new_size, const size_t alignment);
  void  scratch_free(void* const ptr);


  // Template parameter passed to std:: collections to ensure they allocate their memory from ScratchAllocator
  template<typename T>
  struct ScratchAllocatorStlAdapter 
  {
      using value_type = T;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using is_always_equal = std::true_type;

      template<typename TOther>
      struct rebind {
        using other = ScratchAllocatorStlAdapter<TOther>;
      };

      constexpr ScratchAllocatorStlAdapter() noexcept {}
      template<typename TOther> constexpr ScratchAllocatorStlAdapter(const ScratchAllocatorStlAdapter<TOther>&) noexcept {}

      template<typename TOther> constexpr bool operator==(const ScratchAllocatorStlAdapter<TOther>&) noexcept { return true; }
      template<typename TOther> constexpr bool operator!=(const ScratchAllocatorStlAdapter<TOther>&) noexcept { return false; }

      T* allocate(const size_type n)
      {
        constexpr size_type MAX_COUNT = std::numeric_limits<size_type>::max() / sizeof(T);
        if (n > MAX_COUNT) {
          throw std::bad_array_new_length();
        }

        if (void* const ret = ScratchAllocator::get().allocate(n * sizeof(T), alignof(T))) {
          return static_cast<T*>(ret);
        }
        throw std::bad_alloc();
      }

      void deallocate(T* const p, [[maybe_unused]] const size_type n)
      {
        if (!p) return;
        ScratchAllocator::get().free(p);
      }

      // Can be passed as second parameter to std::unique_ptr
      struct Deleter
      {
        void operator()(T* const ptr) { scratch_delete(ptr); }
      };
  };



  // Defines for scratch versions of std collections
  using scratch_string = std::basic_string<char, std::char_traits<char>, ScratchAllocatorStlAdapter<char>>;
  using scratch_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, ScratchAllocatorStlAdapter<wchar_t>>;

  template<typename T>
  using scratch_vector = std::vector<T, ScratchAllocatorStlAdapter<T>>;

  template<typename T>
  using scratch_deque = std::deque<T, ScratchAllocatorStlAdapter<T>>;

  template<typename TKey, typename TValue, typename THasher = std::hash<TKey>, typename TKeyEq = std::equal_to<TKey>>
  using scratch_unordered_map = std::unordered_map<TKey, TValue, THasher, TKeyEq, ScratchAllocatorStlAdapter<std::pair<const TKey, TValue>>>;

  template<typename TKey, typename THasher = std::hash<TKey>, typename TKeyEq = std::equal_to<TKey>>
  using scratch_unordered_set = std::unordered_set<TKey, THasher, TKeyEq, ScratchAllocatorStlAdapter<TKey>>;

  template<typename T>
  using scratch_unique_ptr = std::unique_ptr<T, typename ScratchAllocatorStlAdapter<T>::Deleter>;

  template<typename T, typename... TArgs>
  scratch_unique_ptr<T> scratch_make_unique(TArgs&&...args) {
    return scratch_unique_ptr<T>(scratch_new<T>(std::forward<TArgs>(args)...));
  }

  template<typename... Args>
  scratch_string scratch_format(std::format_string<Args...> fmt, Args&&... args) {
    scratch_string ret;
    std::format_to(std::back_inserter(ret), fmt, std::forward<Args>(args)...);
    return ret;
  }


  static inline constexpr size_t operator ""_kb(const unsigned long long int num) { return static_cast<size_t>(num * 1024u); }
  static inline constexpr size_t operator ""_mb(const unsigned long long int num) { return static_cast<size_t>(num * 1024_kb); }

}/// namespace nc

