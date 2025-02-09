/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 20:13:31 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 20:16:15 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_ERRORS_H
# define FLASHFIX_ERRORS_H

# include "../src/errors.h"

COLD void ff_get_error(char *restrict buffer, const uint16_t buffer_size);
COLD const char *ff_get_error(void); 

#endif