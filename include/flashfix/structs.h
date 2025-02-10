/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   structs.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:51:14 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:48:08 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

# ifndef FIX_MAX_FIELDS
#   define FIX_MAX_FIELDS 64
# endif

typedef struct __attribute__((aligned(64)))
{
  const char *tag;
  const char *value;
  uint16_t tag_len;
  uint16_t value_len;
} fix_field_t;

typedef struct __attribute__((aligned(64)))
{
  fix_field_t fields[FIX_MAX_FIELDS];
  uint8_t n_fields;
} fix_message_t;

#endif