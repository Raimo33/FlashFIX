/*================================================================================

File: flashfix.h                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-12 13:35:28                                                 
last edited: 2025-02-16 19:07:50                                                

================================================================================*/

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

/***************************************************************************************

NOTE: FIX_MAX_FIELDS must be baked into the library at compile time.

***************************************************************************************/

/***************************************************************************************

WARNING: this library expects the host to support unaligned memory access. 

***************************************************************************************/

//TODO explore <stdbit.h> for bit manipulation

#endif