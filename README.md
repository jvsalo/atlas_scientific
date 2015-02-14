# atlas_scientific
Atlas Scientific EZO pH, EC and dissolved oxygen circuit I2C CLI tools

Developed originally for Raspberry Pi. Works at least with version 1.6 firmware (you can get the version with the 'info' command). Output of the commands is intended to be machine (eg. shell script) readable.

Build using 'make'. You might need to install libraries like libi2c-dev if it doesn't build.

Make sure you have set the chip up in I2C mode; UART is the default. See the Atlas Scientific PDF for how to do this.

The I2C device should be /dev/i2c-0 or /dev/i2c-1 depending on your Raspberry Pi revision.

These commands exit with status 0 if everything went OK.

Usege:

```
$ ./atsci_ec 
Atlas Scientific EZO class EC sensor I2C driver
Author: Jaakko Salo (jaakkos@gmail.com)

Usage: atsci_ec <device> <operation> [arguments ...]

Device is the Linux device node, like /dev/i2c-2.
Supported operations:

   read               Get a reading from the probe
   read_avg <count>   Read count times and return average.
   info               Get device type and firmware version
   status             Get reason for previous restart, and voltage at VCC pin
   temp get           Query current temperature compensation value
   temp set <T>       Set temperature compensation value (Celsius)
   K get              Query probe K constant
   K set <K>          Set probe K constant
   led get            Query LED status
   led set <on/off>   Turn LED on/off
   cal get            Get calibration status
   cal clear          Clear all calibration data
   cal dry            Start calibration. Probe must be dry first.
   cal one <EC>       Single point calibration at EC (after 'cal dry'). Ends calibration.
   cal low <EC>       Start dual point calibration (after 'cal dry'). Low point at EC.
   cal high <EC>      Continue dual point calibration (after 'cal dry'). High point at EC.
   sleep              Enter low-power sleep mode.

$ ./atsci_ph 
Atlas Scientific EZO class pH sensor I2C driver
Author: Jaakko Salo (jaakkos@gmail.com)

Usage: atsci_ph <device> <operation> [arguments ...]

Device is the Linux device node, like /dev/i2c-2.
Supported operations:

   read               Get a reading from the probe
   read_avg <count>   Read count times and return average.
   info               Get device type and firmware version
   status             Get reason for previous restart, and voltage at VCC pin
   temp get           Query current temperature compensation value
   temp set <T>       Set temperature compensation value (Celsius)
   led get            Query LED status
   led set <on/off>   Turn LED on/off
   cal get            Get calibration status
   cal clear          Clear all calibration data
   cal mid <pH>       Midpoint calibration at given pH, preferably 7.00. Clears low
                      and high calibration points, so this must be done first!
   cal low <pH>       Lowpoint calibration at given pH, should be from 1.00 to 6.00
   cal high <pH>      Highpoint calibration at given pH, should be from 8.00 to 14.00
   
$ ./atsci_do 
Atlas Scientific EZO class dissolved oxygen sensor I2C driver
Author: Jaakko Salo (jaakkos@gmail.com)

Usage: atsci_do <device> <operation> [arguments ...]

Device is the Linux device node, like /dev/i2c-2.
Supported operations:

   read_saturation     Get saturation reading from the probe
   read_do             Get dissolved oxygen reading in mg/L
   read_avgsat <count> Read count times and return average
   read_avgdo <count>  Read count times and return average
   info                Get device type and firmware version
   status              Get reason for previous restart, and voltage at VCC pin
   temp get            Query current temperature compensation value
   temp set <T>        Set temperature compensation value (Celsius)
   EC get              Query current conductivity compensation value (uS/cm)
   EC set <EC>         Set conductivity compensation value (uS/cm)
   pressure get        Query current pressure compensation value (kPa)
   pressure set <P>    Set pressure compensation value (kPa)
   led get             Query LED status
   led set <on/off>    Turn LED on/off
   cal get             Get calibration status
   cal clear           Clear all calibration data
   cal zero            Calibrate at zero dissolved oxygen level
   cal atmospheric     Calibrate at atmospheric oxygen levels
   sleep               Enter low-power sleep mode.
```
