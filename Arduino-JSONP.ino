/* jsonp-server.ino - demonstrates jsfiddle.com exchanging data with Arduino server
 *
 * To be used with http://jsfiddle.net/user/dashboard/
 * Details at http://www.coertvonk.com/technology/arduino/arduino-webserver-using-jquerymobile-and-jsfiddle-11803
 * Compiler: Arduino IDE 1.6.3, requires C++11
 * Hardware: EtherShield(w5100) and Arduino Ethernet (w5200) with a few tweaks
 * Inspiration by: http://playground.arduino.cc/Code/WebServerST
 *
 * To support four clients connecting to the server, modify the Ethernet library according to
 *   http://subethasoftware.com/2013/04/09/arduino-ethernet-and-multiple-socket-server-connections/
 * This might cause received data to be overwritten, and result in requests like
 *   GET /?callbac?callback=jQuery210049570703599601984_1431443159249&_=1431443159250 HTTP/1.1
 *
 * 2015, Coert Vonk.  MIT License.  */

#include <SPI.h>
#include <Time.h>
#include <Ethernet.h>  // see comment above
#include <utility/w5100.h>
#include <utility/socket.h>

namespace {

    // MAC address must be unique and statically mapped by DHCPd, so the IP addr is predictable
    byte _mac[] = {0x00, 0xC0, 0x7B, 0x00, 0x00, 0x00};  // reuse our ASND range

    enum class ioType_t {  // requires C++11
        digitalInput,
        digitalInputPullup,
        digitalOutput,
        digitalOutputPWM,
        analogInput
    };

    struct tIO {
        char const * const id;
        uint8_t pin;
        ioType_t typ;
        uint8_t value;
    } _io[] = {
        {"r", 5, ioType_t::digitalOutputPWM, 0}, // Digital PWM output 5 on Arduino via 150 Ohm to Red LED pin
        {"g", 6, ioType_t::digitalOutputPWM, 0}, // Digital PWM output 6 on Arduino via 68 Ohm to Green LED pin
        {"b", 9, ioType_t::digitalOutputPWM, 0}  // Digital PWM output 9 on Arduino via 68 Ohm to Blue LED pin
    };                                           // GND to Cathode LED pin
    uint8_t const IO_COUNT = sizeof( _io ) / sizeof( _io[0] );

    void _skipPostParameters( EthernetClient & client )
    {
        while ( client.available() ) {
            (void)client.read();
        }
    }

    void _parseHTTPparameters( char * line, char * * urlPtr )
    {
        *urlPtr = (char *)"";

        char * p;
        p = strtok( line, " " ); // ignore METHOD
        p = strtok( NULL, " " ); *urlPtr = p;
        p = strtok( NULL, " " ); // ignore PROTOCOL
    }

    void _receiveJsonpRequest( char * url, char * * callbackPtr )
    {
        *callbackPtr = (char *)"";

        char * p = strtok( url, "?" ); // PATH

        while ( (p = strtok( NULL, "&" )) != NULL ) {

            char * callbackId = (char *)F( "callback" );
            uint8_t const len = strlen( callbackId );
            if ( strncmp( p, callbackId, len ) == 0 && p[len] == '=' ) {
                *callbackPtr = p + len + 1;
            } else {
                for ( uint8_t ii = 0; ii < IO_COUNT; ii++ ) {
                    tIO * const io = &_io[ii];
                    uint8_t const len = strlen( io->id );
                    if ( strncmp( p, io->id, len ) == 0 && p[len] == '=' ) {

                        io->value = atoi( p + len + 1 );  // skip just past the = sign
                        switch ( io->typ ) {
                            case ioType_t::digitalInput:
                            case ioType_t::digitalInputPullup:
                                break;
                            case ioType_t::digitalOutput:
                                digitalWrite( io->pin, io->value );
                                break;
                            case ioType_t::digitalOutputPWM:
                                analogWrite( io->pin, io->value );
                                break;
                            case ioType_t::analogInput:
                                break;
                        }
                        Serial.print( "rx { " ); Serial.print( io->id ); Serial.print( ": " );
                        Serial.print( io->value ); Serial.println( " }" );
                    }
                }
            }
        }  // while
    }

