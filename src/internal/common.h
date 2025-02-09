/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   common.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:14:17 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 20:08:47 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <stdbool.h>
# include <immintrin.h>

# include "extensions.h"
# include "tags.h"
# include "structs.h"
# include "errors.h"

# define STR_LEN(s) (sizeof(s) - 1)
# define STR_AND_LEN(s) s, STR_LEN(s)
# define MAX_ERROR_MESSAGE_SIZE 256

HOT uint8_t compute_checksum(const char *buffer, const uint16_t len);

#endif