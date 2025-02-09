/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 20:17:34 by craimond          #+#    #+#             */
/*   Updated: 2025/02/09 20:19:09 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "internal/errors.h"

__Thread_local char last_error_message[MAX_ERROR_MESSAGE_SIZE];

//TODO capire internal/external separation di file con stesso nome