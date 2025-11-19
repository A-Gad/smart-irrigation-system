#ifndef I_PUMP_INTERFACE_HPP
#define I_PUMP_INTERFACE_HPP

class IPumpInterface {
public:
    virtual ~IPumpInterface() = default;
    
    virtual bool initialize() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool isActive() = 0;
};

#endif