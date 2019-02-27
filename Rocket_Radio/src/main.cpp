#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <TaskScheduler.h>

/******************************************************************************
 *                                 Defines
 *****************************************************************************/
#define SerialXbee Serial1

/******************************************************************************
 *                               Global Types
 *****************************************************************************/


/******************************************************************************
 *                          Function Declarations
 *****************************************************************************/
void dataSend();
void dataReceive();

/******************************************************************************
 *                               Global Vars
 *****************************************************************************/
// Scheduler / Tasks
Scheduler scheduler;
Task t1( 100, TASK_FOREVER, dataSend );
Task t2( 100, TASK_FOREVER, dataReceive );

uint8_t rand_max = 255;

/******************************************************************************
 *                                 Procedures
 *****************************************************************************/

void setup()
{
    Serial.begin( 9600 );
    SerialXbee.begin( 9600 );

    randomSeed( analogRead(0) );

    // Setup cooperative scheduler
    scheduler.init();
    scheduler.addTask( t1 );
    scheduler.addTask( t2 );

    t1.enable();
    t2.enable();
}

void loop()
{
    scheduler.execute();
}


/**********************************************************
*   dataTask
*       Called every 100ms. Sends out data to ground
*       station.
**********************************************************/
void dataSend()
{
    uint8_t rand = random( rand_max );

    SerialXbee.write( rand );

}   /* dataTask() */

void dataReceive()
{
    if( SerialXbee.available() > 0 )
    {
        rand_max = SerialXbee.read();
        Serial.println( rand_max );
    }
}