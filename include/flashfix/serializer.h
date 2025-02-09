/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serializer.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 18:57:56 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 19:33:47 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include "../src/common.h"

HOT uint16_t serialize_fix_message(char *restrict buffer, const uint16_t buffer_size, const fix_message_t *restrict message);
HOT uint16_t finalize_fix_message(char *restrict buffer, const uint16_t buffer_size, const uint16_t len);

#endif