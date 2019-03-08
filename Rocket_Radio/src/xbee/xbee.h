#ifndef XBEE_H
#define XBEE_H

#include <Arduino.h>
#include <stdint.h>

#include "hdlc/hdlc.h"


/******************************************************************************
 *                                   Defines
 *****************************************************************************/
#define MAX_DATA_LENGTH 64


/******************************************************************************
 *                               Global Types
 *****************************************************************************/
// Labels what's in the data field of the HDLC frame.
typedef uint8_t data_type_t;
enum
{
    SENSOR_DATA     = 0,
    RND_MAX         = 1,
    GPS_DATA        = 2,
};

/******************************************************************************
 *                                    Xbee
 *****************************************************************************/
class Xbee
{
public:
    Xbee( HardwareSerial *serial );

    void setup( uint32_t baud_rate );

    void read();
    bool new_data_received();
    void get_data( data_type_t &data_type, uint8_t *buffer, uint8_t &size );
    void send_data( data_type_t data_type, uint8_t const * const buffer, uint8_t size );

private:
    HardwareSerial *m_Serial;
    Hdlc m_hdlc;

    uint8_t m_data[MAX_DATA_LENGTH];
    uint8_t m_data_sz;
    bool m_new_data = false;
};

#endif