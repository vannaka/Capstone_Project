// #ifndef hdlc_h
#define hdlc_h

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>
#include <functional>
// #include "../../circular_buffer/circular_buffer.h"

/******************************************************************************
 *                                 Defines
 *****************************************************************************/


/******************************************************************************
 *                               Global Types
 *****************************************************************************/

typedef std::function<void(uint8_t)> send_hdnlr_t;
typedef std::function<void(uint8_t*, uint8_t)> recv_hndlr_t;

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
    Hdlc( uint16_t max_data_length );

    void set_send_hndlr( send_hdnlr_t const & send_byte_hndlr );
    void set_rcv_hndlr( recv_hndlr_t const & recv_hndlr );

    void byte_receive( uint8_t data );
    void send_frame( uint8_t const * const buffer, uint8_t length );

private:    
    void send_byte( uint8_t data );
    void send_boundry_byte();

    send_hdnlr_t m_send_byte_hndlr;
    recv_hndlr_t m_recv_frame_hndlr;
        
    bool escape_character;
    uint8_t *receive_frame_buffer;
    uint8_t frame_position;
    uint16_t frame_checksum;
    uint16_t max_frame_length;

    // Circular_Buffer out_buf;
};

// #endif