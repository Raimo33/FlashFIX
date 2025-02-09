/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:52:22 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 19:53:15 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERRORS_H
# define ERRORS_H

# include <stdint.h>
# include <stdbool.h>

# include "extensions.h"

# define static_assert _Static_assert

extern __Thread_local char last_error_message[MAX_ERROR_MESSAGE_SIZE];

HOT extern inline void dynamic_assert(const bool condition, const char *message);
HOT extern inline void panic(const char *message);

#endif