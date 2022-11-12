/*
 * Copyright (C) 2012 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.

--------------------------------------------------------------------------------------

CATATAN :

 * > Library DMDESP Updated 26 April 2020 - bonny@elektronmart.com
 * > HJS589(DMD porting for ESP8266 by dmk007) 
 * > DMD (https://github.com/rweather/arduino-projects/tree/master/libraries/DMD)

 * Penggunaan : Copy Folder DMDESP ke Arduino Library
  
 * JANGAN LUPA PESAN HEADER DI ATAS.

 * Silahkan untuk dipergunakan untuk kepentingan Ibadah, Edukasi dan Komersil. Jangan lupa
   jika menggunakan library ini jangan dihapus headernya dan jika ada perbaikan atau
   pengembangan lanjutan ANDA WAJIB untuk MEMBAGIKAN KEMBALI hasilnya ke PUBLIK (GNU 
   General Public License).

 * JANGAN PELIT ILMU.....!!!

--------------------------------------------------------------------------------------*/

#include "DMDESP.h"
#include "Arduino.h"
#include <string.h>
#include <stdlib.h>
#include "Bitmap.h"

/**
 * \class DMD DMD.h <DMD.h>
 * \brief Handle large dot matrix displays composed of LED's.
 *
 * This class is designed for use with
 * <a href="http://www.freetronics.com/dmd">Freetronics Large Dot Matrix
 * Displays</a>.  These displays have 512 LED's arranged in a 32x16 matrix
 * and controlled by an SPI interface.  The displays are available in
 * red, blue, green, yellow, and white variations (for which this class
 * always uses the constant \ref White regardless of the physical color).
 *
 * \section dmd_drawing Drawing
 *
 * DMD inherits from Bitmap so that any of the drawing functions in that
 * class can be used to draw directly to dot matrix displays.  The following
 * example initializes a single display panel and draws a rectangle and a
 * circle into it at setup time:
 *
 * \code
 * #include <DMD.h>
 *
 * DMD display;
 *
 * void setup() {
 *     display.drawRect(5, 2, 27, 13);
 *     display.drawCircle(16, 8, 4);
 * }
 * \endcode
 *
 * The display must be updated frequently from the application's main loop:
 *
 * \code
 * void loop() {
 *     display.loop();
 * }
 * \endcode
 *
 * \section dmd_interrupts Interrupt-driven display refresh
 *
 * The loop() method simplifies updating the display from the application's
 * main loop but it can sometimes be inconvenient to arrange for it to be
 * called regularly, especially if the application wishes to use
 * <tt>delay()</tt> or <tt>delayMicroseconds()</tt>.
 *
 * DMD provides an asynchronous display update mechanism using Timer1
 * interrupts.  The application turns on interrupts using enableTimer1()
 * and then calls refresh() from the interrupt service routine:
 *
 * \code
 * #include <DMD.h>
 *
 * DMD display;
 *
 * ISR(TIMER1_OVF_vect)
 * {
 *     display.refresh();
 * }
 *
 * void setup() {
 *     display.enableTimer1();
 * }
 * \endcode
 *
 * If Timer1 is already in use by some other part of your application,
 * then Timer2 can be used as an alternative interrupt source:
 *
 * \code
 * #include <DMD.h>
 *
 * DMD display;
 *
 * ISR(TIMER2_OVF_vect)
 * {
 *     display.refresh();
 * }
 *
 * void setup() {
 *     display.enableTimer2();
 * }
 * \endcode
 *
 * DMD can also be used with third-party timer libraries such as
 * <a href="http://code.google.com/p/arduino-timerone/downloads/list">TimerOne</a>:
 *
 * \code
 * #include <DMD.h>
 * #include <TimerOne.h>
 *
 * DMD display;
 *
 * void refreshDisplay()
 * {
 *     display.refresh();
 * }
 *
 * void setup() {
 *     Timer1.initialize(5000);
 *     Timer1.attachInterrupt(refreshDisplay);
 * }
 * \endcode
 *
 * \section dmd_double_buffer Double buffering
 *
 * When using interrupts, the system can sometimes exhibit "tearing" artifacts
 * where half-finished images are displayed because an interrupt fired in
 * the middle of a screen update.
 *
 * This problem can be alleviated using double buffering: all rendering is done
 * to an off-screen buffer that is swapped onto the screen once it is ready
 * for display.  Rendering then switches to the other buffer that is now
 * off-screen.  The following example demonstrates this:
 *
 * \code
 * #include <DMD.h>
 *
 * DMD display;
 *
 * ISR(TIMER1_OVF_vect)
 * {
 *     display.refresh();
 * }
 *
 * void setup() {
 *     display.setDoubleBuffer(true);
 *     display.enableTimer1();
 * }
 *
 * void loop() {
 *     updateDisplay();
 *     display.swapBuffers();
 *     delay(50);   // Delay between frames.
 * }
 *
 * void updateDisplay() {
 *     // Draw the new display contents into the off-screen buffer.
 *     display.clear();
 *     ...
 * }
 * \endcode
 *
 * The downside of double buffering is that it uses twice as much main memory
 * to manage the contents of the screen.
 *
 * \section dmd_multi Multiple panels
 *
 * Multiple panels can be daisy-chained together using ribbon cables.
 * If there is a single row of panels, then they must be connected
 * to the Arduino board as follows:
 *
 * \image html dmd-4x1.png
 *
 * If there are multiple rows of panels, then alternating rows are
 * flipped upside-down so that the short ribbon cables provided by
 * Freetronics reach (this technique is thanks to Chris Debenham; see
 * http://www.adebenham.com/category/arduino/dmd/ for more details):
 *
 * \image html dmd-4x2.png
 *
 * This technique can be repeated for as many rows as required, with the
 * bottom row always right-way-up:
 *
 * \image html dmd-4x3.png
 *
 * DMD automatically takes care of flipping the data for panels in the
 * alternating rows.  No special action is required by the user except
 * to physically connect the panels as shown and to initialize the DMD
 * class appropriately:
 *
 * \code
 * #include <DMD.h>
 *
 * DMD display(4, 2);   // 4 panels wide, 2 panels high
 * \endcode
 */

