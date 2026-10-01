#include<memory.h>

#include<common.h>

namespace nc {
  
  static constexpr size_t DEFAULT_SCRATCH_SIZE = static_cast<size_t>(2) * 1024 * 1024; // 2MB

  static ScratchAllocator ScratchAllocatorInstance (DEFAULT_SCRATCH_SIZE);

  ScratchAllocator& ScratchAllocator::get() { return ScratchAllocatorInstance; }

  ScratchAllocator::ScratchAllocator(std::unique_ptr<unsigned char[]>&& the_data, const size_t the_size)
    : data(std::move(the_data))
    , size(the_size)
    , bump_ptr(data.get())
  {}


  void* nc::ScratchAllocator::allocate(const size_t size_to_alloc, const size_t alignment) {
    void* ret = bump_ptr;
    size_t remaining_space = get_remaining_capacity();
    if (std::align(alignment, size_to_alloc, ret, remaining_space)) {
#if NC_SCRATCH_ALLOCATOR_CHECKS
      nc_expect(scopes_debug.size() > 0);
      nc_expect(current_top_scope == (scopes_debug.size() - 1));
      nc_expect(scopes_debug[current_top_scope].scope_start <= bump_ptr);
      ++scopes_debug[current_top_scope].allocations_count;
#endif
      bump_ptr = static_cast<unsigned char*>(ret) + size_to_alloc;
      return ret;
    }

#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_crit("[ScratchAllocator] Failed to allocate {0}B with alignment {1}B! (Only {2}B is available), at scope `{3}`", size_to_alloc, alignment, get_remaining_capacity(), get_current_scope_path_string());
#else
    nc_crit("[ScratchAllocator] Failed to allocate {0}B with alignment {1}B! (Only {2}B is available)", size_to_alloc, alignment, get_remaining_capacity());
#endif
    return nullptr;
  }



  void ScratchAllocator::free([[maybe_unused]] void* const ptr)
  {
    nc_assert(ptr);
    nc_assert(this->is_owner_of(ptr));
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect(scopes_debug.size() > 0);
    nc_expect(current_top_scope == (scopes_debug.size() - 1));
    const size_t used_scope = find_scope_for_allocation(ptr);
    nc_expect(used_scope != INVALID_SCOPE_ID);
    nc_expect(used_scope <= current_top_scope);
    nc_expect(scopes_debug[used_scope].allocations_count > 0);
    --scopes_debug[used_scope].allocations_count;
#endif
  }

#if NC_SCRATCH_ALLOCATOR_CHECKS
  size_t ScratchAllocator::find_scope_for_allocation(void* const allocation) const
  {
    nc_assert(allocation);
    nc_assert(this->is_owner_of(allocation));
    for (size_t scope_idx = scopes_debug.size(); scope_idx --> 0; ) { // Iterate backwards because we are more likely to search for an allocation that's near the top
      // Iterate over all scopes and check if they're the one where this allocation resides
      if ((scopes_debug[scope_idx].scope_start <= allocation) && ((scopes_debug.size() == (scope_idx + 1)) || (allocation < scopes_debug[scope_idx + 1].scope_start))) {
        // We found our scope
        return scope_idx;
      }
    }
    return INVALID_SCOPE_ID;
  }

  std::string ScratchAllocator::get_current_scope_path_string() const
  {
    std::string ret;
    for (const auto& scope : scopes_debug) {
      if (!ret.empty()) ret.append("/");
      ret.append(scope.scope_name);
    }
    return ret;
  }
#endif

  // Does given pointer point into the memory owned by this allocator
  bool ScratchAllocator::is_owner_of(void* const ptr)
  {
    if (!ptr) return false;

    const uintptr_t begin = reinterpret_cast<uintptr_t>(data.get());
    const uintptr_t end = reinterpret_cast<uintptr_t>(data.get() + size);
    const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
    return (begin <= p) && (p < end);
  }



  ScratchAllocator::Scope::Scope(ScratchAllocator* const the_allocator, [[maybe_unused]] const cstr& the_debug_name)
    : allocator(the_allocator), checkpoint(the_allocator->bump_ptr)

#if NC_SCRATCH_ALLOCATOR_CHECKS
    , scope_id(++the_allocator->current_top_scope)
#endif
  {
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect(allocator->scopes_debug.size() == scope_id);
    allocator->scopes_debug.emplace_back(scope_info_t{ .scope_name = the_debug_name, .scope_start = checkpoint, .allocations_count = 0 });
#endif
  }


  ScratchAllocator::Scope::~Scope() {
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect((allocator->scopes_debug.size() == (scope_id + 1)) && (scope_id == allocator->current_top_scope), "Destroying scope '{0}' at index {1} while current scope is '{2}'", allocator->scopes_debug[scope_id].scope_name, scope_id, allocator->get_current_scope_path_string());
    nc_expect(allocator->scopes_debug[scope_id].scope_start == checkpoint); // If this doesn't hold, we got weird data corruption xD
    nc_expect(allocator->scopes_debug[scope_id].allocations_count == 0, "Destroying scope '{0}' while it still has {1} unfreed allocations!, at scopes '{2}'", allocator->scopes_debug[scope_id].scope_name, allocator->scopes_debug[scope_id].allocations_count, allocator->get_current_scope_path_string()); // Scope must not be destroyed unless all of its objects have been freed
    allocator->scopes_debug.pop_back();
    --allocator->current_top_scope;
#endif
    allocator->bump_ptr = checkpoint;
  }


} // namespace nc
