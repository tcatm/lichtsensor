#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

/* this is libusb, see http://libusb.sourceforge.net/ */
#include <usb.h>

// Same as in main.c
#define USB_LED_OFF 0
#define USB_LED_ON  1

/* Used to get descriptor strings for device identification */
static int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {
	char buffer[256];
	int rval, i;

	// make standard request GET_DESCRIPTOR, type string and given index
	// (e.g. dev->iProduct)
	rval = usb_control_msg(dev,
			USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
			buffer, sizeof(buffer), 1000);

	if(rval < 0) // error
		return rval;

	// rval should be bytes read, but buffer[0] contains the actual response size
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0]; // string is shorter than bytes read

	if(buffer[1] != USB_DT_STRING) // second byte is the data type
		return 0; // invalid return type

	// we're dealing with UTF-16LE here so actual chars is half of rval,
	// and index 0 doesn't count
	rval /= 2;

	/* lossy conversion to ISO Latin1 */
	for(i = 1; i < rval && i < buflen; i++) {
		if(buffer[2 * i + 1] == 0)
			buf[i-1] = buffer[2 * i];
		else
			buf[i-1] = '?'; /* outside of ISO Latin1 range */
	}
	buf[i-1] = 0;

	return i-1;
}

static usb_dev_handle * usbOpenDevice(int vendor, char *vendorName, int product, char *productName) {
	struct usb_bus *bus;
	struct usb_device *dev;
	char devVendor[256], devProduct[256];

	usb_dev_handle * handle = NULL;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for(bus=usb_get_busses(); bus; bus=bus->next) {
		for(dev=bus->devices; dev; dev=dev->next) {
			if(dev->descriptor.idVendor != vendor ||
					dev->descriptor.idProduct != product)
				continue;

			/* we need to open the device in order to query strings */
			if(!(handle = usb_open(dev))) {
				fprintf(stderr, "Warning: cannot open USB device: %s\n",
						usb_strerror());
				continue;
			}

			/* get vendor name */
			if(usbGetDescriptorString(handle, dev->descriptor.iManufacturer, 0x0409, devVendor, sizeof(devVendor)) < 0) {
				fprintf(stderr,
						"Warning: cannot query manufacturer for device: %s\n",
						usb_strerror());
				usb_close(handle);
				continue;
			}

			/* get product name */
			if(usbGetDescriptorString(handle, dev->descriptor.iProduct,
						0x0409, devProduct, sizeof(devVendor)) < 0) {
				fprintf(stderr,
						"Warning: cannot query product for device: %s\n",
						usb_strerror());
				usb_close(handle);
				continue;
			}

			if(strcmp(devVendor, vendorName) == 0 &&
					strcmp(devProduct, productName) == 0)
				return handle;
			else
				usb_close(handle);
		}
	}

	return NULL;
}

void usage(char *cmdname) {
  printf("Usage: %s [-l] [-h]\n", cmdname);
}

void help(char *cmdname) {
  usage(cmdname);
  puts("\t-l\tread sensor value in a loop (1s delay)");
  puts("\t-h\tdisplay this short help and exit");
}

int main(int argc, char **argv) {
	usb_dev_handle *handle = NULL;
	int nBytes = 0;
	int res;
	uint32_t illumination;
  char c;
	bool loop = false;

  while ((c = getopt(argc, argv, "lh")) != -1)
		 switch (c) {
      case 'l':
				loop = true;
				break;
      case 'h':
        help(argv[0]);
        return 0;
    }

	handle = usbOpenDevice(0x16C0, "MetaMeute", 0x03EB, "Lichtsensor");

	if(handle == NULL) {
		fprintf(stderr, "Could not find USB device!\n");
		exit(1);
	}

	usb_detach_kernel_driver_np(handle, 0);

	res = usb_claim_interface(handle, 0);

	if(res < -EBUSY) {
		printf("Device interface not available to be claimed! \n");
		exit(0);
	}

	if(res < -ENOMEM) {
		printf("Insufficient Memory! \n");
		exit(0);
	}

	do {
		res = usb_control_msg(handle, USB_TYPE_CLASS | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
				0x01, 3 << 8 | 1, 0, (char*)&illumination, sizeof(illumination), 5000);

		if (res < 0) {
			fprintf(stderr, "Error!\n");
			break;
		}

		if (res == sizeof(illumination)) {
			printf("%u\n", illumination);
			fflush(stdout);
		}

		if (loop)
			sleep(1);

	} while (loop);

	usb_release_interface(handle, 0);

	usb_close(handle);

	return 0;
}
