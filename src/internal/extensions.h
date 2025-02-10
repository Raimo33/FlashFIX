/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extensions.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:53:32 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:33:16 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXTENSIONS_H
# define EXTENSIONS_H

# define UNLIKELY(x)                __builtin_expect(!!(x), 0)
# define LIKELY(x)                  __builtin_expect(!!(x), 1)
# define PREFETCH(x, rw, priority)  __builtin_prefetch(x, rw, priority)
# define PREFETCHW(x, priority)     __builtin_prefetch(x, 1, priority)
# define PREFETCHR(x, priority)     __builtin_prefetch(x, 0, priority)
# define UNREACHABLE                __builtin_unreachable()
# define FALLTHROUGH                __attribute__((fallthrough))
# define COLD                       __attribute__((cold))
# define HOT                        __attribute__((hot))
# define CONST                      __attribute__((const))
# define PURE                       __attribute__((pure))
# define UNUSED                     __attribute__((unused))
# define PACKED                     __attribute__((packed))
# define ALIGNED(x)                 __attribute__((aligned(x)))
# define NORETURN                   __attribute__((noreturn))
# define ALWAYS_INLINE              __attribute__((always_inline))
# define NO_INLINE                  __attribute__((noinline))
# define FLATTEN                    __attribute__((flatten))
# define MALLOC                     __attribute__((malloc))
# define NONNULL(...)               __attribute__((nonnull(__VA_ARGS__)))

#endif