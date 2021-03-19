#ifndef GPIO_hpp
#define GPIO_hpp

#include <string>

class GPIO {
public:
    static void write(int num, int value);
private:
    static void set_output(int num);
    static void export_gpio(int num);
    static std::string path(int num, std::string node);
};

#endif
