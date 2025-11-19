#ifndef I_SENSOR_INTERFACE_HPP
#define I_SENSOR_INTERFACE_HPP

class ISensorInterface
{
    public:
        ~ISensorInterface() = default;
        virtual bool initialize() = 0;
        virtual double getMoisture() = 0;
        virtual double getTemp() = 0;
        virtual double getHumid() = 0;
        virtual bool isRainDetected() = 0;
        virtual bool isHealthy() = 0;
};

#endif