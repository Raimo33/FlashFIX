/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deserializer.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 18:54:25 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 19:33:35 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include "../src/common.h"

HOT bool is_full_fix_message(const char *restrict buffer, const uint16_t buffer_size, const uint16_t message_len);
HOT uint16_t deserialize_fix_message(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message);

#endif