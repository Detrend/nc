#pragma once

#include<config.h>

#include<token.h>

#include<memory>

#include<string>
#include<vector>
#include<type_traits>

namespace nc
{

  /// <summary>
  /// Simple, extremely fast bump allocator, must only be used from the main thread.
  /// </summary>
  /// 
  struct ScratchAllocator
  {
    static ScratchAllocator& get();


    ScratchAllocator(std::unique_ptr<unsigned char[]>&& the_data, const size_t the_size);

    ScratchAllocator(const size_t the_size) : ScratchAllocator(std::make_unique<unsigned char[]>(the_size), the_size) {}


    void* allocate(const size_t size_to_alloc, const size_t alignment);

    void free([[maybe_unused]] void* const ptr);

    struct Scope {
      Scope(ScratchAllocator* const the_allocator,const cstr& the_debug_name);
      ~Scope();

    private:
      ScratchAllocator* allocator;
      unsigned char* checkpoint;
#if NC_SCRATCH_ALLOCATOR_CHECKS
      size_t scope_id;
#endif
    };


    Scope create_scope(const cstr& debug_name) { return Scope(this, debug_name); }

    bool is_empty() const { return bump_ptr == data.get(); }
    size_t get_remaining_capacity() const { return (data.get() + size) - bump_ptr; }
    size_t get_total_capacity() const { return size; }

  private:
    std::unique_ptr<unsigned char[]> data;
    size_t size;
    unsigned char* bump_ptr;

#if NC_SCRATCH_ALLOCATOR_CHECKS
    static constexpr size_t INVALID_SCOPE_ID = ~static_cast<size_t>(0);
    size_t current_top_scope = INVALID_SCOPE_ID;
    struct scope_info_t {
      cstr            scope_name;
      unsigned char*  scope_start;
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
      new (ret) T(std::forward<TArgs>(args)...);
      return ret;
    }
    throw std::bad_alloc();
  }

  template<typename T>
  void scratch_delete(T* const ptr) {
    ptr->~T();
    ScratchAllocator::get().free(ptr);
  }


  // Template parameter passed to std:: collections to ensure they allocate their memory from ScratchAllocator
  template<typename T>
  struct ScratchAllocatorStlAdapter 
  {
      using value_type = T;

      ScratchAllocatorStlAdapter() {}
      template<typename TOther> ScratchAllocatorStlAdapter(const ScratchAllocatorStlAdapter<TOther>&) {}

      T* allocate(const std::size_t n) 
      {
        if (void* const ret = ScratchAllocator::get().allocate(n * sizeof(T), alignof(T))) {
          return static_cast<T*>(ret);
        }
        throw std::bad_alloc();
      }

      void deallocate(T* const p, [[maybe_unused]] const std::size_t n)
      {
        ScratchAllocator::get().free(p);
      }

      // Can be passed as second parameter to std::unique_ptr
      struct Deleter
      {
        void operator()(T* const ptr) { scratch_delete(ptr);}
      };
  };



  // Defines for scratch versions of std collections
  using scratch_string = std::basic_string<char, std::char_traits<char>, ScratchAllocatorStlAdapter<char>>;
  using scratch_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, ScratchAllocatorStlAdapter<wchar_t>>;

  template<typename T>
  using scratch_vector = std::vector<T, ScratchAllocatorStlAdapter<T>>;

  template<typename T>
  using scratch_unique_ptr = std::unique_ptr<T, typename ScratchAllocatorStlAdapter<T>::Deleter>;

  template<typename T, typename... TArgs>
  scratch_unique_ptr<T> scratch_make_unique(TArgs&&...args) {
    return scratch_unique_ptr<T>(scratch_new<T>(std::forward<TArgs>(args)...));
  }

}/// namespace nc

