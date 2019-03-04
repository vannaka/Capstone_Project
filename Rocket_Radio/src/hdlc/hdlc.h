#ifndef hdlc_h
#define hdlc_h

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>

typedef void (*sendchar_type)(uint8_t);
typedef void (*frame_handler_type)(const uint8_t *framebuffer, uint16_t framelength);

class Hdlc
{
public:
  Hdlc( sendchar_type, frame_handler_type, uint16_t max_frame_length );
  void byte_receive( uint8_t data );
  void send_frame( const uint8_t *framebuffer, uint8_t frame_length );

private:
  sendchar_type sendchar_function;
  frame_handler_type frame_handler;
  
  void send_byte(uint8_t data);

  bool escape_character;
  uint8_t *receive_frame_buffer;
  uint8_t frame_position;
  // 16bit CRC sum for _crc_ccitt_update
  uint16_t frame_checksum;
  uint16_t max_frame_length;
};

#endif