// Pins on the DMDESP connector board.
#define DMD_PIN_PHASE_LSB       16    //D0 // A 
#define DMD_PIN_PHASE_MSB       12    //D6 // B 
#define DMD_PIN_LATCH           0     //D3 // SCLK 
#define DMD_PIN_OUTPUT_ENABLE   15    //D8 // nOE
#define DMD_PIN_SPI_MOSI        13    //D7 // R SPI Master Out, Slave In 
#define DMD_PIN_SPI_SCK         14    //D5 // CLK SPI Serial Clock 

// Dimension information for the display.
#define DMDESP_NUM_COLUMNS      32    // Number of columns in a panel.
#define DMDESP_NUM_ROWS         16    // Number of rows in a panel.

// Refresh times.
//#define DMDESP_REFRESH_MS       100
#define DMDESP_REFRESH_US       100

/**
 * \brief Constructs a new dot matrix display handler for a display that
 * is \a widthPanels x \a heightPanels in size.
 *
 * Note: the parameters to this constructor are specified in panels,
 * whereas width() and height() are specified in pixels.
 *
 * \sa width(), height()
 */
DMDESP::DMDESP(int widthPanels, int heightPanels)
  : Bitmap(widthPanels * DMDESP_NUM_COLUMNS, heightPanels * DMDESP_NUM_ROWS)
  , _doubleBuffer(false)
  , phase(0)
  , fb0(0)
  , fb1(0)
  , displayfb(0)
  , lastRefresh(millis())
{ 
  // Both rendering and display are to fb0 initially.
  fb0 = displayfb = fb; 

  // Initialize SPI to MSB-first, mode 0, clock divider = 2.
    pinMode(DMD_PIN_SPI_SCK, OUTPUT);
    pinMode(DMD_PIN_SPI_MOSI, OUTPUT);
    //pinMode(DMD_PIN_SPI_SS, OUTPUT);
    digitalWrite(DMD_PIN_SPI_SCK, LOW);
    digitalWrite(DMD_PIN_SPI_MOSI, LOW);
    //digitalWrite(DMD_PIN_SPI_SS, HIGH);
    //SPCR |= _BV(MSTR);
    //SPCR |= _BV(SPE);
    //SPCR &= ~(_BV(DORD));   // MSB-first
    //SPCR &= ~0x0C;          // Mode 0
    //SPCR &= ~0x03;          // Clock divider rate 2
    //SPSR |= 0x01;           // MSB of clock divider rate

    // Initialize the DMD-specific pins.
    pinMode(DMD_PIN_PHASE_LSB, OUTPUT);
    pinMode(DMD_PIN_PHASE_MSB, OUTPUT);
    pinMode(DMD_PIN_LATCH, OUTPUT);
    pinMode(DMD_PIN_OUTPUT_ENABLE, OUTPUT);
    digitalWrite(DMD_PIN_PHASE_LSB, LOW);
    digitalWrite(DMD_PIN_PHASE_MSB, LOW);
    digitalWrite(DMD_PIN_LATCH, LOW);
    digitalWrite(DMD_PIN_OUTPUT_ENABLE, LOW);
    digitalWrite(DMD_PIN_SPI_MOSI, HIGH);
}

/**
 * \brief Destroys this dot matrix display handler.
 */
DMDESP::~DMDESP()
{
  if (fb0)
    free(fb0);
  if (fb1)
    free(fb1);
  fb = 0; // Don't free the buffer again in the base class.
}

