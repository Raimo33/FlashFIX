/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serializer.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 18:57:56 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:53:44 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>
# include <stdbool.h>

# include "structs.h"
# include "errors.h"

uint16_t serialize_fix_message(char *restrict buffer, const uint16_t buffer_size, const fix_message_t *restrict message, ff_error_t *restrict error);
uint16_t finalize_fix_message(char *restrict buffer, const uint16_t buffer_size, const uint16_t len, ff_error_t *restrict error);

#endif