#include "config.h"

#ifdef ILI9486

#include "spi.h"

#include <memory.h>
#include <stdio.h>

static void ILI9486ClearScreen()
{
  // Since we are doing delta updates to only changed pixels, clear display initially to black for known starting state
  for(int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    SPI_TRANSFER(DISPLAY_SET_CURSOR_X, 0, 0, 0, 0, 0, DISPLAY_WIDTH >> 8, 0, DISPLAY_WIDTH & 0xFF);
    SPI_TRANSFER(DISPLAY_SET_CURSOR_Y, 0, y >> 8, 0, y & 0xFF, 0, DISPLAY_HEIGHT >> 8, 0, DISPLAY_HEIGHT & 0xFF);
    SPITask *clearLine = AllocTask(DISPLAY_WIDTH*2);
    clearLine->cmd = DISPLAY_WRITE_PIXELS;
    memset(clearLine->data, 0, clearLine->size);
    CommitTask(clearLine);
    RunSPITask(clearLine);
    DoneTask(clearLine);
  }
  SPI_TRANSFER(DISPLAY_SET_CURSOR_X, 0, 0, 0, 0, 0, DISPLAY_WIDTH >> 8, 0, DISPLAY_WIDTH & 0xFF);
  SPI_TRANSFER(DISPLAY_SET_CURSOR_Y, 0, 0, 0, 0, 0, DISPLAY_HEIGHT >> 8, 0, DISPLAY_HEIGHT & 0xFF);
}

void InitILI9486()
{
  // If a Reset pin is defined, toggle it briefly high->low->high to enable the device. Some devices do not have a reset pin, in which case compile with GPIO_TFT_RESET_PIN left undefined.
#if defined(GPIO_TFT_RESET_PIN) && GPIO_TFT_RESET_PIN >= 0
  printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
  SET_GPIO_MODE(GPIO_TFT_RESET_PIN, 1);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  CLEAR_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
#endif

  // Do the initialization with a very low SPI bus speed, so that it will succeed even if the bus speed chosen by the user is too high.
  spi->clk = 34;
  __sync_synchronize();

  BEGIN_SPI_COMMUNICATION();
  {
    SPI_TRANSFER(0xB0/*Interface Mode Control*/, 0x00, 0x00/*DE polarity=High enable, PCKL polarity=data fetched at rising time, HSYNC polarity=Low level sync clock, VSYNC polarity=Low level sync clock*/);
    SPI_TRANSFER(0x11/*Sleep OUT*/);
    usleep(120*1000);
    SPI_TRANSFER(0x3A/*Interface Pixel Format*/, 0x00, 0x55/*DPI(RGB Interface)=16bits/pixel, DBI(CPU Interface)=16bits/pixel*/); // This can be switched from 0x55 to 0x66 for 18bits/pixel instead.
    SPI_TRANSFER(0x21/*Display Inversion ON*/);
    SPI_TRANSFER(0xC0/*Power Control 1*/, 0x00, 0x09, 0x00, 0x09);
    SPI_TRANSFER(0xC1/*Power Control 2*/, 0x00, 0x41, 0x00, 0x00);
    SPI_TRANSFER(0xC5/*VCOM Control*/, 0x00, 0x00, 0x00, 0x36);

#define MADCTL_BGR_PIXEL_ORDER (1<<3)
#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
#define MADCTL_ROW_ADDRESS_ORDER_SWAP (1<<7)

#define MADCTL_ROTATE_180_DEGREES 0xC0
    uint8_t madctl = MADCTL_BGR_PIXEL_ORDER;
#ifdef DISPLAY_ROTATE_180_DEGREES
    madctl |= MADCTL_ROTATE_180_DEGREES;
#endif
#if defined(DISPLAY_OUTPUT_LANDSCAPE) && !defined(DISPLAY_FLIP_OUTPUT_XY_IN_SOFTWARE)
    madctl |= MADCTL_ROW_COLUMN_EXCHANGE;
#endif
    SPI_TRANSFER(0x36/*MADCTL: Memory Access Control*/, 0x00, madctl);

    SPI_TRANSFER(0xE0/*Positive Gamma Control*/, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x2C, 0x00, 0x0B, 0x00, 0x0C, 0x00, 0x04, 0x00, 0x4C, 0x00, 0x64, 0x00, 0x36, 0x00, 0x03, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00);
    SPI_TRANSFER(0xE1/*Negative Gamma Control*/, 0x00, 0x0F, 0x00, 0x37, 0x00, 0x37, 0x00, 0x0C, 0x00, 0x0F, 0x00, 0x05, 0x00, 0x50, 0x00, 0x32, 0x00, 0x36, 0x00, 0x04, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x19, 0x00, 0x14, 0x00, 0x0F);
    SPI_TRANSFER(0x11/*Sleep OUT*/);
    usleep(120*1000);
    SPI_TRANSFER(0x29/*Display ON*/);

    ILI9486ClearScreen();
  }
#ifndef USE_DMA_TRANSFERS // For DMA transfers, keep SPI CS & TA active.
  END_SPI_COMMUNICATION();
#endif

  // And speed up to the desired operation speed finally after init is done.
  spi->clk = SPI_BUS_CLOCK_DIVISOR;
}

void DeinitSPIDisplay()
{

}

#endif