/**
 * \fn bool DMD::doubleBuffer() const
 * \brief Returns true if the display is double-buffered; false if
 * single-buffered.  The default is false.
 *
 * \sa setDoubleBuffer(), swapBuffers(), refresh()
 */

/**
 * \brief Enables or disables double-buffering according to \a doubleBuffer.
 *
 * When double-buffering is enabled, rendering operations are sent to a
 * memory buffer that isn't currently displayed on-screen.  Once the
 * application has completed the screen update, it calls swapBuffers()
 * to display the current buffer and switch rendering to the other
 * now invisible buffer.
 *
 * Double-buffering is recommended if refresh() is being called from an
 * interrupt service routine, to prevent "tearing" artifacts that result
 * from simultaneous update of a single shared buffer.
 *
 * This function will allocate memory for the extra buffer when
 * \a doubleBuffer is true.  If there is insufficient memory for the
 * second screen buffer, then this class will revert to single-buffered mode.
 *
 * \sa doubleBuffer(), swapBuffers(), refresh()
 */
void DMDESP::setDoubleBuffer(bool doubleBuffer)
{
    if (doubleBuffer != _doubleBuffer) {
        _doubleBuffer = doubleBuffer;
        if (doubleBuffer) {
            // Allocate a new back buffer.
            unsigned int size = _stride * _height;
            fb1 = (uint8_t *)malloc(size);

            // Clear the new back buffer and then switch to it, leaving
            // the current contents of fb0 on the screen.
            if (fb1) {
                memset(fb1, 0xFF, size);
                cli();
                fb = fb1;
                displayfb = fb0;
                sei();
            } else {
                // Failed to allocate the memory, so revert to single-buffered.
                _doubleBuffer = false;
            }
        } else if (fb1) {
            // Disabling double-buffering, so forcibly switch to fb0.
            cli();
            fb = fb0;
            displayfb = fb0;
            sei();

            // Free the unnecessary buffer.
            free(fb1);
            fb1 = 0;
        }
    }
}

/**
 * \brief Swaps the buffers that are used for rendering to the display.
 *
 * When doubleBuffer() is false, this function does nothing.
 * Otherwise the front and back rendering buffers are swapped.
 * See the description of setDoubleBuffer() for more information.
 *
 * The new rendering back buffer will have undefined contents and will
 * probably need to be re-inialized with clear() or fill() before
 * drawing to it.  The swapBuffersAndCopy() function can be used instead
 * to preserve the screen contents from one frame to the next.
 *
 * \sa swapBuffersAndCopy(), setDoubleBuffer()
 */
void DMDESP::swapBuffers()
{
    if (_doubleBuffer) {
        // Turn off interrupts while swapping buffers so that we don't
        // accidentally try to refresh() in the middle of this code.
        cli();
        if (fb == fb0) {
            fb = fb1;
            displayfb = fb0;
        } else {
            fb = fb0;
            displayfb = fb1;
        }
        sei();
    }
}

/**
 * \brief Swaps the buffers that are used for rendering to the display
 * and copies the former back buffer contents to the new back buffer.
 *
 * Normally when swapBuffers() is called, the new rendering back buffer
 * will have undefined contents from two frames prior and must be cleared
 * with clear() or fill() before writing new contents to it.
 * This function instead copies the previous frame into the new
 * rendering buffer so that it can be updated in-place.
 *
 * This function is useful if the screen does not change much from one
 * frame to the next.  If the screen changes a lot between frames, then it
 * is usually better to explicitly clear() or fill() the new back buffer.
 *
 * \sa swapBuffers(), setDoubleBuffer()
 */
void DMDESP::swapBuffersAndCopy()
{
    swapBuffers();
    if (_doubleBuffer)
        memcpy((void *)fb, (void *)displayfb, _stride * _height);
}

/**
 * \brief Performs regular display refresh activities from the
 * application's main loop.
 *
 * \code
 * DMD display;
 *
 * void loop() {
 *     display.loop();
 * }
 * \endcode
 *
 * If you are using a timer interrupt service routine, then call
 * refresh() in response to the interrupt instead of calling loop().
 *
 * \sa refresh()
 */
extern "C" {
  #define USE_US_TIMER
  #include "user_interface.h"
  #include "osapi.h"
  #define os_timer_arm_us(a, b, c) ets_timer_arm_new(a, b, c, 0)
}

os_timer_t dispTimer;
bool tickOccured;

void timerCallback(void *pArg) {

  tickOccured = true;  

}

void dispinit(void) {
  
  system_timer_reinit();
  os_timer_setfn(&dispTimer, timerCallback, NULL);
  os_timer_arm_us(&dispTimer , DMDESP_REFRESH_US, true);
      
}

