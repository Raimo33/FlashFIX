/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   flashfix.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 18:52:50 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 17:21:00 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_H
# define FLASHFIX_H

# ifndef FIX_VERSION
#   define FIX_VERSION "FIX.4.4"
# endif

# include "flashfix/serializer.h"
# include "flashfix/deserializer.h"

/***************************************************************************************

NOTE: this is a serializer, not a parser. It will not check if the message is correct.

***************************************************************************************/

#endif