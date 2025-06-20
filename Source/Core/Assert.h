#pragma once

#if defined(DM_ASSERT)
#  error "DM_ASSERT already defined"
#endif // defined(DM_ASSERT)

#define PPK_ASSERT_ENABLED DM_WITH_DEBUG_UTILS
#define PPK_ASSERT_DISABLE_STL // Reduce compile times

#include <ppk_assert.h>

#define DM_ASSERT                PPK_ASSERT
#define DM_ASSERT_WARNING        PPK_ASSERT_WARNING
#define DM_ASSERT_DEBUG          PPK_ASSERT_DEBUG
#define DM_ASSERT_ERROR          PPK_ASSERT_ERROR
#define DM_ASSERT_FATAL          PPK_ASSERT_FATAL
#define DM_ASSERT_CUSTOM         PPK_ASSERT_CUSTOM
#define DM_ASSERT_USED           PPK_ASSERT_USED
#define DM_ASSERT_USED_WARNING   PPK_ASSERT_USED_WARNING
#define DM_ASSERT_USED_DEBUG     PPK_ASSERT_USED_DEBUG
#define DM_ASSERT_USED_ERROR     PPK_ASSERT_USED_ERROR
#define DM_ASSERT_USED_FATAL     PPK_ASSERT_USED_FATAL
#define DM_ASSERT_USED_CUSTOM    PPK_ASSERT_USED_CUSTOM

#define DM_STATIC_ASSERT         PPK_STATIC_ASSERT
