#include <fstream>
#include "GPIO.hpp"

void GPIO::write(int num, int value) {
    GPIO::export_gpio(num);
    GPIO::set_output(num);
    std::ofstream ofs(GPIO::path(num, "value"), std::ofstream::out);
    ofs << std::to_string(value);
    ofs.close();
}

void GPIO::set_output(int num) {
    std::ofstream ofs(GPIO::path(num, "direction"), std::ofstream::out);
    ofs << "out";
    ofs.close();
}

void GPIO::export_gpio(int num) {
    std::ofstream ofs("/sys/class/gpio/export", std::ofstream::out);
    ofs << std::to_string(num);
    ofs.close();
}

std::string GPIO::path(int num, std::string node) {
    std::string out("/sys/class/gpio/gpio");
    out += std::to_string(num);
    out += "/";
    out += node;
    return out;
}
