#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "usbdrv/usbdrv.h"

PROGMEM char usbHidReportDescriptor[14] = {
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x35,                    // USAGE (Illumination)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x04,                    // REPORT_SIZE (2)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x81, 0x02                     // INPUT (Data,Var,Abs)
};


uint32_t report;

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t    *rq = (void *)data;

  if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){
    if (rq->bRequest == USBRQ_HID_GET_REPORT) {
      usbMsgPtr = (void *)&report;
      return sizeof(report);
    }
  }
  return 0;
}

enum sensor_states { IDLE, DISCHARGING };

int main() {
  uint32_t illumination;
  enum sensor_states state;

  wdt_enable(WDTO_1S);

  usbInit();

  // enforce re-enumeration
  usbDeviceDisconnect();
  _delay_ms(250);
  wdt_reset();
  usbDeviceConnect();

  sei();

  state = IDLE;

  report = 0;

  while (1) {
    wdt_reset(); // keep the watchdog happy
    usbPoll();

    switch (state) {
      case IDLE:
        // Charge LED in reverse
        DDRA  |= 0x03;
        PORTA &= ~0x02;
        PORTA |= 0x01;
        _delay_us(10);

        // Switch cathode to high-impedance
        DDRA  &= ~0x01;
        PORTA &= ~0x01;
        illumination = 0;

        state = DISCHARGING;

        break;
      case DISCHARGING:
        if (PINA & 0x01 && illumination < 0xffffffff)
          illumination++;
        else {
          report = illumination;
          state = IDLE;
        }

        break;
    }
  }

  return 0;
}

