#include<memory.h>

#include<common.h>

namespace nc {
  
  static constexpr size_t DEFAULT_SCRATCH_SIZE = static_cast<size_t>(4) * 1024 * 1024; // 4MB

  static ScratchAllocator ScratchAllocatorInstance (DEFAULT_SCRATCH_SIZE);

  ScratchAllocator& ScratchAllocator::get() { return ScratchAllocatorInstance; }


  static int ptr_compare(void* const a, void* const b)
  {
    const uintptr_t ai = reinterpret_cast<uintptr_t>(a);
    const uintptr_t bi = reinterpret_cast<uintptr_t>(b);
    return ai < bi ? -1 : (ai > bi ? 1 : 0);
  }

#if NC_SCRATCH_ALLOCATOR_CHECKS
# define scratch_log(type, format, ...) type("[ScratchAllocator] " format ", at scope `{}`", __VA_ARGS__ , get_current_scope_path_string())
#else
# define scratch_log(type, format, ...) type("[ScratchAllocator] " format , __VA_ARGS__ )
#endif


  ScratchAllocator::ScratchAllocator(std::unique_ptr<uchar[]>&& the_data, const size_t the_size)
    : data(std::move(the_data))
    , size(the_size)
    , bump_ptr(data.get())
  {}


  void* nc::ScratchAllocator::allocate(const size_t size_to_alloc, const size_t alignment) {
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect(scopes_debug.size() > 0);
    nc_expect(current_top_scope == (scopes_debug.size() - 1));
    nc_expect(scopes_debug[current_top_scope].scope_start <= bump_ptr);
#endif
    void* ret = bump_ptr;
    size_t remaining_space = get_remaining_capacity();
    if (std::align(alignment, size_to_alloc, ret, remaining_space)) {
#if NC_SCRATCH_ALLOCATOR_CHECKS
      ++scopes_debug[current_top_scope].allocations_count;
#endif
      bump_ptr = static_cast<uchar*>(ret) + size_to_alloc;
      return ret;
    }

    scratch_log(nc_crit, " Failed to allocate {}B with alignment {}B! (Only {}B is available)", size_to_alloc, alignment, get_remaining_capacity());
    return nullptr;
  }

  void* ScratchAllocator::reallocate(void* const ptr, const size_t old_size, const size_t new_size, const size_t alignment, memcopy_callback_t memcopy_callback)
  {
    if (!ptr) {
      return allocate(new_size, alignment);
    }

    if (reinterpret_cast<uintptr_t>(ptr) == (reinterpret_cast<uintptr_t>(bump_ptr) - old_size)) {
      // We are reallocating the very last allocation - leave it where it is and just reserve additional space for it
      uchar*const old_bump_ptr = bump_ptr;
      this->bump_ptr = static_cast<uchar*>(ptr);
      void* ret = ptr;
      size_t remaining_space = get_remaining_capacity();
      if (std::align(alignment, new_size, ret, remaining_space)) {
        if(ptr != ret) {
          // Alignment changed, we need to move the data
          scratch_log(nc_warn, "Realloc {}->{}B with alignment {}B was re-aligned from {} to {}", old_size, new_size, alignment, ptr, ret);
          if (memcopy_callback)
            memcopy_callback(ret, ptr, std::min<size_t>(old_size, new_size));
          else
            std::memmove(ret, ptr, std::min<size_t>(old_size, new_size));
        }
        bump_ptr = static_cast<uchar*>(ret) + new_size;
        return ret;
      }
      scratch_log(nc_crit, "Failed to fast-reallocate {}->{}B with alignment {}B! (Only {}B is available)", old_size, new_size, alignment, get_remaining_capacity());
      this->bump_ptr = old_bump_ptr; // Restore bump pointer to its original value
      return nullptr;
    }
    void* const ret = allocate(new_size, alignment);
    if (!ret) {

      scratch_log(nc_crit, " Failed to slow-reallocate {}->{}B with alignment {}B! (Only {}B is available)", old_size, new_size, alignment, get_remaining_capacity());
      return nullptr;
    }
    nc_assert(ptr_compare(ptr, ret) != 0);
    if (memcopy_callback)
      memcopy_callback(ret, ptr, std::min<size_t>(old_size, new_size));
    else
      std::memcpy(ret, ptr, std::min<size_t>(old_size, new_size));
    free(ptr);
    return ret;
  }