    void _sendJsonpResponse( EthernetClient & client, char const * const callback )
    {
        client.println( F( "HTTP/1.1 200 OK" ) );
        client.println( F( "Content-Type: application/json" ) );
        client.println();

        String jsonp = callback;
        jsonp += "({ ";
        for ( uint8_t ii = 0; ii < IO_COUNT; ii++ ) {
            tIO * const io = &_io[ii];

            switch ( io->typ ) {
                case ioType_t::digitalInput:
                    io->value = digitalRead( io->pin );
                    break;
                case ioType_t::digitalInputPullup:
                    io->value = !digitalRead( io->pin );
                    break;  // inverse
                case ioType_t::digitalOutput:
                case ioType_t::digitalOutputPWM:
                    break;
                case ioType_t::analogInput:
                    io->value = analogRead( io->pin );
                    break;
            }
            jsonp += io->id;
            jsonp += ": ";
            jsonp += io->value;
            if ( ii < IO_COUNT - 1 ) {
                jsonp += ", ";
            }
        }
        jsonp += " })";
        client.print( jsonp );
        Serial.print( "tx " ); Serial.println( jsonp );
    }

    EthernetServer server( 80 );
}  // name space


void setup()
{
    Serial.begin( 115200 );

    pinMode( 4, OUTPUT ); // disable SD card when not in use, otherwis Client.connect() starts failing after a while
    digitalWrite( 4, 1 );

    for ( uint8_t ii = 0; ii < IO_COUNT; ii++ ) {
        tIO * const io = &_io[ii];

        switch ( io->typ ) {
            case ioType_t::digitalInput:
                pinMode( io->pin, INPUT );
                break;
            case ioType_t::digitalInputPullup:
                pinMode( io->pin, INPUT_PULLUP );
                break;
            case ioType_t::digitalOutput:
            case ioType_t::digitalOutputPWM:
                pinMode( io->pin, OUTPUT );
                break;
            case ioType_t::analogInput:
                break;
        }
    }
    delay( 50 );  // give the Wiznet W5100 an additional 50 msec to come out of reset

    if ( Ethernet.begin( _mac ) == 0 ) {
        Serial.println( F( "Failed to configure Ethernet using DHCP" ) );
        for ( ;; ) {
            ;
        }
    }
    delay( 2000 );  // make it more like that server gets up and running
    server.begin();
    Serial.print( F( "Server running on " ) );
    Serial.println( Ethernet.localIP() );
}

void loop()
{
    EthernetClient client = server.available();
    if ( client ) {
        boolean currentLineIsBlank = true;
        boolean currentLineIsGet = true;
        time_t start = now();

        while ( client.connected() ) {

            uint8_t cnt = 0;
            char line[128];  // must fit the first HTTP line
            char * callback;
            char * url;

            while ( client.available() ) {

                start = now();  // reset timeout on packet received
                char c = client.read();

                if ( currentLineIsGet && cnt < sizeof( line ) - 1 ) {
                    line[cnt++] = c;
                }
                if ( c == '\n' ) {
                    if ( currentLineIsBlank ) {
                        line[cnt - 1] = '\0';
                        _skipPostParameters( client );
                        _parseHTTPparameters( line, &url );
                        Serial.println( url );

                        char const * const jsonpPath = "/jsonp";
                        if ( strncmp( jsonpPath, url, strlen( jsonpPath ) ) == 0 ) {
                            _receiveJsonpRequest( url, &callback );
                            _sendJsonpResponse( client, callback );
                            Serial.println( "done" );
                        } else {
                            Serial.println( "Unknown URL=" );
                            Serial.println( url );
                        }
                        client.stop();
                        return;

                    } else {
                        currentLineIsBlank = true;
                        currentLineIsGet = false;
                    }
                } else if ( c != '\r' ) {
                    currentLineIsBlank = false;
                }
            }  // while client.available

            if ( now() - start > 2 ) {  // disconnect idle client
                Serial.println( "\nTimeout" );
                break;
            }
        }
        client.stop();
    }
}
