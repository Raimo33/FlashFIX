/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   common.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/10 12:19:21 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:48:46 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <stdbool.h>
# include <immintrin.h>

# include "extensions.h"
# include "tags.h"

# define STR_LEN(x) (sizeof(x) - 1)

uint8_t compute_checksum(const char *buffer, const uint16_t len); 

#endif