#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <sys/ioctl.h>

void usage() {
	std::cout <<	"Atlas Scientific EZO class dissolved oxygen sensor I2C driver\n"
			"Author: Jaakko Salo (jaakkos@gmail.com)\n"
			"\n"
			"Usage: atsci_do <device> <operation> [arguments ...]\n"
			"\n"
			"Device is the Linux device node, like /dev/i2c-2.\n"
			"Supported operations:\n"
			"\n"
			"   read_saturation     Get saturation reading from the probe\n"
			"   read_do             Get dissolved oxygen reading in mg/L\n"
			"   read_avgsat <count> Read count times and return average\n"
			"   read_avgdo <count>  Read count times and return average\n"
			"   info                Get device type and firmware version\n"
			"   status              Get reason for previous restart, and voltage at VCC pin\n"
			"   temp get            Query current temperature compensation value\n"
			"   temp set <T>        Set temperature compensation value (Celsius)\n"
			"   EC get              Query current conductivity compensation value (uS/cm)\n"
			"   EC set <EC>         Set conductivity compensation value (uS/cm)\n"
			"   pressure get        Query current pressure compensation value (kPa)\n"
			"   pressure set <P>    Set pressure compensation value (kPa)\n"
			"   led get             Query LED status\n"
			"   led set <on/off>    Turn LED on/off\n"
			"   cal get             Get calibration status\n"
			"   cal clear           Clear all calibration data\n"
			"   cal zero            Calibrate at zero dissolved oxygen level\n"
            "   cal atmospheric     Calibrate at atmospheric oxygen levels\n"
			"   sleep               Enter low-power sleep mode.\n"
			"\n";

	exit(1);
}

int read_string(std::string &out, int dev) {
	char buf[65] = {0};
	out = "";

	if(read(dev, buf, 64) < 1) {
		perror("read");
		std::cout << "I2C read failed." << std::endl;
		return 1;
	}

	for(int byte=0; byte<64; byte++)
		buf[byte] &= 0x7F;

	out = std::string(buf);

	if(out[0] != 1) {
		std::cout << "Command failed. The error from device was: ";
		switch((unsigned char)out[0]) {
			case 255: std::cout << "No Data (no pending request)" << std::endl; break;
			case 254: std::cout << "Pending (request still being processed)" << std::endl; break;
			case 2: std::cout << "Failed (the request failed)" << std::endl; break;
			default: std::cout << "Unknown" << std::endl;
		}

		return 1;
	}

	out = out.substr(1);
	//std::cout << "Outputting: " << out << std::endl;
	return 0;
}

int write_string(const std::string &cmd, int dev) {
	//std::cout << "Writing: " << cmd << std::endl;

	if(write(dev, cmd.c_str(), cmd.size()) != (int)cmd.size()) {
		perror("write");
		std::cout << "I2C write failed." << std::endl;
		return 1;
	}

	return 0;
}

