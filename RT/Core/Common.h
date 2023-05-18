#pragma once

#include "ApiTypes.h"

#include <assert.h>

#define RT_ASSERT(x) assert(x)
#define RT_INVALID_DEFAULT_CASE default: { RT_ASSERT(!"Invalid default case!"); } break;

#define ALWAYS(x) (RT_ASSERT(x), x)
#define NEVER(x)  (RT_ASSERT(!(x)), x)

#define RT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define RT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define RT_CLAMP(X, Min, Max) ((X) < (Min) ? (Min) : (X) > (Max) ? (Max) : (X))

#define RT_BITN(n) (1u << (n))
#define RT_SET_FLAG(flags, n, x) (x ? flags | (n) : flags & ~(n))

#define RT_ZERO_STRUCT(x) (memset(x, 0, sizeof(*(x))))

#define RT_ALIGN_POW2(x, align)      ((intptr_t)(x) + ((align) - 1) & (-(intptr_t)(align)))
#define RT_ALIGN_DOWN_POW2(x, align) ((intptr_t)(x) & (-(intptr_t)(align)))

#define RT_PASTE_(a, b) a##b
#define RT_PASTE(a, b) RT_PASTE_(a, b)

#define RT_STRINGIFY_(a) #a
#define RT_STRINGIFY(a) RT_STRINGIFY_(a)
#define RT_STRINGIFY_WIDE(a) L"" RT_STRINGIFY(a)

#define RT_PAD(n) char RT_PASTE(pad__, __LINE__)[n]

// TODO: do freaky template version for C++?
#define RT_ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

#define RT_KB(x) ((size_t)(x) << 10)
#define RT_MB(x) ((size_t)(x) << 20)
#define RT_GB(x) ((size_t)(x) << 30)

#define RT_SLL_PUSH(head, node) ((node)->next = (head), (head) = (node))
#define RT_SLL_POP(head) head; do { (head) = (head)->next; } while(0)

#define RT_DLL_PUSH_BACK(first, last, node) \
	do {                                    \
		(node)->prev = (last);              \
		if (first)                          \
		{                                   \
			(last) = (last)->next = (node); \
		}                                   \
		else                                \
		{                                   \
			(first) = (last) = (node);      \
		}                                   \
	} while(0)

#define RT_DLL_REMOVE(first, last, node)                     \
	do {                                                     \
		if ((node)->prev) (node)->prev->next = (node)->next; \
		if ((node)->next) (node)->next->prev = (node)->prev; \
		if ((node) == (last))  (last)  = (node)->prev;       \
		if ((node) == (first)) (first) = (node)->next;       \
	} while(0)

#define RT_SWAP(type, a, b) do { type temp__ = a; a = b; b = temp__; } while (0)

#ifdef __cplusplus

template <typename T>
struct DeferDoodad
{
    T lambda;
    DeferDoodad(T lambda) : lambda(lambda) {}
    ~DeferDoodad() { lambda(); }
};

struct DeferDoodadHelp
{
    template <typename T>
    DeferDoodad<T> operator + (T t) { return t; }
};

#define defer const auto RT_PASTE(defer_, __LINE__) = DeferDoodadHelp() + [&]()

#endif