  void ScratchAllocator::free([[maybe_unused]] void* const ptr)
  {
    nc_assert(ptr);
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect(scopes_debug.size() > 0);
    nc_expect(current_top_scope == (scopes_debug.size() - 1));
    const size_t used_scope = find_scope_for_allocation(ptr);
    nc_expect(used_scope != INVALID_SCOPE_ID, "Pointer {0} does not belong to any of the scopes in allocator {1}", ptr, get_current_scope_path_string());
    nc_assert(this->is_owner_of(used_scope, ptr));
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
    for (size_t scope_idx = scopes_debug.size(); scope_idx --> 0; ) { // Iterate backwards because we are more likely to search for an allocation that's near the top
      // Iterate over all scopes and check if they're the one where this allocation resides
      const auto& scope = scopes_debug[scope_idx];
      if (ptr_compare(scope.scope_start, allocation) <= 0) {
        void* const upper_bound = ((scopes_debug.size() > (scope_idx + 1)) && (scopes_debug[scope_idx + 1].allocator_data == scope.allocator_data))
          ? scopes_debug[scope_idx + 1].scope_start
          : (scope.allocator_data + scope.allocator_size)
        ;
        if (ptr_compare(allocation, upper_bound) >= 0) {
          continue;
        }
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

  // Does given pointer point into the memory owned by this allocator
  bool ScratchAllocator::is_owner_of(size_t scope_id, void* const ptr) const
  {
    if (!ptr) return false;

    const auto& scope = scopes_debug[scope_id];

    if ((ptr_compare(scope.allocator_data, ptr) <= 0) && (ptr_compare(ptr, scope.allocator_data + scope.allocator_size) < 0)) {
      // Pointer definitely points to the block of memory that this scope allocates from
      if (ptr_compare(ptr, scope.scope_start) < 0) {
        return false; // Pointer points somewhere before start of this scope
      }
      if ((scopes_debug.size() > (scope_id + 1)) && (scopes_debug[scope_id + 1].allocator_data == scope.allocator_data)) {
        // There is another scope after this one, allocating from this same block of memory
        if (ptr_compare(scopes_debug[scope_id + 1].scope_start, ptr) <= 0) {
          return false; // Pointer points to this next scope or some other after it
        }
      }
      return true; // Pointer points to the memory block used by this scope - not before this scope's checkpoint and not after some succeeding scope's checkpoint -> it points into this scope's memory
    }
    return false; // Pointer points to some totally different memory block
  }
#endif



  ScratchAllocator::Scope::Scope(ScratchAllocator* const the_allocator, [[maybe_unused]] const cstr& the_debug_name, std::unique_ptr<uchar[]>&& new_data/*= nullptr */, const size_t new_size/*= 0*/)
    : allocator(the_allocator), checkpoint(the_allocator->bump_ptr), data_checkpoint(nullptr)

#if NC_SCRATCH_ALLOCATOR_CHECKS
    , scope_id(++the_allocator->current_top_scope)
#endif
  {
    if (new_data) {
      this->data_checkpoint = std::move(allocator->data);
      this->size_checkpoint = allocator->size;
      allocator->data = std::move(new_data);
      allocator->size = new_size;
      allocator->bump_ptr = allocator->data.get();
    }
#if NC_SCRATCH_ALLOCATOR_CHECKS
    nc_expect(allocator->scopes_debug.size() == scope_id);
    allocator->scopes_debug.emplace_back(scope_info_t{ .scope_name = the_debug_name, .scope_start = allocator->bump_ptr, .allocator_data = allocator->data.get(), .allocator_size = allocator->size, .allocations_count = 0});
#endif
  }


  ScratchAllocator::Scope::~Scope() {
#if NC_SCRATCH_ALLOCATOR_CHECKS
    const auto& scope = allocator->scopes_debug[scope_id];
    nc_expect((allocator->scopes_debug.size() == (scope_id + 1)) && (scope_id == allocator->current_top_scope), "Destroying scope '{0}' at index {1} while current scope is '{2}'", allocator->scopes_debug[scope_id].scope_name, scope_id, allocator->get_current_scope_path_string());
    nc_expect(data_checkpoint || (scope.scope_start == checkpoint)); // If this doesn't hold, we got weird data corruption xD, only holds for scopes that didn't provide their own data block
    nc_expect(scope.allocations_count == 0, "Destroying scope '{0}' while it still has {1} unfreed allocations!, at scopes '{2}'", allocator->scopes_debug[scope_id].scope_name, allocator->scopes_debug[scope_id].allocations_count, allocator->get_current_scope_path_string()); // Scope must not be destroyed unless all of its objects have been freed
    allocator->scopes_debug.pop_back();
    --allocator->current_top_scope;
#endif
    allocator->bump_ptr = checkpoint;
    if (data_checkpoint) {
      allocator->data = std::move(data_checkpoint);
      allocator->size = size_checkpoint;
    }
  }


  void* scratch_malloc(const size_t size_to_alloc, const size_t alignment) { return ScratchAllocator::get().allocate(size_to_alloc, alignment); }

  void* scratch_realloc(void* const ptr, const size_t old_size, const size_t new_size, const size_t alignment) { return ScratchAllocator::get().reallocate(ptr, old_size, new_size, alignment); }

  void scratch_free(void* const ptr) { if(ptr) return ScratchAllocator::get().free(ptr); }


  void* scratch_malloc_safe(const size_t size_to_alloc, const size_t alignment)
  {
    void* ret = scratch_malloc(size_to_alloc, alignment);
    if (!ret) throw std::bad_alloc();
    return ret;
  }

} // namespace nc
