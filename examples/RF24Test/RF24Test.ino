/**
 * Example for Getting Started with nRF24L01+ radios.
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting
 * with the serial monitor and sending a 'T'.  The ping node sends the current
 * time to the pong node, which responds by sending the value back.  The ping
 * node can then see how long the whole cycle took.

  Atom                 NRF24L01+
  GND                   1 (GND)
  3V3 (3.3V)            2 (3V3)
  D6 (CSN)              3 (CE)
  A4 (SS)               4 (CSN)
  A5 (SCK)              5 (SCK)
  A6 (MOSI)             6 (MOSI)
  A7 (MISO)             7 (MISO)
 */


#include <nRF24L01.h>

nRF24L01 radio(D0,A0);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.
//

// The various roles supported by this sketch
typedef enum { role_ping_out = 1, role_pong_back } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role = role_pong_back; // Start as a Receiver

void setup(void)
{
    //
    // Print preamble
    //

    Serial.begin(57600); // make sure serial terminal is closed before booting the Core
    while(!Serial.available()); // wait for user to open serial terminal and press enter
    SERIAL("\n\rSparkCore-RF24/Examples/GettingStartedRF24\n\r");
    SERIAL("ROLE: RECEIVING\n\r");
    SERIAL("*** PRESS 'T' to begin transmitting to the other node\n\r");

    //
    // Setup and configure rf radio
    //

    radio.begin();

    // optionally, uncomment to increase the delay between retries & # of retries.
    // delay is in 250us increments (4ms max), retries is 15 max.
    //radio.setRetries(15,15);

    // optionally, uncomment to reduce the payload size.
    // seems to improve reliability.
    //radio.setPayloadSize(8);

    //
    // Open pipes to other nodes for communication
    //

    // This simple sketch opens two pipes for these two nodes to communicate
    // back and forth.
    // Open 'our' pipe for writing
    // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

    if ( role == role_ping_out )
    {
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]);
    }
    else
    {
        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(1,pipes[0]);
    }

    //
    // Start listening
    //

    radio.startListening();

    //
    // Dump the configuration of the rf unit for debugging
    //

    radio.printDetails();
}

void loop(void)
{
    //
    // Ping out role.  Repeatedly send the current time
    //

    if (role == role_ping_out)
    {
        // Switch to a Receiver before each transmission,
        // or this will only transmit once.
        radio.startListening();

        // Re-open the pipes for Tx'ing
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]);

        // First, stop listening so we can talk.
        radio.stopListening();

        // Take the time, and send it.  This will block until complete
        unsigned long time = millis();
        SERIAL("Now sending %lu...",time);
        bool ok = radio.write( &time, sizeof(unsigned long) );

        if (ok)
            SERIAL("ok...\n\r");
        else
            SERIAL(" failed.\n\r");

        // Try again 1s later
        delay(100);
    }

    //
    // Pong back role.  Receive each packet, dump it out, and send it back
    //

    if ( role == role_pong_back )
    {
        // if there is data ready
        if ( radio.available() )
        {
            // Dump the payloads until we've gotten everything
            unsigned long got_time;
            bool done = false;
            while (!done)
            {
                // Fetch the payload, and see if this was the last one.
                done = radio.read( &got_time, sizeof(unsigned long) );

                // Spew it
                //printf("Got payload %lu...\n\r",got_time);
                Serial.print("Got payload "); Serial.println(got_time);

                // Delay just a little bit to let the other unit
                // make the transition to receiver
                delay(20);
            }

            // Switch to a transmitter after each received payload
            // or this will only receive once
            radio.stopListening();

            // Re-open the pipes for Rx'ing
            radio.openWritingPipe(pipes[1]);
            radio.openReadingPipe(1,pipes[0]);

            // Now, resume listening so we catch the next packets.
            radio.startListening();
        }
    }

    //
    // Change roles
    //

    if ( Serial.available() )
    {
        char c = toupper(Serial.read());
        if ( c == 'T' && role == role_pong_back )
        {
            SERIAL("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");

            // Become the primary transmitter (ping out)
            role = role_ping_out;
            radio.openWritingPipe(pipes[0]);
            radio.openReadingPipe(1,pipes[1]);
            radio.stopListening();
        }
        else if ( c == 'R' && role == role_ping_out )
        {
            SERIAL("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");

            // Become the primary receiver (pong back)
            role = role_pong_back;
            radio.openWritingPipe(pipes[1]);
            radio.openReadingPipe(1,pipes[0]);
            radio.startListening();
        }
        else if ( c == 'P' )
        {
            SERIAL("*** PRINTING DETAILS\n\r");
            radio.printDetails();
        }
    }
}