int check_and_set_format(int dev) {
	if(write_string("O,?", dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(result.find("%") == std::string::npos) {
		if(write_string("O,%,1", dev) != 0)
			return 1;

		usleep(350000); // Sleep min 300 milliseconds

		if(read_string(result, dev) != 0)
			return 1;
	}

	if(result.find("DO") == std::string::npos) {
		if(write_string("O,DO,1", dev) != 0)
			return 1;

		usleep(350000); // Sleep min 300 milliseconds

		if(read_string(result, dev) != 0)
			return 1;
	}

	return 0;
}

int do_read(int dev, float *dissoxy, float *saturation) {
	if(write_string("R", dev) != 0)
		return 1;

	usleep(1050000); // Sleep min 1 second

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(sscanf(result.c_str(), "%f,%f", dissoxy, saturation) != 2) {
		std::cout << "Float conversion of the result failed. The raw result was " << result << std::endl;
		return 1;
	}

	return 0;
}

int do_read_saturation(std::vector<std::string>& args, int dev, float *out) {
	if(args.size() != 3 && !out) usage();

    if(!out && check_and_set_format(dev) != 0)
        return 1;

    float dissoxy, saturation;
    if(do_read(dev, &dissoxy, &saturation) != 0)
        return 1;

	if(out) *out = saturation;
	else std::cout << saturation << std::endl;

	return 0;
}

int do_read_dissoxy(std::vector<std::string>& args, int dev, float *out) {
	if(args.size() != 3 && !out) usage();

    if(!out && check_and_set_format(dev) != 0)
        return 1;

    float dissoxy, saturation;
    if(do_read(dev, &dissoxy, &saturation) != 0)
        return 1;

	if(out) *out = dissoxy;
	else std::cout << dissoxy << std::endl;

	return 0;
}


int do_read_avgsat(std::vector<std::string>& args, int dev) {
    if(args.size() != 4) usage();

    int count;

    if(sscanf(args[3].c_str(), "%d", &count) != 1) {
        std::cout << "Invalid argument." << std::endl;
        return 1;
    }

    if(check_and_set_format(dev) != 0)
        return 1;

    float avg = 0.0;

    for(int i=0; i<count; i++) {
        float sample;

        if(do_read_saturation(args, dev, &sample) != 0)
            return 1;

        else avg += sample;
    }

    printf("%.3f\n", avg/count);
    return 0;
}

int do_read_avgdo(std::vector<std::string>& args, int dev) {
    if(args.size() != 4) usage();

    int count;

    if(sscanf(args[3].c_str(), "%d", &count) != 1) {
        std::cout << "Invalid argument." << std::endl;
        return 1;
    }

    if(check_and_set_format(dev) != 0)
        return 1;

    float avg = 0.0;

    for(int i=0; i<count; i++) {
        float sample;

        if(do_read_dissoxy(args, dev, &sample) != 0)
            return 1;

        else avg += sample;
    }

    printf("%.3f\n", avg/count);
    return 0;
}

int do_info(const std::vector<std::string>& args, int dev) {
	if(args.size() != 3) usage();

	if(write_string("I", dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(result.length() < 3) {
		std::cout << "Invalid info string returned: " << result << std::endl;
		return 1;
	}
	
	std::cout << "Device info string: " << result.substr(3) << std::endl;
	return 0;
}

int do_status(const std::vector<std::string>& args, int dev) {
        if(args.size() != 3) usage();

        if(write_string("STATUS", dev) != 0)
                return 1;

        usleep(350000); // Sleep min 300 milliseconds

        std::string result;
        if(read_string(result, dev) != 0)
                return 1;

	char reason;
	float vcc;
	if((result.length() < 8) || (sscanf(result.c_str() + 8, "%c,%f", &reason, &vcc) != 2)) {
		std::cout << "Invalid status string returned: " << result << std::endl;
		return 1;
	}

	std::string sreason;
	switch(reason) {
		case 'P': sreason = "power on reset"; break;
		case 'S': sreason = "software reset"; break;
		case 'B': sreason = "brown out reset"; break;
		case 'W': sreason = "watchdog reset"; break;
		default: sreason = "unknown";
	}

	std::cout << "Last restart reason: " << sreason << ", voltage at VCC pin: " << vcc << std::endl;
	return 0;
}

int do_temp(const std::vector<std::string>& args, int dev) {
	if(args.size() < 4) usage();

	std::string tstr;

	if(args[3] == "get") {
		if(args.size() != 4) usage();
		tstr = "?";
	}

	else if(args[3] == "set") {
		if(args.size() != 5) usage();

		float temp;
		if(sscanf(args[4].c_str(), "%f", &temp) != 1) {
			std::cout << "Invalid floating point as temperature: " << args[4] << std::endl;
			return 1;
		}

		tstr = args[4];
	}

	else usage();

	if(write_string(std::string("T,") + tstr, dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(args[3] == "set")
		return 0;

	float temp;
	if((result.length() < 3) || (sscanf(result.c_str(), "?T,%f", &temp) != 1))
		std::cout << "Invalid floating point from device: " << result << std::endl;

	std::cout << temp << std::endl;
	return 0;
}

int do_EC(const std::vector<std::string>& args, int dev) {
	if(args.size() < 4) usage();

	std::string tstr;

	if(args[3] == "get") {
		if(args.size() != 4) usage();
		tstr = "?";
	}

	else if(args[3] == "set") {
		if(args.size() != 5) usage();

		float EC;
		if(sscanf(args[4].c_str(), "%f", &EC) != 1) {
			std::cout << "Invalid floating point as EC: " << args[4] << std::endl;
			return 1;
		}

		tstr = args[4];
	}

	else usage();

	if(write_string(std::string("S,") + tstr, dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(args[3] == "set")
		return 0;

	float EC;
	if((result.length() < 3) || (sscanf(result.c_str(), "?S,%f", &EC) != 1))
		std::cout << "Invalid floating point from device: " << result << std::endl;

	std::cout << EC << std::endl;
	return 0;
}

int do_pressure(const std::vector<std::string>& args, int dev) {
	if(args.size() < 4) usage();

	std::string tstr;

	if(args[3] == "get") {
		if(args.size() != 4) usage();
		tstr = "?";
	}

	else if(args[3] == "set") {
		if(args.size() != 5) usage();

		float pressure;
		if(sscanf(args[4].c_str(), "%f", &pressure) != 1) {
			std::cout << "Invalid floating point as pressure: " << args[4] << std::endl;
			return 1;
		}

		tstr = args[4];
	}

	else usage();

	if(write_string(std::string("P,") + tstr, dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(args[3] == "set")
		return 0;

	float pressure;
	if((result.length() < 3) || (sscanf(result.c_str(), "?P,%f", &pressure) != 1))
		std::cout << "Invalid floating point from device: " << result << std::endl;

	std::cout << pressure << std::endl;
	return 0;
}

int do_led(const std::vector<std::string>& args, int dev) {
	if(args.size() < 4) usage();

	std::string lstr;

	if(args[3] == "get") {
		if(args.size() != 4) usage();
		lstr = "?";
	}

	else if(args[3] == "set") {
		if(args.size() != 5) usage();

		if(args[4] == "on") lstr = "1";
		else if(args[4] == "off") lstr = "0";
		else {
			std::cout << "State must be on or off." << std::endl;
			return 1;
		}
	}

	else usage();

	if(write_string(std::string("L,") + lstr, dev) != 0)
		return 1;

	usleep(350000); // Sleep min 300 milliseconds

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	if(args[3] == "set")
		return 0;

	if(result.length() < 4) {
		std::cout << "Invalid LED state from device: " << result << std::endl;
		return 1;
	}

	switch(result[3]) {
		case '1': std::cout << "on" << std::endl; return 0;
		case '0': std::cout << "off" << std::endl; return 0;
		default:
			std::cout << "Invalid LED state from device: " << result << std::endl;
			return 1;
	}
}

int do_cal(const std::vector<std::string>& args, int dev) {
	std::string result;

    if(args.size() < 4) usage();

    if(args[3] == "get") {
        if(args.size() != 4) usage();

        if(write_string(std::string("Cal,?"), dev) != 0)
            return 1;

        usleep(350000); // Sleep min 300 milliseconds

        if(read_string(result, dev) != 0)
            return 1;

        if(result.length() < 6) {
            std::cout << "Invalid calibration state from device: " << result << std::endl;
            return 1;
        }

        switch(result[5]) {
            case '0': std::cout << "Not calibrated." << std::endl; break;
            case '1': std::cout << "Single-point calibrated." << std::endl; break;
            case '2': std::cout << "Two-point calibrated." << std::endl; break;
            default: std::cout << "Unknown calibration status." << std::endl; break;
        }

        return 0;
    }

    else if(args[3] == "clear") {
        if(args.size() != 4) usage();

        if(write_string("Cal,clear", dev) != 0)
            return 1;

        usleep(500000); // Sleep min 300 milliseconds

        return read_string(result, dev);
    }

    else if(args[3] == "zero") {
        if(args.size() != 4) usage();

        if(write_string("Cal,0", dev) != 0)
            return 1;

        usleep(2000000); // Sleep min 1.3 seconds

        return read_string(result, dev);
    }

    else if(args[3] == "atmospheric") {
        if(args.size() != 4) usage();

        if(write_string("Cal", dev) != 0)
            return 1;

        usleep(2000000); // Sleep min 1.3 seconds

        return read_string(result, dev);
    }

    else {
        std::cout << "No such calibration mode." << std::endl;
        return 1;
    }
}

int do_sleep(const std::vector<std::string>& args, int dev) {
	if(args.size() != 3) usage();

	if(write_string("SLEEP", dev) != 0)
		return 1;

	return 0;
}

int init_dev(const std::vector<std::string>& args) {
	int dev = open(args[1].c_str(), O_RDWR);
	if(dev < 0) {
		perror("open");
		std::cout << "Failed to open the device node; exiting." << std::endl;
		return -1;
	}

	if(ioctl(dev, I2C_SLAVE, 0x61) < 0) {
		perror("ioctl");
		std::cout << "Unable to set I2C slave address." << std::endl;
		return -1;
	}

	return dev;
}

int main(int argc, char **argv) {
	if(argc < 3) usage();
	std::vector<std::string> args(argv, argv+argc);

	int dev = init_dev(args);
	if(dev < 0) return 1;

	if(args[2] == "read_do") return do_read_dissoxy(args, dev, NULL);
	else if(args[2] == "read_saturation") return do_read_saturation(args, dev, NULL);
	else if(args[2] == "read_avgdo") return do_read_avgdo(args, dev);
	else if(args[2] == "read_avgsat") return do_read_avgsat(args, dev);
	else if(args[2] == "info") return do_info(args, dev);
	else if(args[2] == "status") return do_status(args, dev);
	else if(args[2] == "temp") return do_temp(args, dev);
	else if(args[2] == "EC") return do_EC(args, dev);
	else if(args[2] == "pressure") return do_pressure(args, dev);
	else if(args[2] == "led") return do_led(args, dev);
	else if(args[2] == "cal") return do_cal(args, dev);
	else if(args[2] == "sleep") return do_sleep(args, dev);
	else usage();
}
