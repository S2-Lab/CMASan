//===-- asan_interface.inc ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Asan interface list.
//===----------------------------------------------------------------------===//

INTERFACE_FUNCTION(__asan_addr_is_in_fake_stack)
INTERFACE_FUNCTION(__asan_address_is_poisoned)
INTERFACE_FUNCTION(__asan_after_dynamic_init)
INTERFACE_FUNCTION(__asan_alloca_poison)
INTERFACE_FUNCTION(__asan_allocas_unpoison)
INTERFACE_FUNCTION(__asan_before_dynamic_init)
INTERFACE_FUNCTION(__asan_describe_address)
INTERFACE_FUNCTION(__asan_exp_load1)
INTERFACE_FUNCTION(__asan_exp_load2)
INTERFACE_FUNCTION(__asan_exp_load4)
INTERFACE_FUNCTION(__asan_exp_load8)
INTERFACE_FUNCTION(__asan_exp_load16)
INTERFACE_FUNCTION(__asan_exp_loadN)
INTERFACE_FUNCTION(__asan_exp_store1)
INTERFACE_FUNCTION(__asan_exp_store2)
INTERFACE_FUNCTION(__asan_exp_store4)
INTERFACE_FUNCTION(__asan_exp_store8)
INTERFACE_FUNCTION(__asan_exp_store16)
INTERFACE_FUNCTION(__asan_exp_storeN)
INTERFACE_FUNCTION(__asan_get_alloc_stack)
INTERFACE_FUNCTION(__asan_get_current_fake_stack)
INTERFACE_FUNCTION(__asan_get_free_stack)
INTERFACE_FUNCTION(__asan_get_report_access_size)
INTERFACE_FUNCTION(__asan_get_report_access_type)
INTERFACE_FUNCTION(__asan_get_report_address)
INTERFACE_FUNCTION(__asan_get_report_bp)
INTERFACE_FUNCTION(__asan_get_report_description)
INTERFACE_FUNCTION(__asan_get_report_pc)
INTERFACE_FUNCTION(__asan_get_report_sp)
INTERFACE_FUNCTION(__asan_get_shadow_mapping)
INTERFACE_FUNCTION(__asan_handle_no_return)
INTERFACE_FUNCTION(__asan_handle_vfork)
INTERFACE_FUNCTION(__asan_init)
INTERFACE_FUNCTION(__asan_load_cxx_array_cookie)
INTERFACE_FUNCTION(__asan_load1)
INTERFACE_FUNCTION(__asan_load2)
INTERFACE_FUNCTION(__asan_load4)
INTERFACE_FUNCTION(__asan_load8)
INTERFACE_FUNCTION(__asan_load16)
INTERFACE_FUNCTION(__asan_loadN)
INTERFACE_FUNCTION(__asan_load1_noabort)
INTERFACE_FUNCTION(__asan_load2_noabort)
INTERFACE_FUNCTION(__asan_load4_noabort)
INTERFACE_FUNCTION(__asan_load8_noabort)
INTERFACE_FUNCTION(__asan_load16_noabort)
INTERFACE_FUNCTION(__asan_loadN_noabort)
INTERFACE_FUNCTION(__asan_locate_address)
INTERFACE_FUNCTION(__asan_memcpy)
INTERFACE_FUNCTION(__asan_memmove)
INTERFACE_FUNCTION(__asan_memset)
INTERFACE_FUNCTION(__asan_poison_cxx_array_cookie)
INTERFACE_FUNCTION(__asan_poison_intra_object_redzone)
INTERFACE_FUNCTION(__asan_poison_memory_region)
INTERFACE_FUNCTION(__asan_poison_stack_memory)
INTERFACE_FUNCTION(__asan_print_accumulated_stats)
INTERFACE_FUNCTION(__asan_region_is_poisoned)
INTERFACE_FUNCTION(__asan_register_globals)
INTERFACE_FUNCTION(__asan_register_elf_globals)
INTERFACE_FUNCTION(__asan_register_image_globals)
INTERFACE_FUNCTION(__asan_report_error)
INTERFACE_FUNCTION(__asan_report_exp_load1)
INTERFACE_FUNCTION(__asan_report_exp_load2)
INTERFACE_FUNCTION(__asan_report_exp_load4)
INTERFACE_FUNCTION(__asan_report_exp_load8)
INTERFACE_FUNCTION(__asan_report_exp_load16)
INTERFACE_FUNCTION(__asan_report_exp_load_n)
INTERFACE_FUNCTION(__asan_report_exp_store1)
INTERFACE_FUNCTION(__asan_report_exp_store2)
INTERFACE_FUNCTION(__asan_report_exp_store4)
INTERFACE_FUNCTION(__asan_report_exp_store8)
INTERFACE_FUNCTION(__asan_report_exp_store16)
INTERFACE_FUNCTION(__asan_report_exp_store_n)
INTERFACE_FUNCTION(__asan_report_load1)
INTERFACE_FUNCTION(__asan_report_load2)
INTERFACE_FUNCTION(__asan_report_load4)
INTERFACE_FUNCTION(__asan_report_load8)
INTERFACE_FUNCTION(__asan_report_load16)
INTERFACE_FUNCTION(__asan_report_load_n)
INTERFACE_FUNCTION(__asan_report_load1_noabort)
INTERFACE_FUNCTION(__asan_report_load2_noabort)
INTERFACE_FUNCTION(__asan_report_load4_noabort)
INTERFACE_FUNCTION(__asan_report_load8_noabort)
INTERFACE_FUNCTION(__asan_report_load16_noabort)
INTERFACE_FUNCTION(__asan_report_load_n_noabort)
INTERFACE_FUNCTION(__asan_report_present)
INTERFACE_FUNCTION(__asan_report_store1)
INTERFACE_FUNCTION(__asan_report_store2)
INTERFACE_FUNCTION(__asan_report_store4)
INTERFACE_FUNCTION(__asan_report_store8)
INTERFACE_FUNCTION(__asan_report_store16)
INTERFACE_FUNCTION(__asan_report_store_n)
INTERFACE_FUNCTION(__asan_report_store1_noabort)
INTERFACE_FUNCTION(__asan_report_store2_noabort)
INTERFACE_FUNCTION(__asan_report_store4_noabort)
INTERFACE_FUNCTION(__asan_report_store8_noabort)
INTERFACE_FUNCTION(__asan_report_store16_noabort)
INTERFACE_FUNCTION(__asan_report_store_n_noabort)
INTERFACE_FUNCTION(__asan_set_death_callback)
INTERFACE_FUNCTION(__asan_set_error_report_callback)
INTERFACE_FUNCTION(__asan_set_shadow_00)
INTERFACE_FUNCTION(__asan_set_shadow_f1)
INTERFACE_FUNCTION(__asan_set_shadow_f2)
INTERFACE_FUNCTION(__asan_set_shadow_f3)
INTERFACE_FUNCTION(__asan_set_shadow_f5)
INTERFACE_FUNCTION(__asan_set_shadow_f8)
INTERFACE_FUNCTION(__asan_stack_free_0)
INTERFACE_FUNCTION(__asan_stack_free_1)
INTERFACE_FUNCTION(__asan_stack_free_2)
INTERFACE_FUNCTION(__asan_stack_free_3)
INTERFACE_FUNCTION(__asan_stack_free_4)
INTERFACE_FUNCTION(__asan_stack_free_5)
INTERFACE_FUNCTION(__asan_stack_free_6)
INTERFACE_FUNCTION(__asan_stack_free_7)
INTERFACE_FUNCTION(__asan_stack_free_8)
INTERFACE_FUNCTION(__asan_stack_free_9)
INTERFACE_FUNCTION(__asan_stack_free_10)
INTERFACE_FUNCTION(__asan_stack_malloc_0)
INTERFACE_FUNCTION(__asan_stack_malloc_1)
INTERFACE_FUNCTION(__asan_stack_malloc_2)
INTERFACE_FUNCTION(__asan_stack_malloc_3)
INTERFACE_FUNCTION(__asan_stack_malloc_4)
INTERFACE_FUNCTION(__asan_stack_malloc_5)
INTERFACE_FUNCTION(__asan_stack_malloc_6)
INTERFACE_FUNCTION(__asan_stack_malloc_7)
INTERFACE_FUNCTION(__asan_stack_malloc_8)
INTERFACE_FUNCTION(__asan_stack_malloc_9)
INTERFACE_FUNCTION(__asan_stack_malloc_10)
INTERFACE_FUNCTION(__asan_stack_malloc_always_0)
INTERFACE_FUNCTION(__asan_stack_malloc_always_1)
INTERFACE_FUNCTION(__asan_stack_malloc_always_2)
INTERFACE_FUNCTION(__asan_stack_malloc_always_3)
INTERFACE_FUNCTION(__asan_stack_malloc_always_4)
INTERFACE_FUNCTION(__asan_stack_malloc_always_5)
INTERFACE_FUNCTION(__asan_stack_malloc_always_6)
INTERFACE_FUNCTION(__asan_stack_malloc_always_7)
INTERFACE_FUNCTION(__asan_stack_malloc_always_8)
INTERFACE_FUNCTION(__asan_stack_malloc_always_9)
INTERFACE_FUNCTION(__asan_stack_malloc_always_10)
INTERFACE_FUNCTION(__asan_store1)
INTERFACE_FUNCTION(__asan_store2)
INTERFACE_FUNCTION(__asan_store4)
INTERFACE_FUNCTION(__asan_store8)
INTERFACE_FUNCTION(__asan_store16)
INTERFACE_FUNCTION(__asan_storeN)
INTERFACE_FUNCTION(__asan_store1_noabort)
INTERFACE_FUNCTION(__asan_store2_noabort)
INTERFACE_FUNCTION(__asan_store4_noabort)
INTERFACE_FUNCTION(__asan_store8_noabort)
INTERFACE_FUNCTION(__asan_store16_noabort)
INTERFACE_FUNCTION(__asan_storeN_noabort)
INTERFACE_FUNCTION(__asan_unpoison_intra_object_redzone)
INTERFACE_FUNCTION(__asan_unpoison_memory_region)
INTERFACE_FUNCTION(__asan_unpoison_stack_memory)
INTERFACE_FUNCTION(__asan_unregister_globals)
INTERFACE_FUNCTION(__asan_unregister_elf_globals)
INTERFACE_FUNCTION(__asan_unregister_image_globals)
INTERFACE_FUNCTION(__asan_version_mismatch_check_v8)
INTERFACE_FUNCTION(__sanitizer_finish_switch_fiber)
INTERFACE_FUNCTION(__sanitizer_print_stack_trace)
INTERFACE_FUNCTION(__sanitizer_ptr_cmp)
INTERFACE_FUNCTION(__sanitizer_ptr_sub)
INTERFACE_FUNCTION(__sanitizer_start_switch_fiber)
INTERFACE_FUNCTION(__sanitizer_unaligned_load16)
INTERFACE_FUNCTION(__sanitizer_unaligned_load32)
INTERFACE_FUNCTION(__sanitizer_unaligned_load64)
INTERFACE_FUNCTION(__sanitizer_unaligned_store16)
INTERFACE_FUNCTION(__sanitizer_unaligned_store32)
INTERFACE_FUNCTION(__sanitizer_unaligned_store64)
INTERFACE_FUNCTION(__asan_update_allocation_context)
INTERFACE_WEAK_FUNCTION(__asan_default_options)
INTERFACE_WEAK_FUNCTION(__asan_default_suppressions)
INTERFACE_WEAK_FUNCTION(__asan_on_error)
#ifndef DISABLE_CMASAN
INTERFACE_FUNCTION(CMASanPIEReferencePoint)
#endif