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
	std::cout <<	"Atlas Scientific EZO class pH sensor I2C driver\n"
			"Author: Jaakko Salo (jaakkos@gmail.com)\n"
			"\n"
			"Usage: atsci_ph <device> <operation> [arguments ...]\n"
			"\n"
			"Device is the Linux device node, like /dev/i2c-2.\n"
			"Supported operations:\n"
			"\n"
			"   read               Get a reading from the probe\n"
			"   read_avg <count>   Read count times and return average.\n"
			"   info               Get device type and firmware version\n"
			"   status             Get reason for previous restart, and voltage at VCC pin\n"
			"   temp get           Query current temperature compensation value\n"
			"   temp set <T>       Set temperature compensation value (Celsius)\n"
			"   led get            Query LED status\n"
			"   led set <on/off>   Turn LED on/off\n"
			"   cal get            Get calibration status\n"
			"   cal clear          Clear all calibration data\n"
			"   cal mid <pH>       Midpoint calibration at given pH, preferably 7.00. Clears low\n"
			"                      and high calibration points, so this must be done first!\n"
			"   cal low <pH>       Lowpoint calibration at given pH, should be from 1.00 to 6.00\n"
			"   cal high <pH>      Highpoint calibration at given pH, should be from 8.00 to 14.00\n"
			"\n";

	exit(1);
}

int read_string(std::string &out, int dev) {
	char buf[33] = {0};
	out = "";

	if(read(dev, buf, 32) < 1) {
		perror("read");
		std::cout << "I2C read failed." << std::endl;
		return 1;
	}

	for(int byte=0; byte<32; byte++)
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
	// std::cout << "Outputting: " << out << std::endl;
	return 0;
}

int write_string(const std::string &cmd, int dev) {
	if(write(dev, cmd.c_str(), cmd.size()) != (int)cmd.size()) {
		perror("write");
		std::cout << "I2C write failed." << std::endl;
		return 1;
	}

	return 0;
}

int do_read(std::vector<std::string>& args, int dev, float *out) {
	if(args.size() != 3 && !out) usage();

	if(write_string("R", dev) != 0)
		return 1;

	usleep(1050000); // Sleep min 1 second

	std::string result;
	if(read_string(result, dev) != 0)
		return 1;

	float pH;
	if(sscanf(result.c_str(), "%f", &pH) != 1) {
		std::cout << "Float conversion of the result failed. The raw result was " << result << std::endl;
		return 1;
	}

	if(out) *out = pH;
	else printf("%.2f\n", pH);
	return 0;
}

int do_read_avg(std::vector<std::string>& args, int dev) {
	if(args.size() != 4) usage();

	int count;

	if(sscanf(args[3].c_str(), "%d", &count) != 1) {
		std::cout << "Invalid argument." << std::endl;
		return 1;
	}

	float avg = 0.0;

	for(int i=0; i<count; i++) {
		float sample;

		if(do_read(args, dev, &sample) != 0)
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
	if((result.length() < 3) || (sscanf(result.c_str() + 3, "%f", &temp) != 1))
		std::cout << "Invalid floating point from device: " << result << std::endl;

	std::cout << temp << std::endl;
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
			case '3': std::cout << "Three-point calibrated." << std::endl; break;
			default: std::cout << "Unknown calibration status." << std::endl; break;
		}

		return 0;
        }

	else if(args[3] == "clear") {
                if(args.size() != 4) usage();

                if(write_string(std::string("Cal,clear"), dev) != 0)
                        return 1;

                usleep(350000); // Sleep min 300 milliseconds

                return read_string(result, dev);
	}

	if(args[3] != "mid" && args[3] != "low" && args[3] != "high")
		usage();

	if(args.size() != 5) usage();

	float pH;
	if(sscanf(args[4].c_str(), "%f", &pH) != 1) {
		std::cout << "Invalid floating point as pH." << std::endl;
		return 1;
	}

	char pHstr[6];
	snprintf(pHstr, 6, "%.2f", pH);

	if(write_string(std::string("Cal,") + args[3] + "," + pHstr, dev) != 0)
		return 1;

	usleep(1350000); // Sleep min 1.3 seconds

	return read_string(result, dev);
}

int init_dev(const std::vector<std::string>& args) {
	int dev = open(args[1].c_str(), O_RDWR);
	if(dev < 0) {
		perror("open");
		std::cout << "Failed to open the device node; exiting." << std::endl;
		return -1;
	}

	if(ioctl(dev, I2C_SLAVE, 0x63) < 0) {
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

	if(args[2] == "read") return do_read(args, dev, NULL);
	else if(args[2] == "info") return do_info(args, dev);
	else if(args[2] == "status") return do_status(args, dev);
	else if(args[2] == "temp") return do_temp(args, dev);
	else if(args[2] == "led") return do_led(args, dev);
	else if(args[2] == "cal") return do_cal(args, dev);
	else if(args[2] == "read_avg") return do_read_avg(args, dev);
	else usage();
}
