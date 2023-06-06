/*
 * Copyright (c) ?? . All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     apps
 * @{
 *
 * @file
 * @brief       camera_func
 *
 * @author      
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel_defines.h"
#include "board.h"
#include "periph/i2c.h"
#include "luid.h"
#include "ztimer.h"

#include "arducam.h"
#include "periph/spi.h"
#include "periph/i2c.h"
#include "ov5642_regs.h"
#define BUFFER_MAX_SIZE 64
uint8_t	Buf1[BUFFER_MAX_SIZE]={0}, Buf2[BUFFER_MAX_SIZE]={0};
void CS_LOW(void)
{
 	gpio_clear(SPI1_CS_PIN);				    
}
void CS_HIGH(void)
{
 	gpio_set(SPI1_CS_PIN);					
}

void ArduCAM_CS_init(void)
{
    gpio_init(SPI1_CS_PIN, GPIO_OUT);
    CS_HIGH();	
}
uint8_t bus_read(int address)
{
	uint8_t value;
    CS_LOW();
	 spi_transfer_byte(1, SPI_CS_UNDEF, 1, address);
	 value = spi_transfer_byte(1, SPI_CS_UNDEF, 1, 0x00);
	 CS_HIGH();
	 return value;
}

uint8_t bus_write(int address,int value)
{	
	CS_LOW();
    ztimer_sleep(ZTIMER_USEC, 10);
	spi_transfer_byte(1, SPI_CS_UNDEF, 1, address);
	spi_transfer_byte(1, SPI_CS_UNDEF, 1, value);
	ztimer_sleep(ZTIMER_USEC, 10);
	CS_HIGH();
	return 1;
}



uint8_t read_reg(uint8_t addr)
{
	uint8_t data;
	data = bus_read(addr &~ 0x80);
	return data;
}

void write_reg(uint8_t addr, uint8_t data)
{
	 bus_write(addr | 0x80, data); 
}

uint8_t read_fifo(void)
{
	uint8_t data;
	data = bus_read(SINGLE_FIFO_READ);
	return data;
}

void set_fifo_burst(void)
{
	spi_transfer_byte(1, SPI_CS_UNDEF, 1, BURST_FIFO_READ);
}


void flush_fifo(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

void start_capture(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

void clear_fifo_flag(void)
{
	write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

uint32_t read_fifo_length(void)
{
	uint32_t len1,len2,len3,len=0;
	len1 = read_reg(FIFO_SIZE1);
	len2 = read_reg(FIFO_SIZE2);
	len3 = read_reg(FIFO_SIZE3) & 0x7f;
	len = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
	return len;	
}

//Set corresponding bit  
void set_bit(uint8_t addr, uint8_t bit)
{
	uint8_t temp;
	temp = read_reg(addr);
	write_reg(addr, temp | bit);
}
//Clear corresponding bit 
void clear_bit(uint8_t addr, uint8_t bit)
{
	uint8_t temp;
	temp = read_reg(addr);
	write_reg(addr, temp & (~bit));
}

//Get corresponding bit status
uint8_t get_bit(uint8_t addr, uint8_t bit)
{
  uint8_t temp;
  temp = read_reg(addr);
  temp = temp & bit;
  return temp;
}

int wrSensorRegs16_8(const struct sensor_reg reglist[])
{
  int err = 0;

  unsigned int reg_addr = 0;
  unsigned char reg_val = 0;
  const struct sensor_reg *next = reglist;

  while ((reg_addr != 0xffff) || (reg_val != 0xff))
  {
    reg_addr = next->reg;
    reg_val = next->val;
    err = i2c_write_reg(1, 0x78>>1, reg_addr, reg_val, I2C_REG16);
    ztimer_sleep(ZTIMER_USEC, 600);
    next++;
  }
  return err;
}

void OV5642_set_JPEG_size(uint8_t size)
{ 
  switch (size)
  {
    case OV5642_320x240:
      wrSensorRegs16_8(ov5642_320x240);
      break;
    case OV5642_640x480:
      wrSensorRegs16_8(ov5642_640x480);
      break;
    case OV5642_1024x768:
      wrSensorRegs16_8(ov5642_1024x768);
      break;
    case OV5642_1280x960:
      wrSensorRegs16_8(ov5642_1280x960);
      break;
    case OV5642_1600x1200:
      wrSensorRegs16_8(ov5642_1600x1200);
      break;
    case OV5642_2048x1536:
      wrSensorRegs16_8(ov5642_2048x1536);
      break;
    case OV5642_2592x1944:
      wrSensorRegs16_8(ov5642_2592x1944);
      break;
    default:
      wrSensorRegs16_8(ov5642_320x240);
      break;
  }
}
uint8_t arducam_init(void){
    uint8_t temp;
    i2c_init(1);
    ArduCAM_CS_init();
    spi_acquire(1, SPI_CS_UNDEF, SPI_MODE_0, SPI_CLK_1MHZ);
    i2c_acquire(1);

    write_reg(0x07, 0x80); //Reset module
    ztimer_sleep(ZTIMER_MSEC, 100);
    write_reg(0x07, 0x00);
    ztimer_sleep(ZTIMER_MSEC, 100);

    write_reg(ARDUCHIP_TEST1, 0x55); 
	temp = read_reg(ARDUCHIP_TEST1);
    if(temp != 0x55) //Arducam module SPI test
        return -ENODEV;

    i2c_read_reg(1, 0x78>>1, 0x300B, &temp, I2C_REG16); 
    if(temp != 0x42) //OV5642 i2c test
        return -ENODEV;


    i2c_write_reg(1, 0x78>>1, 0x3008, 0x08, I2C_REG16);
    wrSensorRegs16_8(OV5642_QVGA_Preview);

    wrSensorRegs16_8(OV5642_JPEG_Capture_QSXGA);
    wrSensorRegs16_8(ov5642_320x240);
    i2c_write_reg(1, 0x78>>1, 0x3818, 0xa8, I2C_REG16);
    i2c_write_reg(1, 0x78>>1, 0x3621, 0x10, I2C_REG16);
    i2c_write_reg(1, 0x78>>1, 0x3801, 0xb0, I2C_REG16);
    i2c_write_reg(1, 0x78>>1, 0x4407, 0x04, I2C_REG16);

    write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);  //VSYNC is active HIGH
    i2c_release(1);
    spi_release(1);
    return 0;
}
