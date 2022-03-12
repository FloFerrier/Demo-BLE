#include "rtc.h"

#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/rcc.h>

static uint8_t _rtc_dec_to_bcd(uint8_t dec)
{
  return (((dec / 10) << 4) | (dec % 10));
}

static uint8_t _rtc_bcd_to_dec(uint8_t bcd)
{
  return (((bcd >> 4) * 10) + (bcd & 0x0F));
}

static uint32_t rtc_set_hour(uint8_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val);
  return ((((bcd >> 4) & RTC_TR_HT_MASK) << RTC_TR_HT_SHIFT) | \
          (((bcd & 0x0F) & RTC_TR_HU_MASK) << RTC_TR_HU_SHIFT));
}

static uint32_t rtc_set_minute(uint8_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val);
  return ((((bcd >> 4) & RTC_TR_MNT_MASK) << RTC_TR_MNT_SHIFT) | \
          (((bcd & 0x0F) & RTC_TR_MNU_MASK) << RTC_TR_MNU_SHIFT));
}

static uint32_t rtc_set_second(uint8_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val);
  return ((((bcd >> 4) & RTC_TR_ST_MASK) << RTC_TR_ST_SHIFT) | \
          (((bcd & 0x0F) & RTC_TR_SU_MASK) << RTC_TR_SU_SHIFT));
}

/* Date Register contains only 1 byte for storing Year.
 * Year Register = (Year - 1970)
 */
static uint32_t rtc_set_year(uint16_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val - 1970);
  return ((((bcd >> 4) & RTC_DR_YT_MASK) << RTC_DR_YT_SHIFT) | \
          (((bcd & 0x0F) & RTC_DR_YU_MASK) << RTC_DR_YU_SHIFT));
}

static uint32_t rtc_set_week_day(uint8_t val)
{
  return ((val & RTC_DR_WDU_MASK) << RTC_DR_WDU_SHIFT);
}

static uint32_t rtc_set_month(uint8_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val);
  return ((((bcd >> 4) & RTC_DR_MT_MASK) << RTC_DR_MT_SHIFT) | \
          (((bcd & 0x0F) & RTC_DR_MU_MASK) << RTC_DR_MU_SHIFT));
}

static uint32_t rtc_set_day(uint8_t val)
{
  uint8_t bcd = _rtc_dec_to_bcd(val);
  return ((((bcd >> 4) & RTC_DR_DT_MASK) << RTC_DR_DT_SHIFT) | \
          (((bcd & 0x0F) & RTC_DR_DU_MASK) << RTC_DR_DU_SHIFT));
}

static uint8_t rtc_get_hour(void)
{
  uint8_t bcd = (((RTC_TR & (RTC_TR_HT_MASK << RTC_TR_HT_SHIFT)) + \
                 (RTC_TR & (RTC_TR_HU_MASK << RTC_TR_HU_SHIFT))) >> 16);
  return _rtc_bcd_to_dec(bcd);
}

static uint8_t rtc_get_minute(void)
{
  uint8_t bcd = (((RTC_TR & (RTC_TR_MNT_MASK << RTC_TR_MNT_SHIFT)) + \
                 (RTC_TR & (RTC_TR_MNU_MASK << RTC_TR_MNU_SHIFT))) >> 8);
  return _rtc_bcd_to_dec(bcd);
}

static uint8_t rtc_get_second(void)
{
  uint8_t bcd = ((RTC_TR & (RTC_TR_ST_MASK << RTC_TR_ST_SHIFT)) + \
                 (RTC_TR & (RTC_TR_SU_MASK << RTC_TR_SU_SHIFT)));
  return _rtc_bcd_to_dec(bcd);
}

/* Date Register contains only 1 byte for storing Year.
 * Year Register = (Year - 1970)
 */
static uint16_t rtc_get_year(void)
{
  uint16_t bcd = (((RTC_DR & (RTC_DR_YT_MASK << RTC_DR_YT_SHIFT)) + \
                  (RTC_DR & (RTC_DR_YU_MASK << RTC_DR_YU_SHIFT))) >> 16);
  return (_rtc_bcd_to_dec(bcd) + 1970);
}

static uint8_t rtc_get_week_day(void)
{
  return ((RTC_DR & (RTC_DR_WDU_MASK << RTC_DR_WDU_SHIFT)) >> 13);
}

static uint8_t rtc_get_month(void)
{
  uint8_t bcd = (((RTC_DR & (RTC_DR_MT_MASK << RTC_DR_MT_SHIFT)) + \
                 (RTC_DR & (RTC_DR_MU_MASK << RTC_DR_MU_SHIFT))) >> 8);
  return _rtc_bcd_to_dec(bcd);
}

static uint8_t rtc_get_day(void)
{
  uint8_t bcd = ((RTC_DR & (RTC_DR_DT_MASK << RTC_DR_DT_SHIFT)) + \
                 (RTC_DR & (RTC_DR_DU_MASK << RTC_DR_DU_SHIFT)));
  return _rtc_bcd_to_dec(bcd);
}

void rtc_calendar_set(struct tm tmp)
{
  pwr_disable_backup_domain_write_protect();
  rtc_unlock();
  /* Allow to update Calendar value */
  RTC_ISR |= RTC_ISR_INIT;
  while(!(RTC_ISR & RTC_ISR_INITF));

  /* Set Time Register */
  /* Be careful : TR register must be setting in one time */
  uint32_t reg_tr = rtc_set_hour(tmp.tm_hour)   + \
                    rtc_set_minute(tmp.tm_min)  + \
                    rtc_set_second(tmp.tm_sec);
  RTC_TR = reg_tr;
  /* Be careful : DR register must be setting in one time */
  /* Set Date Register */
  uint32_t reg_dr = rtc_set_year(tmp.tm_year + 1900)  + \
                    rtc_set_month(tmp.tm_mon + 1)     + \
                    rtc_set_week_day(tmp.tm_wday + 1) + \
                    rtc_set_day(tmp.tm_mday);
  RTC_DR = reg_dr;

  RTC_CR &= ~RTC_CR_FMT; // 24-hour format

  /* Exit Initialization sequence */
  RTC_ISR &= ~RTC_ISR_INIT;
  rtc_lock();
  pwr_enable_backup_domain_write_protect();

  while(!(RTC_ISR & RTC_ISR_RSF)); // Wait for allowing Read Date and Time register
}

struct tm rtc_calendar_get(void)
{
  struct tm tmp;

  while(RTC_CR & RTC_CR_BYPSHAD);
  tmp.tm_hour  = rtc_get_hour();
  tmp.tm_min   = rtc_get_minute();
  tmp.tm_sec   = rtc_get_second();
  tmp.tm_year  = rtc_get_year() - 1900;
  tmp.tm_mon   = rtc_get_month() - 1;
  tmp.tm_mday  = rtc_get_day();
  tmp.tm_wday  = rtc_get_week_day() - 1;

  return tmp;
}

void rtc_setup(void)
{
  /* Enable Clock for RTC */
  rcc_periph_clock_enable(RCC_PWR);
  rcc_periph_clock_enable(RCC_RTC);

  pwr_disable_backup_domain_write_protect();
  /* Enable LSE for calendar using */
  RCC_BDCR |= RCC_BDCR_LSEON;
  RCC_BDCR |= RCC_BDCR_RTCEN;
  RCC_BDCR |= (1<<8); //RTCSEL at 0b01
  RCC_BDCR &= ~(1<<9); //RTCSEL at 0b01

  while(!(RCC_BDCR & RCC_BDCR_LSERDY));

  pwr_enable_backup_domain_write_protect();
}