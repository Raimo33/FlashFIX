/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 20:13:31 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 13:39:07 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_ERRORS_H
# define FLASHFIX_ERRORS_H

typedef enum
{
  FF_OK,
  FF_NULL_POINTER,
  FF_INVALID_MESSAGE,
  FF_CONTENT_LENGTH_MISMATCH,
  FF_CHECKSUM_MISMATCH,
  FF_BUFFER_TOO_SMALL
} ff_error_t;

#endif