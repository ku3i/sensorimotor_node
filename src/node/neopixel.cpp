#include "neopixel.hpp"


// Actually send a bit to the string. We must to drop to asm to enusre that the complier does
// not reorder things and make it so the delay happens in the wrong place.

inline void sendBit( bool bitVal ) {

    if (  bitVal ) {				// 0 bit

		asm volatile (
			"sbi %[port], %[bit] \n\t"				// Set the output bit
			".rept %[onCycles] \n\t"                                // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]		"I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]		"I" (PIXEL_BIT),
			[onCycles]	"I" (NS_TO_CYCLES(T1H) - 2),		// 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
			[offCycles] 	"I" (NS_TO_CYCLES(T1L) - 2)			// Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness

		);

    } else {					// 1 bit

		// **************************************************************************
		// This line is really the only tight goldilocks timing in the whole program!
		// **************************************************************************


		asm volatile (
			"sbi %[port], %[bit] \n\t"				// Set the output bit
			".rept %[onCycles] \n\t"				// Now timing actually matters. The 0-bit must be long enough to be detected but not too long or it will be a 1-bit
			"nop \n\t"                                              // Execute NOPs to delay exactly the specified number of cycles
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]		"I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]		"I" (PIXEL_BIT),
			[onCycles]	"I" (NS_TO_CYCLES(T0H) - 2),
			[offCycles]	"I" (NS_TO_CYCLES(T0L) - 2)

		);

    }

    // Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the 5us reset timeout (which is A long time)
    // Here I have been generous and not tried to squeeze the gap tight but instead erred on the side of lots of extra time.
    // This has thenice side effect of avoid glitches on very long strings becuase


}


inline void sendByte( unsigned char byte ) {
    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {
      cli();
      sendBit( bitRead( byte , 7 ) );                // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc
      sei();
    }
}



void ledsetup() {
  bitSet(PIXEL_DDR , PIXEL_BIT);
}


inline void sendPixel( unsigned char r, unsigned char g , unsigned char b ) {
  sendByte(g);          // Neopixel wants colors in green then red then blue order
  sendByte(r);
  sendByte(b);
}


void show() {
	_delay_us( (RES / 1000UL) + 1);	// Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
}


/* EXAMPLE USAGE */

// Display a single color on the whole string

void showColor( unsigned char r , unsigned char g , unsigned char b ) {

  //cli();
  for( int p=0; p<PIXELS; p++ ) {
    sendPixel(r, g, b);
  }
  //sei();
  show();

}



// Fill the dots one after the other with a color
// rewrite to lift the compare out of the loop
void colorWipe(unsigned char r , unsigned char g, unsigned char b, unsigned  char wait ) {
  for(unsigned int i=0; i<PIXELS; i+= (PIXELS/60) ) {

    cli();
    unsigned int p=0;

    while (p++<=i) {
        sendPixel(r,g,b);
    }

    while (p++<=PIXELS) {
        sendPixel(0,0,0);

    }

    sei();
    show();
    delay(wait);
  }
}




#define THEATER_SPACING (PIXELS/20)
void theaterChase( unsigned char r , unsigned char g, unsigned char b, unsigned char wait ) {

  for (int j=0; j< 3 ; j++) {

    for (int q=0; q < THEATER_SPACING ; q++) {

      unsigned int step=0;

      cli();

      for (int i=0; i < PIXELS ; i++) {

        if (step==q) {

          sendPixel( r , g , b );

        } else {

          sendPixel( 0 , 0 , 0 );

        }

        step++;

        if (step==THEATER_SPACING) step =0;

      }

      sei();

      show();
      delay(wait);

    }

  }

}



void rainbowCycle(unsigned char frames , unsigned int frameAdvance, unsigned int pixelAdvance ) {

  // Hue is a number between 0 and 3*256 than defines a mix of r->g->b where
  // hue of 0 = Full red
  // hue of 128 = 1/2 red and 1/2 green
  // hue of 256 = Full Green
  // hue of 384 = 1/2 green and 1/2 blue
  // ...

  unsigned int firstPixelHue = 0;     // Color for the first pixel in the string

  for(unsigned int j=0; j<frames; j++) {

    unsigned int currentPixelHue = firstPixelHue;

    cli();

    for(unsigned int i=0; i< PIXELS; i++) {

      if (currentPixelHue>=(3*256)) {                  // Normalize back down incase we incremented and overflowed
        currentPixelHue -= (3*256);
      }

      unsigned char phase = currentPixelHue >> 8;
      unsigned char step = currentPixelHue & 0xff;

      switch (phase) {

        case 0:
          sendPixel( ~step , step ,  0 );
          break;

        case 1:
          sendPixel( 0 , ~step , step );
          break;

        case 2:
          sendPixel(  step ,0 , ~step );
          break;

      }

      currentPixelHue+=pixelAdvance;


    }

    sei();

    show();

    firstPixelHue += frameAdvance;

  }
}



void detonate( unsigned char r , unsigned char g , unsigned char b , unsigned int startdelayms) {
  while (startdelayms) {

    showColor( r , g , b );      // Flash the color
    showColor( 0 , 0 , 0 );

    delay( startdelayms );

    startdelayms =  ( startdelayms * 4 ) / 5 ;           // delay between flashes is halved each time until zero

  }

  // Then we fade to black....

  for( int fade=256; fade>0; fade-- ) {

    showColor( (r * fade) / 256 ,(g*fade) /256 , (b*fade)/256 );

  }

  showColor( 0 , 0 , 0 );
}