void DMDESP::loop()
{    
    if (tickOccured == true) {    
      tickOccured = false;
      refresh();
      system_timer_reinit();  
    }

}

// Flip the bits in a byte.  Table generated by genflip.c
static const uint8_t flipBits[256] PROGMEM = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
    0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
    0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
    0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
    0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
    0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
    0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
    0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
    0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
    0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
    0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
    0x3F, 0xBF, 0x7F, 0xFF
};

/**
 * \brief Refresh the display.
 *
 * This function must be called at least once every 5 milliseconds
 * for smooth non-flickering update of the display.  It is usually
 * called by loop(), but can also be called in response to a
 * timer interrupt.
 *
 * If this function is called from an interrupt service routine,
 * then it is recommended that double-buffering be enabled with
 * setDoubleBuffer() to prevent "tearing" artifacts that result
 * from simultaneous update of a single shared buffer.
 *
 * \sa loop(), setDoubleBuffer(), enableTimer1()
 */
void DMDESP::refresh(){

  // Transfer the data for the next group of interleaved rows.
  int stride4 = _stride * 4;
  volatile uint8_t *data0;
  volatile uint8_t *data1;
  volatile uint8_t *data2;
  volatile uint8_t *data3;
  bool flipRow = ((_height & 0x10) == 0);
  for (byte y = 0; y < _height; y += 16) {
    if (!flipRow) {
      // The panels in this row are the right way up.
      data0 = displayfb + _stride * (y + phase);
      data1 = data0 + stride4;
      data2 = data1 + stride4;
      data3 = data2 + stride4;
      for (int x = _stride; x > 0; --x) {
        SPI.write(*data3++);
        SPI.write(*data2++);
        SPI.write(*data1++);
        SPI.write(*data0++);
      }
      flipRow = true;
      
    } else {
        
      data0 = displayfb + _stride * (y + 16 - phase) - 1;
      data1 = data0 - stride4;
      data2 = data1 - stride4;
      data3 = data2 - stride4;
      for (int x = _stride; x > 0; --x) {
        SPI.transfer(pgm_read_byte(&(flipBits[*data3--])));
        SPI.transfer(pgm_read_byte(&(flipBits[*data2--])));
        SPI.transfer(pgm_read_byte(&(flipBits[*data1--])));
        SPI.transfer(pgm_read_byte(&(flipBits[*data0--])));
      }
      flipRow = false;
      
    }
  }

  pinMode(DMD_PIN_OUTPUT_ENABLE, INPUT);
  digitalWrite(DMD_PIN_LATCH, HIGH);
  digitalWrite(DMD_PIN_LATCH, LOW);
  digitalWrite(DMD_PIN_PHASE_LSB,bitRead(phase, LOW));
  digitalWrite(DMD_PIN_PHASE_MSB,bitRead(phase, HIGH));
  pinMode(DMD_PIN_OUTPUT_ENABLE, OUTPUT);
  analogWrite(DMD_PIN_OUTPUT_ENABLE, cr);
  phase = (phase + 1) & 0x03;

}

void DMDESP::start(){
  
  analogWriteFreq(16384);
  pinMode(SCK, SPECIAL);  
  pinMode(MOSI, SPECIAL);
  SPI1C = 0;
  SPI1U = SPIUMOSI | SPIUDUPLEX | SPIUSSE;
  SPI1U1 = (7 << SPILMOSI) | (7 << SPILMISO);
  SPI1C1 = 0;
  SPI1C &= ~(SPICWBO | SPICRBO);
  SPI1U &= ~(SPIUSME);SPI1P &= ~(1<<29);
  SPI.setFrequency(10000000); 
  //SPI.setDataMode(SPI_MODE0);
  //SPI.setBitOrder(MSBFIRST);
  byte jsh=0x11; 
  while(jsh--){if(jsh==DMD_PIN_PHASE_LSB||jsh==DMD_PIN_OUTPUT_ENABLE||jsh==DMD_PIN_PHASE_MSB||jsh==DMD_PIN_LATCH){
  digitalWrite(jsh, 0);pinMode(jsh,OUTPUT);}else{continue;}}
  tickOccured = false;
  dispinit();
  
}

void DMDESP::setBrightness(uint16_t b){
  
  if(b==1){b=2;} 
  cr=b;
  
}


/**
 * \brief Converts an RGB value into a pixel color value.
 *
 * Returns \ref White if any of \a r, \a g, or \a b are non-zero;
 * otherwise returns \ref Black.
 *
 * This function is provided for upwards compatibility with future
 * displays that support full color.  Monochrome applications should
 * use the \ref Black and \ref White constants directly.
 */
DMDESP::Color DMDESP::fromRGB(uint8_t r, uint8_t g, uint8_t b) 
{  
  if (r || g || b)
    return White;
  else
    return Black;    
}