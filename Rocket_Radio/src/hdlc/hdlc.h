#ifndef hdlc_h
#define hdlc_h

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 *                                 Defines
 *****************************************************************************/


/******************************************************************************
 *                               Global Types
 *****************************************************************************/

// Labels what's in the data field of the HDLC frame.
typedef uint8_t data_type_t;
enum
{
    SENSOR_DATA = 0,
    RND_MAX = 1,
};

typedef void (*send_byt_hndlr_t)( uint8_t byte );
typedef void (*rcvd_data_hndlr_t)( data_type_t data_type, const uint8_t *buffer, uint16_t length );

/******************************************************************************
 *                          Function Declarations
 *****************************************************************************/


/******************************************************************************
 *                                Classes
 *****************************************************************************/

/**********************************************************
*   Hdlc
*       Used to wrap data in an HDLC frame for transmission
*       and to parse HDLC frames byte by byte as they're 
*       received.
**********************************************************/
class Hdlc
{
public:
    Hdlc( send_byt_hndlr_t send_byte_handler, rcvd_data_hndlr_t receive_data_handler, uint16_t max_frame_length );

    void byte_receive( uint8_t data );
    void send_frame( data_type_t data_type, const uint8_t *buffer, uint8_t length );

private:
    send_byt_hndlr_t send_byte_handler;
    rcvd_data_hndlr_t receive_data_handler;
    
    void send_byte( uint8_t data );
    void send_boundry_byte();
        
    bool escape_character;
    uint8_t *receive_frame_buffer;
    uint8_t frame_position;
    uint16_t frame_checksum;
    uint16_t max_frame_length;
};

#endif