/*
  Sketch to show scrolling of the graphics in sprites.
  Scrolling in this way moves the pixels in a defined rectangle
  within the Sprite. By defalt the whole sprite is scrolled.
  The gap left by scrolling is filled with a defined colour.

  Example for library:
  https://github.com/Bodmer/TFT_eSPI

  A Sprite is notionally an invisible graphics screen that is
  kept in the processors RAM. Graphics can be drawn into the
  Sprite just as it can be drawn directly to the screen. Once
  the Sprite is completed it can be plotted onto the screen in
  any position. If there is sufficient RAM then the Sprite can
  be the same size as the screen and used as a frame buffer.

  A 16 bit Sprite occupies (2 * width * height) bytes in RAM.

  An 8 bit Sprite occupies (width * height) bytes in RAM.
 
  https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
*/

/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

TFT_eSprite graph1 = TFT_eSprite(&tft); // Sprite object graph1

TFT_eSprite stext1 = TFT_eSprite(&tft); // Sprite object stext1

TFT_eSprite stext2 = TFT_eSprite(&tft); // Sprite object stext2

int graphVal = 1;
int delta = 1;
int grid = 0;
int tcount = 0;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

//==========================================================================================
void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Create a sprite for the graph
  graph1.setColorDepth(8);
  graph1.createSprite(128, 61);
  graph1.fillSprite(TFT_BLUE); // Note: Sprite is filled with black when created

  // The scroll area is set to the full sprite size upon creation of the sprite
  // but we can change that by defining a smaller area using "setScrollRect()"if needed
  // parameters are x,y,w,h,color as in drawRect(), the color fills the gap left by scrolling
  //graph1.setScrollRect(64, 0, 64, 61, TFT_DARKGREY);  // Try this line to change the graph scroll area

  // Create a sprite for the scrolling numbers
  stext1.setColorDepth(8);
  stext1.createSprite(32, 64);
  stext1.fillSprite(TFT_BLUE); // Fill sprite with blue
  stext1.setScrollRect(0, 0, 32, 64, TFT_BLUE);     // here we set scroll gap fill color to blue
  stext1.setTextColor(TFT_WHITE); // White text, no background
  stext1.setTextDatum(BR_DATUM);  // Bottom right coordinate datum

  // Create a sprite for Hello World
  stext2.setColorDepth(8);
  stext2.createSprite(80, 16);
  stext2.fillSprite(TFT_DARKGREY);
  stext2.setScrollRect(0, 0, 40, 16, TFT_DARKGREY); // Scroll the "Hello" in the first 40 pixels
  stext2.setTextColor(TFT_WHITE); // White text, no background

  Serial.begin(115200);
  SPIClass &spi=SPI;
  if(!SD.begin(15, spi, 4000000, "/sd")){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(SD, "/", 0);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  SD.end();
}

//==========================================================================================
void loop() {
  // Draw point in graph1 sprite at far right edge (this will scroll left later)
  graph1.drawFastVLine(127,60-graphVal,2,TFT_YELLOW); // draw 2 pixel point on graph

  // Draw number in stext1 sprite at 31,63 (bottom right datum set)
  stext1.drawNumber(graphVal, 31, 63, 2); // plot value in font 2

  // Push the sprites onto the TFT at specied coordinates
  graph1.pushSprite(0, 0);
  stext1.pushSprite(0, 64);
  stext2.pushSprite(40, 70);

  // Change the value to plot
  graphVal+=delta;

  // If the value reaches a limit, then change delta of value
  if (graphVal >= 60)     delta = -1;  // ramp down value
  else if (graphVal <= 1) delta = +1;  // ramp up value

  //delay(50); // wait so things do not scroll too fast

  // Now scroll the sprites scroll(dt, dy) where:
  // dx is pixels to scroll, left = negative value, right = positive value
  // dy is pixels to scroll, up = negative value, down = positive value
  graph1.scroll(-1, 0); // scroll graph 1 pixel left, 0 up/down
  stext1.scroll(0,-16); // scroll stext 0 pixels left/right, 16 up
  stext2.scroll(1);     // scroll stext 1 pixel right, up/down default is 0

  // Draw the grid on far right edge of sprite as graph has now moved 1 pixel left
  grid++;
  if (grid >= 10)
  { // Draw a vertical line if we have scrolled 10 times (10 pixels)
    grid = 0;
    graph1.drawFastVLine(127, 0, 61, TFT_NAVY); // draw line on graph
  }
  else
  { // Otherwise draw points spaced 10 pixels for the horizontal grid lines
    for (int p = 0; p <= 60; p += 10) graph1.drawPixel(127, p, TFT_NAVY);
  }

  tcount--;
  if (tcount <=0)
  { // If we have scrolled 40 pixels the redraw text
    tcount = 40;
    stext2.drawString("Hello World", 6, 0, 2); // draw at 6,0 in sprite, font 2
  }

} // Loop back and do it all again
//==========================================================================================


