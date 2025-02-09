/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extensions.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:53:32 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 19:53:39 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXTENSIONS_H
# define EXTENSIONS_H

# define COLD         __attribute__((cold))
# define HOT          __attribute__((hot))
# define UNLIKELY(x)  __builtin_expect(!!(x), 0)
# define LIKELY(x)    __builtin_expect(!!(x), 1)

#endif