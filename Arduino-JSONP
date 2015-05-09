/* webserverst.ino - demonstrates jsfiddle exchaning data with Arduino server
 *
 * To be used with http://jsfiddle.net/cvonk/jr5utgsh/44/
 * Details at http://www.coertvonk.com/technology/arduino/arduino-webserver-using-jquerymobile-and-jsfiddle-11803
 * Compiler: Arduino IDE 1.6.3
 * Hardware: EtherShield(w5100) or possibly Ether(w5200)
 *
 * To support four clients connecting to the server, modify the Ethernet library according to
 *   http://subethasoftware.com/2013/04/09/arduino-ethernet-and-multiple-socket-server-connections/
 *
 * 2015, Coert Vonk.  MIT License.
 */

// MAC address must be unique and statically mapped by DHCPd, so the IP addr is predictable
byte mac[] = {0x00, 0xC0, 0x7B, 0x00, 0x00, 0x00};  // reuse our ASND range

struct tRGB  {
    char const * const id;
    uint8_t value;
    uint8_t dPort;
} _rgb[] = {      // Cathode LED pin to GND
    {"r", 20, 5}, // Red     LED pin via 150 Ohm to Digital output 5 on Arduino
    {"g", 20, 6}, // Green   LED pin via 68 Ohm to Digital output 6 on Arduino
    {"b", 20, 9}  // Blue    LED pin via 68 Ohm to Ditial output 9 on Arduino
};
uint8_t const RGB_COUNT = sizeof( _rgb ) / sizeof( _rgb[0] );

void
_skipPostParameters( EthernetClient & client )
{
    while ( client.available() ) {
        (void)client.read();
    }
}


void
_parseURLparameters( char * p, char * * callbackPtr, char * * urlPtr, char * * cmdPtr)
{
    *callbackPtr = *urlPtr = *cmdPtr = (char *)"";

    p = strtok( p,    " " ); *cmdPtr = p;
    p = strtok( NULL, "?" ); *urlPtr = p;

    while ( (p = strtok( NULL, "&" )) != NULL ) {

        char * callbackId = (char *)"callback";
        uint8_t const len = strlen( callbackId );
        if ( strncmp( p, callbackId, len ) == 0 && p[len] == '=' ) {
            *callbackPtr = p + len + 1;
        } else {
            for ( uint8_t ii = 0; ii < RGB_COUNT; ii++ ) {
                tRGB * const rgb = &_rgb[ii];
                uint8_t const len = strlen( rgb->id );
                if ( strncmp( p, rgb->id, len ) == 0 && p[len] == '=' ) {
                    rgb->value = atoi( p + len + 1 );  // skip just past the = sign
                    Serial.print( "rx { " ); Serial.print( rgb->id ); Serial.print( ": " );
                    Serial.print( rgb->value ); Serial.println( " }" );
                }
            }
        }
    }  // while
}


void
_sendJsonpResponse( EthernetClient & client, char const * const callback )
{
    client.println( "HTTP/1.1 200 OK" );
    client.println( "Content-Type: application/json" );
    client.println();

    String jsonp = callback;
    jsonp += "({ ";
    for ( uint8_t ii = 0; ii < RGB_COUNT; ii++ ) {
        tRGB * const rgb = &_rgb[ii];
        jsonp += rgb->id; 
        jsonp += ": "; 
        jsonp += rgb->value;
        if ( ii < RGB_COUNT - 1 ) {
            jsonp += ", ";
        }
    }
    jsonp += " })";
    client.print( jsonp );
    Serial.print( "tx " ); Serial.println( jsonp );
}


EthernetServer server( 80 );


void setup()
{
    Serial.begin( 115200 );

    for ( uint8_t ii = 0; ii < RGB_COUNT; ii++ ) {  // not needed, dflt is OUTPUT
        tRGB * const rgb = &_rgb[ii];
        pinMode( rgb->dPort, OUTPUT );
    }

    delay( 50 );  // allow some time (50 ms) after powerup and sketch start, for the Wiznet W5100 Reset IC to release and come out of reset.
 
    if ( Ethernet.begin( mac ) == 0 ) {
        Serial.println( F("Failed to configure Ethernet using DHCP") );
        for ( ;; ) {
            ;
        }
    }
    delay( 2000 );  // make it more like that server gets up and running
    server.begin();
    Serial.print( F("Server running on ") );
    Serial.println( Ethernet.localIP() );
}


void loop()
{
    EthernetClient client = server.available();
    if ( client ) {
        boolean currentLineIsBlank = true;
        boolean currentLineIsGet = true;
        uint8_t cnt = 0;
        char getStr[128];  // must be big enough for first HTTP line
        char * callback;
        char * cmd;
        char * url;
        time_t start = now();

        while ( client.connected() ) {
            while ( client.available() ) {

                start = now();
                char c = client.read();

                if ( currentLineIsGet && cnt < sizeof(getStr) - 1 ) {
                    getStr[cnt++] = c;
                }
                if ( c == '\n' && currentLineIsBlank ) {
                    getStr[cnt] = '\0';
                    _skipPostParameters( client );
                    _parseURLparameters( getStr, &callback, &cmd, &url );
                    _sendJsonpResponse( client, callback );
                    client.stop();

                } else if ( c == '\n' ) {
                    currentLineIsBlank = true;
                    currentLineIsGet = false;

                } else if ( c != '\r' ) {
                    currentLineIsBlank = false;
                }
            }
            // disconnect client that has been idle for >1 seconds
            if ( now() - start > 2 ) {
                Serial.println( "\nTimeout" );
                break;
            }
        }
        client.stop();
    }

    for ( uint8_t ii = 0; ii < RGB_COUNT; ii++ ) {
        tRGB * const rgb = &_rgb[ii];
        analogWrite( rgb->dPort, rgb->value );
    }
}
