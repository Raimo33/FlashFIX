#ifndef FLASHFIX_ERRORS_H
# define FLASHFIX_ERRORS_H

typedef enum
{
  FF_OK,
  FF_INVALID_MESSAGE,
  FF_CONTENT_LENGTH_MISMATCH,
  FF_CHECKSUM_MISMATCH,
  FF_BUFFER_TOO_SMALL
} ff_error_t;

#endif