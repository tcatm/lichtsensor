# Abstract

A simple USB light detector consisting of a simple circuit based on an
ATtiny44 and a LED. Readings are accomplished by running a userspace tool on
the host. Each run will return the most recent measurement.

# Measurements

Measured values will be within 0…4294967295 (32 bit value), with 0 meaning "pretty bright"
and higher values meaning "less light".

## Sample Readings

<table>
<tr> <th>Value</th> <th>Environment</th> </tr>
<tr> <td>8…15</td> <td>average desk lamp</td> </tr>
<tr> <td>30.000…60.000</td> <td>clouded sky</td> </tr>
<tr> <td>&gt; 800.000</td> <td>sensor hidden in a drawer</td> </tr>
</table>

# Compiling the source

## Firmware

Run `make` within the `avr` directory. Make sure to the the fuses to "External
Crystal" and disable CLKDIV8 (see `avr/fuses` for the values I used).

## Client

The sample client is a single file. You'll need
[libusb](http://www.libusb.org/) to compile it.
Usually, you can just install the devel package (`apt-get install libusb-dev`)
on Debian-like distributions.

To compile the client, just run:

    gcc -o lichtsensor client/lichtsensor.c -lusb

# Taking readings

Just run the `lichtsensor` binary after connecting the sensor:

    % ./lichtsensor
    18214

If you get a *Operation not permitted*, try running it as root and 
take a look at the [libusb FAQ](http://www.libusb.org/wiki/FAQ#CanIrunLinuxlibusbapplicationswithoutrootrootprivilege).

# Schematic

![Schematic of Lichtsensor](raw/master/schematic.png)

# Pictures

http://imgur.com/a/VfXc4

# Further research

The sensor's readings are likely to drift over time as there is no compensation
or calibration yet.
