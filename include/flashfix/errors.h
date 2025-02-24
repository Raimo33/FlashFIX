/*================================================================================

File: errors.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-24 16:35:15                                                

================================================================================*/

#ifndef FLASHFIX_ERRORS_H
# define FLASHFIX_ERRORS_H

typedef enum
{
  FF_OK = 0,
  FF_INVALID_MESSAGE,
  FF_BODY_LENGTH_MISMATCH,
  FF_CHECKSUM_MISMATCH,
  FF_TOO_MANY_FIELDS,
  FF_MESSAGE_TOO_BIG,
  FF_WOULD_BLOCK,
  FF_IO_ERROR
} ff_error_t;

#endif